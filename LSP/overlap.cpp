// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.
//
// Module Name: overlapped.cpp
//
// Description:
//
//    This module is responsible for handling the overlapped IO 
//    passed to us by the upper layer. If the LSP is on NT, then
//    an IO completion port (IOCP) is created and all overlapped
//    IO initiated by the upper layer passes through our IOCP.
//    If this is Win9x then we create a worker thread and execute
//    the overlapped IO via asynchronous procedure calls (APC).
//
#include "provider.h"

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_POOL_COUNT  100         // Default number of OVERLAPPED
#define MAX_PROC_COUNT      32          // Maximum number of worker threads

extern CRITICAL_SECTION    gOverlappedCS; // Protect access to the overlapped funcs
extern HANDLE              hLspHeap;

//
// For each overlapped operation given to us from the upper layer, we'll
// assign a WSAOVERLAPPEDPLUS structure so that we can make the overlapped
// call to the lower layer. The following variables are pointers to these
// structures.
//
LPWSAOVERLAPPEDPLUS FreePool  = NULL,           // List of unused structures
                    PendingStart = NULL,        // List of pending ops - start
                    PendingEnd  = NULL,         // List of pending ops - end
                    OverlappedPool = NULL;      // Entire list of structures

HANDLE              gWorkerThread[MAX_PROC_COUNT] = {NULL, NULL, NULL, NULL, NULL, NULL,
                                                     NULL, NULL, NULL, NULL, NULL, NULL,
                                                     NULL, NULL, NULL, NULL, NULL, NULL,
                                                     NULL, NULL, NULL, NULL, NULL, NULL,
                                                     NULL, NULL, NULL, NULL, NULL, NULL,
                                                     NULL, NULL},
                    ghWakeupSemaphore = NULL,   // Wakeup semaphore
                    ghIocp = NULL;              // Handle to the IOCP
DWORD               gdwThreadCount = -1;        // Number of worker threads

//
// Extenion function pointers declared in extension.cpp
//  These functions are loaded only when the caller calls them.
//
static TCHAR Msg[512]; 

//
// Function prototypes
//
int   ExecuteOverlappedOperation(WSAOVERLAPPEDPLUS *lpOverlapped, BOOL bSynchronous);
int   EnqueueOverlappedOperation(WSAOVERLAPPEDPLUS *op);
void  SetOverlappedInProgress(OVERLAPPED *ol);
void  PutbackOverlappedStructure(WSAOVERLAPPEDPLUS *olp);
DWORD WINAPI   OverlappedManagerThread(LPVOID lpParam);
VOID  CALLBACK CallUserApcProc(ULONG_PTR Context);
void  CheckForContextCleanup(WSAOVERLAPPEDPLUS *ol);


int AllocateFreePool()
{
    DWORD     i;

    //
    // Allocate our overlapped structures as well as zeroes it out
    //
    OverlappedPool = (LPWSAOVERLAPPEDPLUS)HeapAlloc(
                        hLspHeap, 
                        HEAP_ZERO_MEMORY,
                        sizeof(WSAOVERLAPPEDPLUS) * DEFAULT_POOL_COUNT);

    if (!OverlappedPool)
    {
        dbgprint("out of memory!");
        return WSAENOBUFS;
    }
    // Initialize our overlapped structures
    //
    for(i=0; i < DEFAULT_POOL_COUNT-1 ;i++)
    {
        OverlappedPool[i].next = &OverlappedPool[i+1];
    }
    FreePool  = &OverlappedPool[0];

    return NO_ERROR;
}

//
// Function: InitOverlappedManager
//
// Description:
//    This function is called once and determines whether we're running
//    on NT or Win9x. If on NT, create an IO completion port as the
//    intermediary between the upper and lower layer. All overlapped IO
//    will be posted using this IOCP. When IOCP are available we'll create
//    a number of threads (equal to the number of procs) to service the
//    completion of the IO. If on Win9x, we'll create a single thread which
//    will post overlapped IO operations using APCs.
//
int InitOverlappedManager()
{
    DWORD   dwId,
            i;

    EnterCriticalSection(&gOverlappedCS);

    if (gWorkerThread[0] != NULL)
    {
        LeaveCriticalSection(&gOverlappedCS);
        return 0;
    }

    if (AllocateFreePool() == WSAENOBUFS)
    {
        LeaveCriticalSection(&gOverlappedCS);
        return WSAENOBUFS;
    }

    //
    // See if we're on NT by trying to create the completion port. If it
    //  fails then we're on Win9x.
    //
    ghIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                    NULL,
                                    (ULONG_PTR)0,
                                    0);
    if (ghIocp)
    {
        SYSTEM_INFO     sinfo;

        // We're on NT so figure out how many processors we have
        //
        GetSystemInfo(&sinfo);
        gdwThreadCount = sinfo.dwNumberOfProcessors;

        dbgprint("Created IOCP: %d", ghIocp);
    }
    else
    {
        // We're on Win9x so create a semaphore instead. This is used to
        //  wake up the worker thread to service overlapped IO calls.
        //
        ghWakeupSemaphore = CreateSemaphore(NULL,
                                            0,
                                            MAXLONG,
                                            NULL);
        if (!ghWakeupSemaphore)
        {
            dbgprint("InitOverlappedManager: CreateSemaphore() failed: %d", GetLastError());
            LeaveCriticalSection(&gOverlappedCS);
            return WSAEPROVIDERFAILEDINIT;
        }
        // This is Win9x, no multiproc support
        //
        gdwThreadCount = 1;
    }

    dbgprint("Creating %d threads", gdwThreadCount);
    //
    // Create our worker threads
    //
    for(i=0; i < gdwThreadCount ;i++)
    {
        gWorkerThread[i] = CreateThread(NULL, 0, OverlappedManagerThread, (LPVOID)ghIocp, 0, &dwId);
        if (!gWorkerThread[i])
        {
            dbgprint("InitOverlappedManager: CreateThread() failed: %d", GetLastError());
            LeaveCriticalSection(&gOverlappedCS);
            return WSAEPROVIDERFAILEDINIT;
        }
    }
    LeaveCriticalSection(&gOverlappedCS);
    return 0;
}

//
// Function: StopOverlappedManager
//
// Description:
//    This function is called before the DLL gets unloaded. It tries to
//    shutdown the worker threads gracefully before exiting. It also
//    frees/closes allocated memory and handles.
//
int StopOverlappedManager()
{
    DWORD     i;
    int       ret;


    EnterCriticalSection(&gOverlappedCS);
    //
    // Post a completion packet to the IOCP (one for each thread)
    //
    if (ghIocp)
    {
        for(i=0; i < gdwThreadCount ;i++)
        {
            ret = PostQueuedCompletionStatus(ghIocp,
                                        -1,
                                         0,
                                         NULL);
            if (ret == 0)
            {
                dbgprint("PostQueuedCompletionStatus() failed: %d", GetLastError());
            }
        }
        ret = WaitForMultipleObjectsEx(gdwThreadCount, gWorkerThread, TRUE, 4000, TRUE);
    }

    for(i=0; i < gdwThreadCount ;i++)
    {
        CloseHandle(gWorkerThread[i]);
        gWorkerThread[i] = NULL;

        dbgprint("Closing thread");
    }

    if (ghIocp != NULL)
    {
        CloseHandle(ghIocp);
        ghIocp = NULL;
        dbgprint("Closing iocp");
    }
    if (ghWakeupSemaphore != NULL)
    {
        CloseHandle(ghWakeupSemaphore);
        ghWakeupSemaphore = NULL;
    }

    LeaveCriticalSection(&gOverlappedCS);

    return 0;
}

//
// Function: QueueOverlappedOperation
//
// Description:
//    Whenever one of the overlapped enabled Winsock SPI functions are
//    called (as an overlapped operation), it calls this function which
//    determines whether it can execute it immediate (in the case of NT
//    and IOCP) or post it to a queue to be executed by the woker thread
//    via an APC (on Win9x).
//
int QueueOverlappedOperation(WSAOVERLAPPEDPLUS *ol, SOCK_INFO *SocketContext)
{
    BOOL    bSynchronous=FALSE;
    int     err;

    // Set the fields of the overlapped to indicate IO is not complete yet
    //
    SetOverlappedInProgress(ol->lpCallerOverlapped);
    if (ghIocp)
    {
        // If we haven't already added the provider socket to the IOCP then
        //  do it now.
        //
        AcquireSocketLock(SocketContext);
        if (!SocketContext->hIocp)
        {
            SocketContext->hIocp = CreateIoCompletionPort((HANDLE)ol->ProviderSocket,
                                                          ghIocp,
                                                          ol->CallerSocket,
                                                          0);
            if (SocketContext->hIocp == NULL)
            {
                if ((err = GetLastError()) == ERROR_INVALID_PARAMETER)
                {
                    // Hack-o-rama. If the socket option SO_SYNCHRONOUS_(NON)ALERT 
                    // is set then no overlapped operation can be performed on that
                    // socket and tryiing to associate it with a completion port
                    // will fail. The other bad news is that an LSP cannot trap this
                    // setsockopt call. In reality we don't have to do anything. If
                    // an app sets this option and then makes overlapped calls anyway,
                    // then they shouldn't be expecting any overlapped notifications!
                    // This statement is put here in case you want to mark the socket
                    // info structure as synchronous.
                    //
                    bSynchronous = TRUE;
                }
                else
                {
                    dbgprint("QueueOverlappedOperation: CreateIoCompletionPort() "
                              "failed: %d (Prov %d Iocp %d Caller %d 0)", 
                            err, ol->ProviderSocket, 
                            ghIocp, ol->CallerSocket);
                }
            }

            dbgprint("Adding provider handle %X to IOCP", ol->ProviderSocket);
        }
        ReleaseSocketLock(SocketContext);
        //
        // Simply execute the operation
        //
        return ExecuteOverlappedOperation(ol, bSynchronous);
    }
    else
    {
        // Queue up the operation for the worker thread to initiate
        //
        return EnqueueOverlappedOperation(ol);
    }
}

//
// Function: EnqueueOverlappedOperation
//
// Description:
//    Enqueue an overlapped operation to be executed by the worker
//    thread via an APC.
//
int EnqueueOverlappedOperation(WSAOVERLAPPEDPLUS *op)
{
    EnterCriticalSection(&gOverlappedCS);

    if (op == NULL)
        dbgprint("EnqueueOverlappedOperation: op == NULL!");

    op->next = NULL;
    if (!PendingEnd)
    {
        PendingStart = op;
        PendingEnd   = op;
    }
    else
    {
        PendingEnd->next = op;
        PendingEnd       = op;
    }
    // Increment the semaphore count. This lets the worker thread
    // know that there are pending overlapped operations to execute.
    //
    ReleaseSemaphore(ghWakeupSemaphore, 1, NULL);

    LeaveCriticalSection(&gOverlappedCS);

    return WSA_IO_PENDING;
}

// 
// Function: DequeueOverlappedOperation
//
// Description:
//    Once the worker thread is notified that there are pending
//    operations, it calls this function to get the first operation
//    pending in order to execute it.
//
WSAOVERLAPPEDPLUS *DequeueOverlappedOperation()
{
    WSAOVERLAPPEDPLUS *op=NULL;

    EnterCriticalSection(&gOverlappedCS);

    op = PendingStart;
    if (PendingStart)
    {
        PendingStart = op->next;
    }
    if (op == PendingEnd)
    {
        PendingStart = PendingEnd = NULL;
    }

    LeaveCriticalSection(&gOverlappedCS);

    return op;
}

//
// Function: ExecuteOverlappedOperation
//
// Description:
//    This function actually executes an overlapped operation that was queued.
//    If on Win9x we substitute our own completion function in order to intercept
//    the results. If on NT we post the operation to our completion port.
//    This function either returns NO_ERROR if the operation succeeds immediately
//    or the Winsock error code upone failure (or overlapped operation).
//
int ExecuteOverlappedOperation(WSAOVERLAPPEDPLUS *ol, BOOL bSynchronous)
{
    LPWSAOVERLAPPED_COMPLETION_ROUTINE   routine = NULL;
    PROVIDER                            *Provider;
    OVERLAPPED                          *col = NULL;
    DWORD                               *lpdwFlags = NULL,
                                        *lpdwBytes = NULL;
    int                                  ret=SOCKET_ERROR, err=0;

    if (!ghIocp)
        routine = IntermediateCompletionRoutine;

    Provider = ol->Provider;
    //
    // Reset the event handle if present. The handle is masked with 0xFFFFFFFE in
    //  order to zero out the last bit. If the last bit is one and the socket is
    //  associated with a compeltion port then when an overlapped operation is 
    //  called, the operation is not posted to the IO completion port.
    //
    if (ol->lpCallerOverlapped->hEvent != NULL)
    {
        ULONG_PTR   ptr=1;

        ResetEvent((HANDLE)((ULONG_PTR)ol->lpCallerOverlapped->hEvent & ~ptr));
    }

    switch (ol->Operation)
    {
        case LSP_OP_IOCTL:
            lpdwFlags = NULL;
            lpdwBytes = &ol->IoctlArgs.cbBytesReturned;
            ret = Provider->NextProcTable.lpWSPIoctl(
                    ol->ProviderSocket,
                    ol->IoctlArgs.dwIoControlCode,
                    ol->IoctlArgs.lpvInBuffer,
                    ol->IoctlArgs.cbInBuffer,
                    ol->IoctlArgs.lpvOutBuffer,
                    ol->IoctlArgs.cbOutBuffer,
                   &ol->IoctlArgs.cbBytesReturned,
                   &ol->ProviderOverlapped,
                    routine,
                    ol->lpCallerThreadId,
                   &ol->Error
                   );
            break;                         
        case LSP_OP_RECV:
            lpdwFlags = &ol->RecvArgs.dwFlags;
            lpdwBytes = &ol->RecvArgs.dwNumberOfBytesRecvd;
            ret = Provider->NextProcTable.lpWSPRecv(
                    ol->ProviderSocket,
                    ol->RecvArgs.lpBuffers,
                    ol->RecvArgs.dwBufferCount,
                   &ol->RecvArgs.dwNumberOfBytesRecvd,
                   &ol->RecvArgs.dwFlags,
                   &ol->ProviderOverlapped,
                    routine,
                    ol->lpCallerThreadId,
                   &ol->Error
                    );
            break;
        case LSP_OP_RECVFROM:
            lpdwFlags = &ol->RecvFromArgs.dwFlags;
            lpdwBytes = &ol->RecvFromArgs.dwNumberOfBytesRecvd;
            ret = Provider->NextProcTable.lpWSPRecvFrom(
                    ol->ProviderSocket,
                    ol->RecvFromArgs.lpBuffers,
                    ol->RecvFromArgs.dwBufferCount,
                   &ol->RecvFromArgs.dwNumberOfBytesRecvd,
                   &ol->RecvFromArgs.dwFlags,
                    ol->RecvFromArgs.lpFrom,
                    ol->RecvFromArgs.lpFromLen,
                   &ol->ProviderOverlapped,
                    routine,
                    ol->lpCallerThreadId,
                   &ol->Error
                    );
            break;
        case LSP_OP_SEND:
            lpdwFlags = &ol->SendArgs.dwFlags;
            lpdwBytes = &ol->SendArgs.dwNumberOfBytesSent;
            ret = Provider->NextProcTable.lpWSPSend(
                    ol->ProviderSocket,
                    ol->SendArgs.lpBuffers,
                    ol->SendArgs.dwBufferCount,
                   &ol->SendArgs.dwNumberOfBytesSent,
                    ol->SendArgs.dwFlags,
                   &ol->ProviderOverlapped,
                    routine,
                    ol->lpCallerThreadId,
                   &ol->Error
                   );
             break;
        case LSP_OP_SENDTO:
            lpdwFlags = &ol->SendToArgs.dwFlags;
            lpdwBytes = &ol->SendToArgs.dwNumberOfBytesSent;
            ret = Provider->NextProcTable.lpWSPSendTo(
                    ol->ProviderSocket,
                    ol->SendToArgs.lpBuffers,
                    ol->SendToArgs.dwBufferCount,
                   &ol->SendToArgs.dwNumberOfBytesSent,
                    ol->SendToArgs.dwFlags,
                    (SOCKADDR *)&ol->SendToArgs.To,
                    ol->SendToArgs.iToLen,
                   &ol->ProviderOverlapped,
                    routine,
                    ol->lpCallerThreadId,
                   &ol->Error
                    );
            break;
        case LSP_OP_TRANSMITFILE:
            lpdwFlags = &ol->TransmitFileArgs.dwFlags;
            lpdwBytes = NULL;
            ret = Provider->NextProcTableExt.lpfnTransmitFile(
                    ol->ProviderSocket,
                    ol->TransmitFileArgs.hFile,
                    ol->TransmitFileArgs.nNumberOfBytesToWrite,
                    ol->TransmitFileArgs.nNumberOfBytesPerSend,
                   &ol->ProviderOverlapped,
                    ol->TransmitFileArgs.lpTransmitBuffers,
                    ol->TransmitFileArgs.dwFlags
                    );
            if (ret == FALSE)
            {
                ret = SOCKET_ERROR;
                ol->Error = WSAGetLastError();
                WSASetLastError(ol->Error);
            }
            else
                ret = NO_ERROR;
            break;
        case LSP_OP_ACCEPTEX:
            lpdwFlags = NULL;
            lpdwBytes = &ol->AcceptExArgs.dwBytesReceived;
            ret = Provider->NextProcTableExt.lpfnAcceptEx(
                    ol->ProviderSocket,
                    ol->AcceptExArgs.sProviderAcceptSocket,
                    ol->AcceptExArgs.lpOutputBuffer,
                    ol->AcceptExArgs.dwReceiveDataLength,
                    ol->AcceptExArgs.dwLocalAddressLength,
                    ol->AcceptExArgs.dwRemoteAddressLength,
                   &ol->AcceptExArgs.dwBytesReceived,
                   &ol->ProviderOverlapped
                    );
            if (ret == FALSE)
            {
                ret = SOCKET_ERROR;
                ol->Error = WSAGetLastError();
                WSASetLastError(ol->Error);
            }
            else
                ret = NO_ERROR;
            break;
        case LSP_OP_CONNECTEX:
            lpdwFlags = NULL;
            lpdwBytes = &ol->ConnectExArgs.dwBytesSent;
            ret = Provider->NextProcTableExt.lpfnConnectEx(
                    ol->ProviderSocket,
                    (SOCKADDR *)&ol->ConnectExArgs.name,
                    ol->ConnectExArgs.namelen,
                    ol->ConnectExArgs.lpSendBuffer,
                    ol->ConnectExArgs.dwSendDataLength,
                   &ol->ConnectExArgs.dwBytesSent,
                   &ol->ProviderOverlapped
                    );
            if (ret == FALSE)
            {
                ret = SOCKET_ERROR;
                ol->Error = WSAGetLastError();
                WSASetLastError(ol->Error);
            }
            else
                ret = NO_ERROR;
            break;
        case LSP_OP_DISCONNECTEX:
            lpdwFlags = &ol->DisconnectExArgs.dwFlags;
            lpdwBytes = NULL;
            ret = Provider->NextProcTableExt.lpfnDisconnectEx(
                    ol->ProviderSocket,
                   &ol->ProviderOverlapped,
                    ol->DisconnectExArgs.dwFlags,
                    ol->DisconnectExArgs.dwReserved
                    );
            if (ret == FALSE)
            {
                ret = SOCKET_ERROR;
                ol->Error = WSAGetLastError();
                WSASetLastError(ol->Error);
            }
            else
                ret = NO_ERROR;
            break;
        case LSP_OP_TRANSMITPACKETS:
            lpdwFlags = &ol->TransmitPacketsArgs.dwFlags;
            lpdwBytes = NULL;
            ret = Provider->NextProcTableExt.lpfnTransmitPackets(
                    ol->ProviderSocket,
                    ol->TransmitPacketsArgs.lpPacketArray,
                    ol->TransmitPacketsArgs.nElementCount,
                    ol->TransmitPacketsArgs.nSendSize,
                   &ol->ProviderOverlapped,
                    ol->TransmitPacketsArgs.dwFlags
                    );
            if (ret == FALSE)
            {
                ret = SOCKET_ERROR;
                ol->Error = WSAGetLastError();
                WSASetLastError(ol->Error);
            }
            else
                ret = NO_ERROR;
            break;
        case LSP_OP_WSARECVMSG:
            lpdwFlags = NULL;
            lpdwBytes = &ol->WSARecvMsgArgs.dwNumberOfBytesRecvd;
            ret = Provider->NextProcTableExt.lpfnWSARecvMsg(
                    ol->ProviderSocket,
                    ol->WSARecvMsgArgs.lpMsg,
                   &ol->WSARecvMsgArgs.dwNumberOfBytesRecvd,
                   &ol->ProviderOverlapped,
                    ol->lpCallerCompletionRoutine
                    );
            if (ret == FALSE)
            {
                ret = SOCKET_ERROR;
                ol->Error = WSAGetLastError();
                WSASetLastError(ol->Error);
            }
            else
                ret = NO_ERROR;
            break;
        default:
            dbgprint("ExecuteOverlappedOperation: Unknown operation!");
            ret = SOCKET_ERROR;
            break;
    }

    err = ol->Error;

    if ((ret != NO_ERROR) && (ol->Error != WSA_IO_PENDING))
    {
        //
        // If the call immediately fails, update the OVERLAPPED info and return
        //
        ol->lpCallerOverlapped->Offset       = ol->Error;
        ol->lpCallerOverlapped->OffsetHigh   = (lpdwFlags ? *lpdwFlags : 0);
        ol->lpCallerOverlapped->InternalHigh = (lpdwBytes ? *lpdwBytes : 0);

        dbgprint("Overlap op failed immediately: %d", ol->Error);

        CheckForContextCleanup(ol);

        PutbackOverlappedStructure(ol);
    }
    else if ((ret == NO_ERROR) && (bSynchronous == FALSE))
    {
        //
        // Ideally we'd like to complete immediately here but we should wait
        // until the completion port has processed the completion packet
        // associated with this operation.
        //
        err = WSA_IO_PENDING;
        ret = SOCKET_ERROR;

        dbgprint("Succeeded without error");
    }
    else if ((ret == NO_ERROR) && (bSynchronous == TRUE))
    {
        // The winsock call actually blocked and there will be no completion
        // notification on the IOCP.
        //
        dbgprint("Succeeded without error - synchronous socket though");
    }
    else
        dbgprint("WSA_IO_PENDING");

    return ((ret == NO_ERROR) ? ret : err);
}

//
// Function: OverlappedManagerThread
//
// Description:
//    This thread receives the completion notifications for operations
//    posted to our IO completion port. Once we receive a completion
//    we'll complete the operation back to the calling app.
//
DWORD WINAPI OverlappedManagerThread(LPVOID lpParam)
{
    WSAOVERLAPPEDPLUS *lpPid = NULL;
    HANDLE             hIocp = (HANDLE)lpParam;
    ULONG_PTR          key;
    DWORD              dwBytesXfered;
    int                ret;

    while (1)
    {
        if (hIocp)
        {
            ret = GetQueuedCompletionStatus(hIocp,
                    &dwBytesXfered,
                    &key,
                    (LPOVERLAPPED *)&lpPid,
                    INFINITE);
            if ((ret == 0) || (ret == WAIT_TIMEOUT))
            {
                // Socket failures could be reported here so we still
                // call IntermediateCompletionRoutine
                dbgprint("GetQueuedCompletionStatus() failed: %d", GetLastError());
            }
            if (dwBytesXfered == -1)
            {
                // StopOverlappedManager will send a completion packet with -1
                //    bytes transfered to indicate the completion routine
                //    should exit
                dbgprint("OverlappedManagerThread: Received exit message");
                ExitThread(0);
            }

            // Handle the IO that completed
            IntermediateCompletionRoutine(WSA_IO_PENDING,
                    dwBytesXfered,
                    (OVERLAPPED *)lpPid,
                    0);
        }
        else
        {
            ret = WaitForSingleObjectEx(ghWakeupSemaphore,
                    INFINITE, 
                    TRUE);
            if ((ret == WAIT_FAILED) || (ret == WAIT_TIMEOUT))
            {
                dbgprint("OverlappedManagerThread: WaitForSingleObjectEx() failed: %d",
                        GetLastError());
            }
            else
            {
                lpPid = DequeueOverlappedOperation();

                if (!lpPid)
                    continue;

                ExecuteOverlappedOperation(lpPid, FALSE);
            }
        }
    }
    ExitThread(0);
    return 0;
}

void StartOverlappedManager()
{
    //
    // Make sure things are already initalized
    //
    if (!gWorkerThread[0])
    {
        InitOverlappedManager();

        if (!gWorkerThread[0])
        {
            dbgprint("GetOverlappedStructure: Still not properly inititalized!");
            return;
        }
    }
}

//
// Function: GetOverlappedStructure
//
// Description:
//    This returns an unused WSAOVERLAPPEDPLUS structure. We maintain a list
//    of freed structures so that we don't spend too much time allocating and
//    freeing memory.
//
LPWSAOVERLAPPEDPLUS GetOverlappedStructure(SOCK_INFO *SocketContext)
{
    LPWSAOVERLAPPEDPLUS lpWorkerOverlappedPlus = NULL;

    if (!SocketContext)
    {
        dbgprint("GetOverlappedStructure: SocketContext == NULL");
    }

    AcquireSocketLock(SocketContext);

    EnterCriticalSection(&gOverlappedCS);
    //
    // We have to keep track of the number of outstanding overlapped requests 
    // an application has. Otherwise, if the app were to close a socket that 
    // had oustanding overlapped ops remaining, we'd start leaking structures 
    // in OverlappedPool. The idea here is to force the CallerSocket to remain 
    // open until the lower provider has processed all the overlapped requests. 
    // If we closed both the lower socket and the caller socket, we would no 
    // longer be able to correlate completed requests to any apps sockets.
    //
    (SocketContext->dwOutstandingAsync)++;

    if (FreePool)
    {
        lpWorkerOverlappedPlus = FreePool;
        FreePool = FreePool->next;
        lpWorkerOverlappedPlus->next = NULL;
    }
    else // Empty pool
    {
        if (AllocateFreePool() != WSAENOBUFS)
        {
            lpWorkerOverlappedPlus = FreePool;
            FreePool = FreePool->next;
            lpWorkerOverlappedPlus->next = NULL;
        }
    }
    if (lpWorkerOverlappedPlus)
        memset(lpWorkerOverlappedPlus, 0, sizeof(WSAOVERLAPPEDPLUS));

    LeaveCriticalSection(&gOverlappedCS);

    ReleaseSocketLock(SocketContext);

    return  lpWorkerOverlappedPlus;
}

//
// Function: PutbackOverlappedStructure
//
// Description:
//    Once we're done using a WSAOVERLAPPEDPLUS structure we return it to
//    a list of free structures for re-use later.
//
void PutbackOverlappedStructure(WSAOVERLAPPEDPLUS *olp)
{
    EnterCriticalSection(&gOverlappedCS);

    memset(olp, 0, sizeof(WSAOVERLAPPEDPLUS));

    //HeapFree(hLspHeap, 0, olp);

    olp->lpCallerThreadId = (LPWSATHREADID)(ULONG_PTR)0xbadbeef;

    olp->next = FreePool;
    FreePool = olp;

    LeaveCriticalSection(&gOverlappedCS);
}

//
// Function: SetOverlappedInProgress
//
// Description:
//    Simply set the interal fields of an OVERLAPPED structure to the
//    "in progress" state.
//
void SetOverlappedInProgress(OVERLAPPED *ol)
{
    ol->Internal = WSS_OPERATION_IN_PROGRESS;
    ol->InternalHigh = 0;
}

//
// Function: IntermediateCompletionRoutine
//
// Description:
//    Once an overlapped operation completes we call this routine to keep
//    count of the bytes sent/received as well as to complete the operation
//    to the application.
//
void CALLBACK IntermediateCompletionRoutine(DWORD dwError,
                                            DWORD cbTransferred,
                                            LPWSAOVERLAPPED lpOverlapped,
                                            DWORD dwFlags)
{
    LPWSAOVERLAPPEDPLUS olp = (LPWSAOVERLAPPEDPLUS) lpOverlapped;
    SOCK_INFO          *SocketContext,
                       *AcceptSocketContext;
    int                 Error,
                        ret;

    if (lpOverlapped == NULL)
    {
        dbgprint("IntermediateCompletionRoutine: lpOverlapped == NULL!");
        return;
    }
    //
    // We actually already have the socket context for this operation (its in
    //    the WSAOVERLAPPEDPLUS structure but do this anyway to make sure the
    //    socket hasn't been closed as well as to increment the ref count while
    //    we're accessing the SOCK_INFO structure.
    //
    SocketContext = FindAndLockSocketContext(olp->CallerSocket, &Error);
    if (SocketContext == NULL)
    {
        dbgprint("IntermediateCompletionRoutine: WPUQuerySocketHandleContext failed: %d", Error);
        PutbackOverlappedStructure(olp);
        return;
    }

    if (dwError == WSA_IO_PENDING)
    {
        //
        // Get the results of the operation
        //
        dwError = NO_ERROR;
        ret = olp->Provider->NextProcTable.lpWSPGetOverlappedResult(olp->ProviderSocket,
                                                     lpOverlapped,
                                                    &cbTransferred,
                                                     FALSE,
                                                    &dwFlags,
                                                    (int *)&dwError);
        if (ret == FALSE)
        {
            dbgprint("IntermediateCompletionRoutine: WSPGetOverlappedResult failed: %d", dwError);
        }

        dbgprint("Bytes transferred on socket %d: %d [op=%d; err=%d]", 
                olp->CallerSocket, cbTransferred, olp->Operation, dwError);
    }

    olp->lpCallerOverlapped->Offset       = dwError;
    olp->lpCallerOverlapped->OffsetHigh   = dwFlags;
    olp->lpCallerOverlapped->InternalHigh = cbTransferred;

    if (dwError == 0)
    {
        AcquireSocketLock(SocketContext);
        switch (olp->Operation)
        {
            case LSP_OP_RECV:
                SocketContext->BytesRecv += cbTransferred;
                FreeBuffer(olp->RecvArgs.lpBuffers);
                break;
            case LSP_OP_RECVFROM:
                SocketContext->BytesRecv += cbTransferred;
                FreeBuffer(olp->RecvFromArgs.lpBuffers);
                break;
            case LSP_OP_SEND:
                SocketContext->BytesSent += cbTransferred;
                FreeBuffer(olp->SendArgs.lpBuffers);
                break;
            case LSP_OP_SENDTO:
                SocketContext->BytesSent += cbTransferred;
                FreeBuffer(olp->SendToArgs.lpBuffers);
                break;
            case LSP_OP_TRANSMITFILE:
                SocketContext->BytesSent += cbTransferred;
                break;
            case LSP_OP_ACCEPTEX:
                AcceptSocketContext = FindAndLockSocketContext(
                        olp->AcceptExArgs.sAcceptSocket,
                       &Error
                       );
                if (AcceptSocketContext == NULL)
                {
                    dbgprint("IntermediateCompletionRoutine: WPUQuerySocketHandleContext failed (accept socket)");
                }
                AcceptSocketContext->BytesRecv += cbTransferred;
                UnlockSocketContext(AcceptSocketContext, &Error);
                break;
            default:
                break;
        }
        ReleaseSocketLock(SocketContext);
    }

    UnlockSocketContext(SocketContext, &Error);

    //
    // If the app supplied a completion routine, queue it up for completion
    //
    if (olp->lpCallerCompletionRoutine)
    {

        olp->lpCallerOverlapped->Internal = (ULONG_PTR)olp->lpCallerCompletionRoutine;

        if (MainUpCallTable.lpWPUQueueApc(olp->lpCallerThreadId,
                                          CallUserApcProc,
                                         (DWORD_PTR) olp->lpCallerOverlapped,
                                          &Error) == SOCKET_ERROR)
        {
            dbgprint("IntermediateCompletionRoutine: WPUQueueApc() failed: %d", Error);
        }
    }
    else
    {
        // Otherwise we signal that the op has completed
        //
        if (WPUCompleteOverlappedRequest(olp->CallerSocket,
                                     olp->lpCallerOverlapped,
                                     dwError,
                                     cbTransferred,
                                    &Error) == SOCKET_ERROR)
        {
            dbgprint("WPUCompleteOverlappedRequest failed: %d", Error);
        }
        dbgprint("Completing request on socket: %d", olp->CallerSocket);
    }

    //
    // Cleanup the accounting on the socket
    //

    //(SocketContext->dwOutstandingAsync)--;

    CheckForContextCleanup(olp);

    PutbackOverlappedStructure(olp);

    return;
}

//
// Function: CallerUserApcProc
//
// Description:
//    This function completes an overlapped request that supplied an APC.
//
VOID CALLBACK CallUserApcProc(ULONG_PTR Context)
{
    LPOVERLAPPED                        lpOverlapped;
    LPWSAOVERLAPPED_COMPLETION_ROUTINE  UserCompletionRoutine;

    lpOverlapped = (LPOVERLAPPED) Context;
    UserCompletionRoutine = (LPWSAOVERLAPPED_COMPLETION_ROUTINE)lpOverlapped->Internal;
    lpOverlapped->Internal = lpOverlapped->Offset; // To make sure it
                                                   // in no longer is
                                                   // WSS_OPERATION_IN_PROGRESS

    UserCompletionRoutine (
        (DWORD)lpOverlapped->Offset,
        (DWORD)lpOverlapped->InternalHigh,
        lpOverlapped,
        (DWORD)lpOverlapped->OffsetHigh
        );
    return;
}

void CheckForContextCleanup(WSAOVERLAPPEDPLUS *ol)
{
    SOCK_INFO *SocketContext=NULL;
    int        Error;

    SocketContext = FindAndLockSocketContext(ol->CallerSocket, &Error);
    if (SocketContext == NULL)
    {
        return;
    }
    AcquireSocketLock(ol->SockInfo);

    (ol->SockInfo->dwOutstandingAsync)--;

    if ((ol->SockInfo->bClosing) && 
        (ol->SockInfo->dwOutstandingAsync == 0) &&
        (ol->SockInfo->RefCount == 1))
    {
        //
        // If the calling app closed the socket while there were still outstanding
        //  async operations then all the outstanding operations have completed so
        //  we can close the apps socket handle.
        //
        if (MainUpCallTable.lpWPUCloseSocketHandle(ol->CallerSocket, &ol->Error) == SOCKET_ERROR)
        {
            dbgprint("CheckForContextClenaup: WPUCloseSocketHandle() failed: %d", ol->Error);
        }

        ol->SockInfo->LayeredSocket = INVALID_SOCKET;

        RemoveSocketInfo(ol->SockInfo->Provider, ol->SockInfo);

        dbgprint("Closing socket %d Bytes Sent [%lu] Bytes Recv [%lu]", 
                ol->CallerSocket, ol->SockInfo->BytesSent, ol->SockInfo->BytesRecv);

        ReleaseSocketLock(ol->SockInfo);

        ol->SockInfo = NULL;

        DeleteCriticalSection(&SocketContext->SockCritSec);
        dbgprint("Freeing a SOCK_INFO (SocketContext) structure");
        HeapFree(hLspHeap, 0, SocketContext);

        return;
    }
    ReleaseSocketLock(SocketContext);

    UnlockSocketContext(SocketContext, &Error);

    return;
}
