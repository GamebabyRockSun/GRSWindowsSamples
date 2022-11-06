// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.
//
// Module Name: spi.cpp
//
// Description:
//
//    This sample illustrates how to develop a layered service provider that is
//    capable of counting all bytes transmitted through an IP socket. The application
//    reports when sockets are created and reports how many bytes were sent and
//    received when a socket closes. The results are reported using the OutputDebugString
//    API which will allow you to intercept the I/O by using a debugger such as cdb.exe
//    or you can monitor the I/O using dbmon.exe.
//
//    This file contains the 30 SPI functions you are required to implement in a
//    service provider. It also contains the two functions that must be exported
//    from the DLL module DllMain and WSPStartup.
//    

#include "provider.h"
#include "install.h"

#include <stdio.h>
#include <stdlib.h>

#pragma comment( lib, "Ws2_32.lib" )

//
// Globals used across files
//
CRITICAL_SECTION    gCriticalSection,
                    gOverlappedCS,
                    gDebugCritSec;
WSPUPCALLTABLE      MainUpCallTable;
HINSTANCE           hDllInstance = NULL;

LPPROVIDER          gBaseInfo = NULL;
INT                 gLayerCount=0;      // Number of base providers we're layered over

HANDLE              hLspHeap=NULL;

extern HANDLE       ghIocp;             // Handle to IO completion port


void FreeSocketsAndMemory(int *lpErrno);

//
// Need to keep track of which PROVIDERs that are currently executing
//  a blocking Winsock call on a per thread basis.
//
#define SetBlockingProvider(Provider)       \
    (TlsIndex!=0xFFFFFFFF)                  \
        ? TlsSetValue (TlsIndex, Provider)  \
        : NULL

//
// Globals local to this file
//
static DWORD               TlsIndex=0xFFFFFFFF;
static DWORD               gEntryCount = 0;    // how many times WSPStartup has been called
static DWORD               gLayerCatId = 0;    // Catalog ID of our dummy entry
static WSPDATA             gWSPData;
static WSPPROC_TABLE       gProcTable;
static BOOL                bDetached=FALSE;

static TCHAR Msg[512];                         // For outputting debug messages

void dbgprint(char *format,...)
{
    //#ifdef DEBUG
    static  DWORD pid=0;
    va_list vl;
    char    dbgbuf1[2048],
            dbgbuf2[2048];

    if (pid == 0)
    {
        pid = GetCurrentProcessId();
    }

    EnterCriticalSection(&gDebugCritSec);
    va_start(vl, format);
    wvsprintf(dbgbuf1, format, vl);
    wsprintf(dbgbuf2, "%lu: %s\r\n", pid, dbgbuf1);
    va_end(vl);

    OutputDebugString(dbgbuf2);
    LeaveCriticalSection(&gDebugCritSec);
    //#endif
}

void PrintProcTable(LPWSPPROC_TABLE lpProcTable)
{
    #ifdef DBG_PRINTPROCTABLE
    dbgprint("WSPAccept              = 0x%X", lpProcTable->lpWSPAccept);
    dbgprint("WSPAddressToString     = 0x%X", lpProcTable->lpWSPAddressToString);
    dbgprint("WSPAsyncSelect         = 0x%X", lpProcTable->lpWSPAsyncSelect);
    dbgprint("WSPBind                = 0x%X", lpProcTable->lpWSPBind);
    dbgprint("WSPCancelBlockingCall  = 0x%X", lpProcTable->lpWSPCancelBlockingCall);
    dbgprint("WSPCleanup             = 0x%X", lpProcTable->lpWSPCleanup);
    dbgprint("WSPCloseSocket         = 0x%X", lpProcTable->lpWSPCloseSocket);
    dbgprint("WSPConnect             = 0x%X", lpProcTable->lpWSPConnect);
    dbgprint("WSPDuplicateSocket     = 0x%X", lpProcTable->lpWSPDuplicateSocket);
    dbgprint("WSPAccept              = 0x%X", lpProcTable->lpWSPEnumNetworkEvents);
    dbgprint("WSPEventSelect         = 0x%X", lpProcTable->lpWSPEventSelect);
    dbgprint("WSPGetOverlappedResult = 0x%X", lpProcTable->lpWSPGetOverlappedResult);
    dbgprint("WSPGetPeerName         = 0x%X", lpProcTable->lpWSPGetPeerName);
    dbgprint("WSPGetSockOpt          = 0x%X", lpProcTable->lpWSPGetSockOpt);
    dbgprint("WSPGetSockName         = 0x%X", lpProcTable->lpWSPGetSockName);
    dbgprint("WSPGetQOSByName        = 0x%X", lpProcTable->lpWSPGetQOSByName);
    dbgprint("WSPIoctl               = 0x%X", lpProcTable->lpWSPIoctl);
    dbgprint("WSPJoinLeaf            = 0x%X", lpProcTable->lpWSPJoinLeaf);
    dbgprint("WSPListen              = 0x%X", lpProcTable->lpWSPListen);
    dbgprint("WSPRecv                = 0x%X", lpProcTable->lpWSPRecv);
    dbgprint("WSPRecvDisconnect      = 0x%X", lpProcTable->lpWSPRecvDisconnect);
    dbgprint("WSPRecvFrom            = 0x%X", lpProcTable->lpWSPRecvFrom);
    dbgprint("WSPSelect              = 0x%X", lpProcTable->lpWSPSelect);
    dbgprint("WSPSend                = 0x%X", lpProcTable->lpWSPSend);
    dbgprint("WSPSendDisconnect      = 0x%X", lpProcTable->lpWSPSendDisconnect);
    dbgprint("WSPSendTo              = 0x%X", lpProcTable->lpWSPSendTo);
    dbgprint("WSPSetSockOpt          = 0x%X", lpProcTable->lpWSPSetSockOpt);
    dbgprint("WSPShutdown            = 0x%X", lpProcTable->lpWSPShutdown);
    dbgprint("WSPSocket              = 0x%X", lpProcTable->lpWSPSocket);
    dbgprint("WSPStringToAddress     = 0x%X", lpProcTable->lpWSPStringToAddress);
    #endif
}

int VerifyProcTable(LPWSPPROC_TABLE lpProcTable)
{
   if ( lpProcTable->lpWSPAccept &&
        lpProcTable->lpWSPAddressToString &&
        lpProcTable->lpWSPAsyncSelect &&
        lpProcTable->lpWSPBind &&
        lpProcTable->lpWSPCancelBlockingCall &&
        lpProcTable->lpWSPCleanup &&
        lpProcTable->lpWSPCloseSocket &&
        lpProcTable->lpWSPConnect &&
        lpProcTable->lpWSPDuplicateSocket &&
        lpProcTable->lpWSPEnumNetworkEvents &&
        lpProcTable->lpWSPEventSelect &&
        lpProcTable->lpWSPGetOverlappedResult &&
        lpProcTable->lpWSPGetPeerName &&
        lpProcTable->lpWSPGetSockOpt &&
        lpProcTable->lpWSPGetSockName &&
        lpProcTable->lpWSPGetQOSByName &&
        lpProcTable->lpWSPIoctl &&
        lpProcTable->lpWSPJoinLeaf &&
        lpProcTable->lpWSPListen &&
        lpProcTable->lpWSPRecv &&
        lpProcTable->lpWSPRecvDisconnect &&
        lpProcTable->lpWSPRecvFrom &&
        lpProcTable->lpWSPSelect &&
        lpProcTable->lpWSPSend &&
        lpProcTable->lpWSPSendDisconnect &&
        lpProcTable->lpWSPSendTo &&
        lpProcTable->lpWSPSetSockOpt &&
        lpProcTable->lpWSPShutdown &&
        lpProcTable->lpWSPSocket &&
        lpProcTable->lpWSPStringToAddress)
    {
        return NO_ERROR;
    }
    return SOCKET_ERROR;
}

//
// Function: DllMain
//
// Description:
//    Provides initialization when the LSP DLL is loaded. In our case we simply,
//    initialize some critical sections used throughout the DLL.
//
BOOL WINAPI DllMain(IN HINSTANCE hinstDll, IN DWORD dwReason, LPVOID lpvReserved)
{
    switch (dwReason)
    {

        case DLL_PROCESS_ATTACH:
            hDllInstance = hinstDll;
            //
            // Initialize some critical section objects 
            //
            InitializeCriticalSection(&gCriticalSection);
            InitializeCriticalSection(&gOverlappedCS);
            InitializeCriticalSection(&gDebugCritSec);

            TlsIndex = TlsAlloc();
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            bDetached = TRUE;

            EnterCriticalSection(&gCriticalSection);
            if (gBaseInfo)
            {
                int Error;

                FreeSocketsAndMemory(&Error);
            }
            LeaveCriticalSection(&gCriticalSection);

            DeleteCriticalSection(&gCriticalSection);
            DeleteCriticalSection(&gOverlappedCS);
            DeleteCriticalSection(&gDebugCritSec);

            if (lpvReserved == NULL)
            {
                if (TlsIndex != 0xFFFFFFFF)
                {
                    TlsFree(TlsIndex);
                    TlsIndex = 0xFFFFFFFF;
                }
            }
            break;
    }

    return TRUE;
}

//
// Function: WSPAccept
//
// Description:
//    Handle the WSAAccept function. The only special consideration here is the
//    conditional accept callback. You can choose to intercept this by substituting
//    your own callback (you'll need to keep track of the user supplied callback so
//    you can trigger that once your substituted function is triggered).
//
SOCKET WSPAPI WSPAccept (
    SOCKET          s,                      
    struct sockaddr FAR * addr,  
    LPINT           addrlen,                 
    LPCONDITIONPROC lpfnCondition,  
    DWORD_PTR       dwCallbackData,          
    LPINT           lpErrno)
{
    SOCKET     NewProviderSocket;
    SOCKET     NewSocket = INVALID_SOCKET;
    SOCK_INFO *NewSocketContext;
    SOCK_INFO *SocketContext;

    // Query for our per socket info
    //
    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        *lpErrno = WSAENOTSOCK;
        return INVALID_SOCKET;
    }

    //
    // Note: You can subsitute your own conditional accept callback function
    //       in order to intercept this callback. You would have to keep track
    //       of the user's callback function so that you can call that when
    //       your intermediate function executes.
    //
    SetBlockingProvider(SocketContext->Provider);
    NewProviderSocket = SocketContext->Provider->NextProcTable.lpWSPAccept(
                            SocketContext->ProviderSocket, 
                            addr, 
                            addrlen,
                            lpfnCondition, 
                            dwCallbackData, 
                            lpErrno);
    SetBlockingProvider(NULL);
    if (NewProviderSocket != INVALID_SOCKET)
    {
        // The underlying provider received a new connection so lets create our own
        //  socket to pass back up to the application.
        //
        if ((NewSocketContext = CreateSockInfo(SocketContext->Provider,
                                              NewProviderSocket,
                                              SocketContext)) == NULL)
        {
            *lpErrno = WSAENOBUFS;
        }
        else
        {
            if ((NewSocket = MainUpCallTable.lpWPUCreateSocketHandle(
                    SocketContext->Provider->LayeredProvider.ProtocolChain.ChainEntries[0], 
                    (DWORD_PTR) NewSocketContext, 
                    lpErrno)) == INVALID_SOCKET)
            {
                dbgprint("WSPAccept(): WPUCreateSocketHandle() failed: %d", *lpErrno);
            }

            NewSocketContext->LayeredSocket = NewSocket;

            dbgprint("Creating socket %d", NewSocket);
        }
    }
    UnlockSocketContext(SocketContext, lpErrno);

    return NewSocket;
}

//
// Function: WSPAdressToString
//
// Description:
//    Convert an address to string. We simply pass this to the lower provider.
//
int WSPAPI WSPAddressToString(
    LPSOCKADDR          lpsaAddress,            
    DWORD               dwAddressLength,               
    LPWSAPROTOCOL_INFOW lpProtocolInfo,   
    LPWSTR              lpszAddressString,            
    LPDWORD             lpdwAddressStringLength,   
    LPINT               lpErrno)
{
    WSAPROTOCOL_INFOW *pInfo=NULL;
    PROVIDER          *Provider=NULL;
    INT                i, ret;

    // First find the appropriate provider
    //
    for(i=0; i < gLayerCount ;i++)
    {
        if ((gBaseInfo[i].NextProvider.iAddressFamily == lpProtocolInfo->iAddressFamily) &&
            (gBaseInfo[i].NextProvider.iSocketType == lpProtocolInfo->iSocketType) && 
            (gBaseInfo[i].NextProvider.iProtocol   == lpProtocolInfo->iProtocol))
        {
            if (lpProtocolInfo)
            {
                // In case of multiple providers check the provider flags 
                if ( (gBaseInfo[i].NextProvider.dwServiceFlags1 & ~XP1_IFS_HANDLES) != 
                     (lpProtocolInfo->dwServiceFlags1 & ~XP1_IFS_HANDLES) )
                {
                    continue;
                }
            }
            Provider = &gBaseInfo[i];
            pInfo = &gBaseInfo[i].NextProvider;
            break;
        }
    }
    if (Provider == NULL)
    {
        *lpErrno = WSAEINVAL;
        return SOCKET_ERROR;
    }
    // Of course if the next layer isn't a base just pass down lpProtocolInfo.
    //
    if (pInfo->ProtocolChain.ChainLen != BASE_PROTOCOL)
    {
        pInfo = lpProtocolInfo;
    }
   
    SetBlockingProvider(Provider);
    ret = Provider->NextProcTable.lpWSPAddressToString(lpsaAddress, 
                                                       dwAddressLength,               
                                                       pInfo, 
                                                       lpszAddressString, 
                                                       lpdwAddressStringLength, 
                                                       lpErrno);
    SetBlockingProvider(NULL);

    return ret;
}

//
// Function: WSPAsyncSelect
//
// Description:
//    Register specific Winsock events with a socket. We need to substitute
//    the app socket with the provider socket and use our own hidden window.
//
int WSPAPI WSPAsyncSelect (
    SOCKET       s,
    HWND         hWnd,
    unsigned int wMsg,
    long         lEvent,
    LPINT        lpErrno)
{
    SOCK_INFO *SocketContext;
    HWND       hWorkerWindow;
    INT        ret;

    // Make sure the window handle is valid
    //
    ret = SOCKET_ERROR;
    if (IsWindow(hWnd))
    {
        // Verify only valid events have been set
        //
        if ( (lEvent & ~FD_ALL_EVENTS) == 0)
        {
            // Find our provider socket corresonding to this one
            //
            SocketContext = FindAndLockSocketContext(s, lpErrno);
            if (SocketContext != NULL)
            {
                SocketContext->hWnd = hWnd;
                SocketContext->uMsg = wMsg;

                // Get the handle to our hidden window
                //
                if ((hWorkerWindow = GetWorkerWindow()) != NULL)
                {
                    SetBlockingProvider(SocketContext->Provider);
                    ret = SocketContext->Provider->NextProcTable.lpWSPAsyncSelect(
                               SocketContext->ProviderSocket, 
                               hWorkerWindow, 
                               WM_SOCKET, 
                               lEvent, 
                               lpErrno);
                    SetBlockingProvider(NULL);
                }
                else
                {
                    *lpErrno = WSAEINVAL;
                }
                UnlockSocketContext(SocketContext, lpErrno);
            }
            else
            {
                dbgprint("WSPAsyncSelect: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
                *lpErrno = WSAENOTSOCK;
            }
        }
        else
        {
            *lpErrno = WSAEINVAL;
        }
    }
    else
    {
        *lpErrno = WSAEINVAL;
    }

    return ret;
}

//
// Function: WSPBind
//
// Description:
//    Bind the socket to a local address. We just map socket handles and
//    call the lower provider.
//
int WSPAPI WSPBind(
    SOCKET                s,
    const struct sockaddr FAR * name,
    int                   namelen,
    LPINT                 lpErrno)
{
    SOCK_INFO *SocketContext;
    INT        ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPBind: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    SetBlockingProvider(SocketContext->Provider);
    ret = SocketContext->Provider->NextProcTable.lpWSPBind(
        SocketContext->ProviderSocket, 
        name, 
        namelen, 
        lpErrno);
    SetBlockingProvider(NULL);

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPCancelBlockingCall
//
// Description:
//    This call cancels any blocking Winsock call in the current thread only.
//    For every Winsock call that blocks we use thread local storage (TLS) to
//    store a pointer to the provider on which the blocking call was issued.
//    This is necessary since WSACancelBlockingCall takes no arguments (i.e.
//    the LSP needs to keep track of what calls are blocking).
//
int WSPAPI WSPCancelBlockingCall(
    LPINT lpErrno)
{
    PROVIDER *Provider=NULL;
    INT       ret = NO_ERROR;

    Provider = (PROVIDER *)TlsGetValue(TlsIndex);
    if (!Provider)
    {
        ret = Provider->NextProcTable.lpWSPCancelBlockingCall(lpErrno);
    }
    return ret;
}

// 
// Function: WSPCleanup
//
// Description:
//    Decrement the entry count. If equal to zero then we can prepare to have us
//    unloaded. Close any outstanding sockets and free up allocated memory.
//
int WSPAPI WSPCleanup(
    LPINT lpErrno  
    )
{
    int        ret=NO_ERROR;

    if (bDetached)
        return NO_ERROR;

    EnterCriticalSection(&gCriticalSection);

    if (!gEntryCount)
    {
        *lpErrno = WSANOTINITIALISED;

        dbgprint("WSPCleanup returning WSAENOTINITIALISED");

        LeaveCriticalSection(&gCriticalSection);
        return SOCKET_ERROR;
    }
    // Decrement the entry count
    //
    gEntryCount--;

    dbgprint("WSPCleanup: %d", gEntryCount);

    if (gEntryCount == 0)
    {
        dbgprint("WSPCleanup: gEntryCount == 0; cleaning up");

        StopAsyncWindowManager();
        StopOverlappedManager();

        Sleep(200);

        FreeSocketsAndMemory(lpErrno);
    /*
        DecrementLspUsage(hDllInstance, 1);
    */
    }
    LeaveCriticalSection(&gCriticalSection);

    return ret;
}

//
// Function: WSPCloseSocket
//
// Description:
//    Close the socket handle of the app socket as well as the provider socket.
//    However, if there are outstanding async IO requests on the app socket
//    we only close the provider socket. Only when all the IO requests complete
//    (with error) will we then close the app socket (this will occur in
//    the overlapped manager - overlapp.cpp).
//
int WSPAPI WSPCloseSocket(  
    SOCKET s,        
    LPINT  lpErrno
)
{
    SOCK_INFO *SocketContext;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPCloseSocket: WPUQuerySocketHandle() failed: %d", *lpErrno);

        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }
    AcquireSocketLock(SocketContext);

    dbgprint("WSPCloseSocket: Closing layered socket 0x%p (provider 0x%p)",
        s, SocketContext->ProviderSocket);

    //
    // If we there are outstanding async calls on this handle don't close the app
    //  socket handle...only close the provider's handle.  Therefore any errors
    //  incurred can be propogated back to the app socket.
    //
    if ((SocketContext->dwOutstandingAsync != 0) || (SocketContext->RefCount != 1))
    {
        SocketContext->bClosing = TRUE;

        if (SocketContext->Provider->NextProcTable.lpWSPCloseSocket(
                SocketContext->ProviderSocket, 
                lpErrno) == SOCKET_ERROR) 
        {
            *lpErrno = WSAENOTSOCK;
            UnlockSocketContext(SocketContext, lpErrno);
            dbgprint("WSPCloseSocket: Invalid socket handle");
            return SOCKET_ERROR;
        }

        SocketContext->ProviderSocket = INVALID_SOCKET;

        UnlockSocketContext(SocketContext, lpErrno);
        return NO_ERROR;
    }
    //
    // Close the provider socket
    //
    SetBlockingProvider(SocketContext->Provider);
    if (SocketContext->Provider->NextProcTable.lpWSPCloseSocket(
            SocketContext->ProviderSocket, 
            lpErrno) == SOCKET_ERROR) 
    {
        SetBlockingProvider(NULL);
        UnlockSocketContext(SocketContext, lpErrno);
        dbgprint("WSPCloseSocket: Provider close failed");
        return SOCKET_ERROR;
    }
    SetBlockingProvider(NULL);

    SocketContext->ProviderSocket = INVALID_SOCKET;

    //
    // Remove the socket info
    //
    RemoveSocketInfo(SocketContext->Provider, SocketContext);

    //
    // Close the app socket
    //
    if (MainUpCallTable.lpWPUCloseSocketHandle(s, lpErrno) == SOCKET_ERROR)
    {
        dbgprint("WPUCloseSocketHandle failed: %d", *lpErrno);

        ReleaseSocketLock(SocketContext);
        return SOCKET_ERROR;
    }

    dbgprint("Closing socket %d Bytes Sent [%lu] Bytes Recv [%lu]", 
        s, SocketContext->BytesSent, SocketContext->BytesRecv);

    ReleaseSocketLock(SocketContext);

    DeleteCriticalSection(&SocketContext->SockCritSec);
    HeapFree(hLspHeap, 0, SocketContext);

    return NO_ERROR;
}

//
// Function: WSPConnect
//
// Description:
//    Performs a connect call. The only thing we need to do is translate
//    the socket handle.
//
int WSPAPI WSPConnect (
    SOCKET                s,
    const struct sockaddr FAR * name,
    int                   namelen,
    LPWSABUF              lpCallerData,
    LPWSABUF              lpCalleeData,
    LPQOS                 lpSQOS,
    LPQOS                 lpGQOS,
    LPINT                 lpErrno
)
{
    SOCK_INFO *SocketContext;
    INT        ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPConnect: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    SetBlockingProvider(SocketContext->Provider);
    ret = SocketContext->Provider->NextProcTable.lpWSPConnect(
                SocketContext->ProviderSocket, 
                name, 
                namelen, 
                lpCallerData, 
                lpCalleeData,
                lpSQOS, 
                lpGQOS, 
                lpErrno);
    SetBlockingProvider(NULL);

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPDuplicateSocket
//
// Description:
//    This function provides a WSAPROTOCOL_INFOW structure which can be passed
//    to another process to open a handle to the same socket. First we need
//    to translate the user socket into the provider socket and call the underlying
//    WSPDuplicateSocket. Note that the lpProtocolInfo structure passed into us
//    is an out parameter only!
//
int WSPAPI WSPDuplicateSocket(
    SOCKET              s,
    DWORD               dwProcessId,                      
    LPWSAPROTOCOL_INFOW lpProtocolInfo,   
    LPINT               lpErrno)
{
    PROVIDER          *Provider=NULL;
    SOCK_INFO         *SocketContext=NULL;
    DWORD              dwReserved;
    int                ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPDuplicateSocket: WPUQuerySocketHandle() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }
    //
    // Find the underlying provider
    //
    Provider = SocketContext->Provider;

    SetBlockingProvider(Provider);
    ret = Provider->NextProcTable.lpWSPDuplicateSocket(
                SocketContext->ProviderSocket,
                dwProcessId,
                lpProtocolInfo,
                lpErrno);
    SetBlockingProvider(NULL);

    UnlockSocketContext(SocketContext, lpErrno);

    if (ret == NO_ERROR)
    {
        // We want to return the WSAPROTOCOL_INFOW structure of the underlying
        // provider but we need to preserve the reserved info returned by the
        // WSPDuplicateSocket call.
        //
        dwReserved = lpProtocolInfo->dwProviderReserved;
        memcpy(lpProtocolInfo, &Provider->LayeredProvider, sizeof(WSAPROTOCOL_INFOW));
        lpProtocolInfo->dwProviderReserved = dwReserved;
    }
    return ret;    
}

//
// Function: WSPEnumNetworkEvents
//
// Description:
//    Enumerate the network events for a socket. We only need to translate the
//    socket handle.
//
int WSPAPI WSPEnumNetworkEvents(  
    SOCKET             s,
    WSAEVENT           hEventObject,
    LPWSANETWORKEVENTS lpNetworkEvents,
    LPINT              lpErrno)
{
    SOCK_INFO *SocketContext;
    INT        ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPEnumNetworkEvents: WPUQuerySocketHandleContext() failed: %d",
            *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    SetBlockingProvider(SocketContext->Provider);
    ret = SocketContext->Provider->NextProcTable.lpWSPEnumNetworkEvents(
                SocketContext->ProviderSocket,                             
                hEventObject, 
                lpNetworkEvents, 
                lpErrno);
    SetBlockingProvider(NULL);

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPEventSelect
//
// Description:
//    Register the specified events on the socket with the given event handle.
//    All we need to do is translate the socket handle.
//
int WSPAPI WSPEventSelect(
    SOCKET   s,
    WSAEVENT hEventObject,
    long     lNetworkEvents,
    LPINT    lpErrno)
{
    SOCK_INFO *SocketContext;
    INT        ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPEventSelect: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }
    
    SetBlockingProvider(SocketContext->Provider);
    ret = SocketContext->Provider->NextProcTable.lpWSPEventSelect(
                SocketContext->ProviderSocket, 
                hEventObject,
                lNetworkEvents, 
                lpErrno);
    SetBlockingProvider(NULL);

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPGetOverlappedResult
//
// Description:
//    This function reports whether the specified overlapped call has
//    completed. If it has, return the requested information. If not,
//    and fWait is true, wait until completion. Otherwise return an
//    error immediately.
//
BOOL WSPAPI WSPGetOverlappedResult (
    SOCKET          s,
    LPWSAOVERLAPPED lpOverlapped,
    LPDWORD         lpcbTransfer,
    BOOL            fWait,
    LPDWORD         lpdwFlags,
    LPINT           lpErrno)
{
    DWORD ret;

    s;
    if (lpOverlapped->Internal != WSS_OPERATION_IN_PROGRESS) 
    {
        // Operation has completed, update the parameters and return 
        //
        *lpcbTransfer = (DWORD)lpOverlapped->InternalHigh;
        *lpdwFlags = (DWORD)lpOverlapped->OffsetHigh;
        *lpErrno = (INT)lpOverlapped->Offset;

        return (lpOverlapped->Offset == 0 ? TRUE : FALSE);
    }
    else
    {
        // Operation is still in progress
        //
        if (fWait) 
        {
            // Wait on the app supplied event handle. Once the operation
            //  is completed the IOCP or completion routine will fire.
            //  Once that is handled, WPUCompleteOverlappedRequest will
            //  be called which will signal the app event.
            //
            ret = WaitForSingleObject(lpOverlapped->hEvent, INFINITE);
            if ( (ret == WAIT_OBJECT_0) &&
                 (lpOverlapped->Internal != WSS_OPERATION_IN_PROGRESS) )
            {
                *lpcbTransfer = (DWORD)lpOverlapped->InternalHigh;
                *lpdwFlags = (DWORD)lpOverlapped->OffsetHigh;
                *lpErrno = (INT)lpOverlapped->Offset;
                    
                return(lpOverlapped->Offset == 0 ? TRUE : FALSE);
            }
            else if (lpOverlapped->Internal == WSS_OPERATION_IN_PROGRESS)
                *lpErrno = WSA_IO_PENDING;
            else 
                *lpErrno = WSASYSCALLFAILURE;
        }
        else 
            *lpErrno = WSA_IO_INCOMPLETE;
    }
    return FALSE;
}

//
// Function: WSPGetPeerName
//
// Description:
//    Returns the address of the peer. The only thing we need to do is translate
//    the socket handle.
//
int WSPAPI WSPGetPeerName(  
    SOCKET          s,
    struct sockaddr FAR * name,
    LPINT           namelen,
    LPINT           lpErrno)
{
    SOCK_INFO *SocketContext;
    INT        ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPGetPeerName: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    SetBlockingProvider(SocketContext->Provider);
    ret = SocketContext->Provider->NextProcTable.lpWSPGetPeerName(
                SocketContext->ProviderSocket, 
                name,
                namelen, 
                lpErrno);
    SetBlockingProvider(NULL);

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPGetSockName
//
// Description:
//    Returns the local address of a socket. All we need to do is translate
//    the socket handle.
//
int WSPAPI WSPGetSockName(
    SOCKET          s,
    struct sockaddr FAR * name,
    LPINT           namelen,
    LPINT           lpErrno)
{
    SOCK_INFO *SocketContext;
    INT        ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPGetSockName: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    SetBlockingProvider(SocketContext->Provider);
    ret = SocketContext->Provider->NextProcTable.lpWSPGetSockName(
                SocketContext->ProviderSocket, 
                name,
                namelen, 
                lpErrno);
    SetBlockingProvider(NULL);

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPGetSockOpt
//
// Description:
//    Get the specified socket option. All we need to do is translate the
//    socket handle.
//
int WSPAPI WSPGetSockOpt(
    SOCKET     s,
    int        level,
    int        optname,
    char FAR * optval,
    LPINT      optlen,
    LPINT      lpErrno)
{
    SOCK_INFO *SocketContext;
    INT        ret=NO_ERROR;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPGetSockOpt: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }
    //
    // We need to capture this and return our own WSAPROTOCOL_INFO structure.
    // Otherwise, if we translate the handle and pass it to the lower provider
    // we'll return the lower provider's protocol info!
    //
    if ((level == SOL_SOCKET) && ((optname == SO_PROTOCOL_INFO) ||
                                  (optname == SO_PROTOCOL_INFOA) ||
                                  (optname == SO_PROTOCOL_INFOW) ))
    {
        if ((optname == SO_PROTOCOL_INFOW) && (*optlen >= sizeof(WSAPROTOCOL_INFOW)))
        {
            // No conversion necessary, just copy the data
            memcpy(optval, 
                   &SocketContext->Provider->LayeredProvider, 
                   sizeof(WSAPROTOCOL_INFOW));
        }
        else if ((optname == SO_PROTOCOL_INFOA) && (*optval >= sizeof(WSAPROTOCOL_INFOA)))
        {
            // Copy everything but the string
            memcpy(optval,
                   &SocketContext->Provider->LayeredProvider,
                   sizeof(WSAPROTOCOL_INFOW)-WSAPROTOCOL_LEN+1);
            // Convert our saved UNICODE string to ASCII
            WideCharToMultiByte(CP_ACP,
                                0,
                                SocketContext->Provider->LayeredProvider.szProtocol,
                                -1,
                                ((WSAPROTOCOL_INFOA *)optval)->szProtocol,
                                WSAPROTOCOL_LEN+1,
                                NULL,
                                NULL);
        }
        else
        {
            *lpErrno = WSAEFAULT;
            ret = SOCKET_ERROR;
        }
    }
    else
    {
        SetBlockingProvider(SocketContext->Provider);
        ret = SocketContext->Provider->NextProcTable.lpWSPGetSockOpt(
                    SocketContext->ProviderSocket, 
                    level,
                    optname, 
                    optval, 
                    optlen, 
                    lpErrno);
        SetBlockingProvider(NULL);
    }

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPGetQOSByName
//
// Description:
//    Get a QOS template by name. All we need to do is translate the socket
//    handle.
//
BOOL WSPAPI WSPGetQOSByName(
    SOCKET   s,
    LPWSABUF lpQOSName,
    LPQOS    lpQOS,
    LPINT    lpErrno)
{
    SOCK_INFO *SocketContext;
    INT        ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPGetQOSByName: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    SetBlockingProvider(SocketContext->Provider);
    ret = SocketContext->Provider->NextProcTable.lpWSPGetQOSByName(
                SocketContext->ProviderSocket, 
                lpQOSName,
                lpQOS, 
                lpErrno);
    SetBlockingProvider(NULL);

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPIoctl
//
// Description:
//    Invoke an ioctl. In most cases, we just need to translate the socket
//    handle. However, if the dwIoControlCode is SIO_GET_EXTENSION_FUNCTION_POINTER,
//    we'll need to intercept this and return our own function pointers when
//    they're requesting either TransmitFile or AcceptEx. This is necessary so
//    we can trap these calls. Also for PnP OS's (Win2k) we need to trap calls
//    to SIO_QUERY_TARGET_PNP_HANDLE. For this ioctl we simply have to return 
//    the provider socket.
//
int WSPAPI WSPIoctl(
    SOCKET          s,
    DWORD           dwIoControlCode,
    LPVOID          lpvInBuffer,
    DWORD           cbInBuffer,
    LPVOID          lpvOutBuffer,
    DWORD           cbOutBuffer,
    LPDWORD         lpcbBytesReturned,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID   lpThreadId,
    LPINT           lpErrno)
{
    LPWSAOVERLAPPEDPLUS ProviderOverlapped=NULL;
    SOCK_INFO          *SocketContext;
    GUID                AcceptExGuid = WSAID_ACCEPTEX;
    GUID                TransmitFileGuid = WSAID_TRANSMITFILE;
    GUID                GetAcceptExSockAddrsGuid = WSAID_GETACCEPTEXSOCKADDRS;
    GUID                ConnectExGuid = WSAID_CONNECTEX;
    GUID                DisconnectExGuid = WSAID_DISCONNECTEX;
    GUID                TransmitPacketsGuid = WSAID_TRANSMITPACKETS;
    GUID                WSARecvMsgGuid = WSAID_WSARECVMSG;
    int                 ret=NO_ERROR;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPIoctl: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    if (dwIoControlCode == SIO_GET_EXTENSION_FUNCTION_POINTER)
    {
        // Check to see which extension function is being requested.
        //
        if (memcmp (lpvInBuffer, &TransmitFileGuid, sizeof (GUID)) == 0)
        {
            // Return a pointer to our intermediate extesion function
            //
            *((LPFN_TRANSMITFILE *)lpvOutBuffer) = ExtTransmitFile;
            //
            // Attempt to load the lower provider's extension function
            //
            if (!SocketContext->Provider->NextProcTableExt.lpfnTransmitFile)
            {
                SetBlockingProvider(SocketContext->Provider);
                ret = SocketContext->Provider->NextProcTable.lpWSPIoctl(
                        SocketContext->ProviderSocket,
                        SIO_GET_EXTENSION_FUNCTION_POINTER,
                        &TransmitFileGuid,
                        sizeof(GUID),
                        (LPVOID) &SocketContext->Provider->NextProcTableExt.lpfnTransmitFile,
                        sizeof(LPFN_TRANSMITFILE),
                        lpcbBytesReturned,
                        NULL, 
                        NULL, 
                        NULL,
                        lpErrno);
                SetBlockingProvider(NULL);
            }

            UnlockSocketContext(SocketContext, lpErrno);

            return ret;
        }
        else if (memcmp(lpvInBuffer, &AcceptExGuid, sizeof(GUID)) == 0)
        {
            // Return a pointer to our intermediate extension function
            //
            *((LPFN_ACCEPTEX *)lpvOutBuffer) = ExtAcceptEx;
            //
            // Attempt to load the lower provider's extension function
            //
            if (!SocketContext->Provider->NextProcTableExt.lpfnAcceptEx)
            {
                SetBlockingProvider(SocketContext->Provider);
                ret = SocketContext->Provider->NextProcTable.lpWSPIoctl(
                        SocketContext->ProviderSocket,
                        SIO_GET_EXTENSION_FUNCTION_POINTER,
                        &AcceptExGuid,
                        sizeof(GUID),
                        (LPVOID) &SocketContext->Provider->NextProcTableExt.lpfnAcceptEx,
                        sizeof(LPFN_ACCEPTEX),
                        lpcbBytesReturned,
                        NULL,
                        NULL,
                        NULL,
                        lpErrno);
                SetBlockingProvider(NULL);
            }

            UnlockSocketContext(SocketContext, lpErrno);

            return ret;
        }
        else if (memcmp (lpvInBuffer, &ConnectExGuid, sizeof(GUID)) == 0)
        {
            // Return a pointer to our intermediate extension function
            //
            *((LPFN_CONNECTEX *)lpvOutBuffer) = ExtConnectEx;
            //
            // Attempt to load the lower provider's extension function
            //
            if (!SocketContext->Provider->NextProcTableExt.lpfnConnectEx)
            {
                SetBlockingProvider(SocketContext->Provider);
                ret = SocketContext->Provider->NextProcTable.lpWSPIoctl(
                        SocketContext->ProviderSocket,
                        SIO_GET_EXTENSION_FUNCTION_POINTER,
                        &ConnectExGuid,
                        sizeof(GUID),
                        (LPVOID) &SocketContext->Provider->NextProcTableExt.lpfnConnectEx,
                        sizeof(LPFN_CONNECTEX),
                        lpcbBytesReturned,
                        NULL,
                        NULL,
                        NULL,
                        lpErrno);
                SetBlockingProvider(NULL);
            }

            UnlockSocketContext(SocketContext, lpErrno);

            return ret;
        }
        else if (memcmp (lpvInBuffer, &DisconnectExGuid, sizeof(GUID)) == 0)
        {
            // Return a pointer to our intermediate extension function
            //
            *((LPFN_DISCONNECTEX *)lpvOutBuffer) = ExtDisconnectEx;
            //
            // Attempt to load the lower provider's extension function
            //
            if (!SocketContext->Provider->NextProcTableExt.lpfnDisconnectEx)
            {
                SetBlockingProvider(SocketContext->Provider);
                ret = SocketContext->Provider->NextProcTable.lpWSPIoctl(
                        SocketContext->ProviderSocket,
                        SIO_GET_EXTENSION_FUNCTION_POINTER,
                        &DisconnectExGuid,
                        sizeof(GUID),
                        (LPVOID) &SocketContext->Provider->NextProcTableExt.lpfnDisconnectEx,
                        sizeof(LPFN_DISCONNECTEX),
                        lpcbBytesReturned,
                        NULL,
                        NULL,
                        NULL,
                        lpErrno);
                SetBlockingProvider(NULL);
            }

            UnlockSocketContext(SocketContext, lpErrno);

            return ret;
        }
        else if (memcmp (lpvInBuffer, &TransmitPacketsGuid, sizeof(GUID)) == 0)
        {
            // Return a pointer to our intermediate extension function
            //
            *((LPFN_TRANSMITPACKETS *)lpvOutBuffer) = ExtTransmitPackets;
            //
            // Attempt to load the lower provider's extension function
            //
            if (!SocketContext->Provider->NextProcTableExt.lpfnTransmitPackets)
            {
                SetBlockingProvider(SocketContext->Provider);
                ret = SocketContext->Provider->NextProcTable.lpWSPIoctl(
                        SocketContext->ProviderSocket,
                        SIO_GET_EXTENSION_FUNCTION_POINTER,
                        &TransmitPacketsGuid,
                        sizeof(GUID),
                        (LPVOID) &SocketContext->Provider->NextProcTableExt.lpfnTransmitPackets,
                        sizeof(LPFN_TRANSMITPACKETS),
                        lpcbBytesReturned,
                        NULL,
                        NULL,
                        NULL,
                        lpErrno);
                SetBlockingProvider(NULL);
            }

            UnlockSocketContext(SocketContext, lpErrno);

            return ret;
        }
        else if (memcmp (lpvInBuffer, &WSARecvMsgGuid, sizeof(GUID)) == 0)
        {
            // Return a pointer to our intermediate extension function
            //
            *((LPFN_WSARECVMSG *)lpvOutBuffer) = ExtWSARecvMsg;
            //
            // Attempt to load the lower provider's extension function
            //
            if (!SocketContext->Provider->NextProcTableExt.lpfnWSARecvMsg)
            {
                SetBlockingProvider(SocketContext->Provider);
                ret = SocketContext->Provider->NextProcTable.lpWSPIoctl(
                        SocketContext->ProviderSocket,
                        SIO_GET_EXTENSION_FUNCTION_POINTER,
                        &WSARecvMsgGuid,
                        sizeof(GUID),
                        (LPVOID) &SocketContext->Provider->NextProcTableExt.lpfnWSARecvMsg,
                        sizeof(LPFN_WSARECVMSG),
                        lpcbBytesReturned,
                        NULL,
                        NULL,
                        NULL,
                        lpErrno);
                SetBlockingProvider(NULL);
            }

            UnlockSocketContext(SocketContext, lpErrno);

            return ret;
        }
        else if (memcmp (lpvInBuffer, &GetAcceptExSockAddrsGuid, sizeof (GUID)) == 0)
        {
            // No socket handle translation needed, let the call pass through below
            // (i.e. we really don't have any need to intercept this call)
        }
        else 
        {
            UnlockSocketContext(SocketContext, lpErrno);

            *lpErrno = WSAEINVAL;
            return SOCKET_ERROR;
        }
    }
    else if (dwIoControlCode == SIO_QUERY_TARGET_PNP_HANDLE)
    {
        dbgprint("SIO_QUERY_PNP_HANDLE requested");

        *((SOCKET *)lpvOutBuffer) = SocketContext->ProviderSocket;
        *lpcbBytesReturned = sizeof(SocketContext->ProviderSocket);

        if (lpOverlapped)
        {
            ProviderOverlapped = GetOverlappedStructure(SocketContext);
            if (ProviderOverlapped == NULL)
            {
                UnlockSocketContext(SocketContext, lpErrno);

                *lpErrno = WSAENOBUFS;
                return SOCKET_ERROR;
            }

            ProviderOverlapped->lpCallerOverlapped = lpOverlapped;
            CopyOffset(&ProviderOverlapped->ProviderOverlapped, lpOverlapped);
            ProviderOverlapped->SockInfo           = SocketContext;
            ProviderOverlapped->CallerSocket       = s;
            ProviderOverlapped->ProviderSocket     = SocketContext->ProviderSocket;
            ProviderOverlapped->Error              = NO_ERROR;
            ProviderOverlapped->Operation          = LSP_OP_IOCTL;
            ProviderOverlapped->lpCallerThreadId   = lpThreadId;
            ProviderOverlapped->lpCallerCompletionRoutine   = lpCompletionRoutine;
            ProviderOverlapped->Provider = SocketContext->Provider;

            lpOverlapped->Internal = (DWORD_PTR)lpCompletionRoutine;
            lpOverlapped->InternalHigh = *lpcbBytesReturned;

            //
            // Call the completion routine immediately since there is nothing
            //  else to do. For this ioctl all we do is return the provider
            //  socket. If it was called overlapped just complete the operation.
            //

            dbgprint("SIO_QUERY_PNP_HANDLE overlapped");

            IntermediateCompletionRoutine(0,
                                         *lpcbBytesReturned,
                                         (WSAOVERLAPPED *)ProviderOverlapped,
                                          0);
        }
        UnlockSocketContext(SocketContext, lpErrno);

        return NO_ERROR;
    }

    //
    // Check for overlapped I/O
    // 
    if (lpOverlapped)
    {
        ProviderOverlapped = GetOverlappedStructure(SocketContext);
        if (!ProviderOverlapped)
        {
            UnlockSocketContext(SocketContext, lpErrno);

            *lpErrno = WSAENOBUFS;
            return SOCKET_ERROR;
        }

        ProviderOverlapped->lpCallerOverlapped = lpOverlapped;
        CopyOffset(&ProviderOverlapped->ProviderOverlapped, lpOverlapped);
        ProviderOverlapped->SockInfo           = SocketContext;
        ProviderOverlapped->CallerSocket       = s;
        ProviderOverlapped->ProviderSocket     = SocketContext->ProviderSocket;
        ProviderOverlapped->Error              = NO_ERROR;
        ProviderOverlapped->Operation          = LSP_OP_IOCTL;
        ProviderOverlapped->lpCallerThreadId   = lpThreadId;
        ProviderOverlapped->lpCallerCompletionRoutine   = lpCompletionRoutine;
        ProviderOverlapped->IoctlArgs.dwIoControlCode   = dwIoControlCode;
        ProviderOverlapped->IoctlArgs.lpvInBuffer       = lpvInBuffer;
        ProviderOverlapped->IoctlArgs.cbInBuffer        = cbInBuffer;
        ProviderOverlapped->IoctlArgs.lpvOutBuffer      = lpvOutBuffer;
        ProviderOverlapped->IoctlArgs.cbOutBuffer       = cbOutBuffer;
        ProviderOverlapped->IoctlArgs.cbBytesReturned   = (lpcbBytesReturned ? *lpcbBytesReturned : 0);
        ProviderOverlapped->Provider = SocketContext->Provider;

        ret = QueueOverlappedOperation(ProviderOverlapped, SocketContext);

        if (ret != NO_ERROR)
        {
            *lpErrno = ret;
            ret = SOCKET_ERROR;
        }
    }
    else
    {
        SetBlockingProvider(SocketContext->Provider);
        ret = SocketContext->Provider->NextProcTable.lpWSPIoctl(
                SocketContext->ProviderSocket, 
                dwIoControlCode, 
                lpvInBuffer,
                cbInBuffer, 
                lpvOutBuffer, 
                cbOutBuffer, 
                lpcbBytesReturned, 
                lpOverlapped, 
                lpCompletionRoutine, 
                lpThreadId, 
                lpErrno);
        SetBlockingProvider(NULL);
    }

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPJoinLeaf
//
// Description:
//    This function joins a socket to a multipoint session. For those providers
//    that support multipoint semantics there are 2 possible behaviors. First,
//    for IP, WSAJoinLeaf always returns the same socket handle which was passed
//    into it. In this case there is no new socket so we don't want to create
//    any socket context once the lower provider WSPJoinLeaf is called. In the
//    second case, for ATM, a new socket IS created when we call the lower
//    provider. In this case we do want to create a new user socket and create
//    a socket context.
//
SOCKET WSPAPI WSPJoinLeaf(
    SOCKET       s,
    const struct sockaddr FAR * name,
    int          namelen,
    LPWSABUF     lpCallerData,
    LPWSABUF     lpCalleeData,
    LPQOS        lpSQOS,
    LPQOS        lpGQOS,
    DWORD        dwFlags,
    LPINT        lpErrno)
{
    SOCK_INFO *SocketContext;
    SOCKET     NextProviderSocket = INVALID_SOCKET,
               NewSocket = INVALID_SOCKET;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPJoinLeaf: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return INVALID_SOCKET;
    }

    SetBlockingProvider(SocketContext->Provider);
    NextProviderSocket = SocketContext->Provider->NextProcTable.lpWSPJoinLeaf(
            SocketContext->ProviderSocket,                           
            name, 
            namelen, 
            lpCallerData, 
            lpCalleeData, 
            lpSQOS, 
            lpGQOS, 
            dwFlags,                        
            lpErrno);
    SetBlockingProvider(NULL);
    //    
    // If the socket returned from the lower provider is the same as the socket
    //  passed into it then there really isn't a new socket - just return. 
    //  Otherwise, a new socket has been created and we need to create the socket
    //  context and create a user socket to pass back.
    //
    if (NextProviderSocket != SocketContext->ProviderSocket)
    {
        SOCK_INFO *NewSocketContext;

        // Create a new socket context structure
        //
        if ((NewSocketContext = CreateSockInfo(SocketContext->Provider,
                                               NextProviderSocket,
                                               SocketContext)) == NULL)
        {
            *lpErrno = WSAENOBUFS;
        }
        else
        {
            // Create a socket handle to pass to app
            //
            NewSocket = MainUpCallTable.lpWPUCreateSocketHandle(
                        SocketContext->Provider->LayeredProvider.ProtocolChain.ChainEntries[0],
                        (DWORD_PTR)NewSocketContext,
                        lpErrno);
            if (NewSocket == INVALID_SOCKET)
            {
                dbgprint("WSPJoinLeaf: WPUCreateSocketHandle() failed: %d", *lpErrno);
        
                HeapFree(hLspHeap, 0, NewSocketContext);

                UnlockSocketContext(SocketContext, lpErrno);

                *lpErrno = WSAENOBUFS;
                return INVALID_SOCKET;
            }
            NewSocketContext->LayeredSocket = NewSocket;
        }

        UnlockSocketContext(SocketContext, lpErrno);

        return NewSocket;
    }
    else
    {
        UnlockSocketContext(SocketContext, lpErrno);

        return s;
    }
}

//
// Function: WSPListen
//
// Description:
//    This function sets the backlog value on a listening socket. All we need to
//    do is translate the socket handle to the correct provider.
//
int WSPAPI WSPListen(
    SOCKET s,        
    int    backlog,     
    LPINT  lpErrno)
{
    SOCK_INFO *SocketContext;
    INT        ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPListen: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    SetBlockingProvider(SocketContext->Provider);
    ret = SocketContext->Provider->NextProcTable.lpWSPListen(
                SocketContext->ProviderSocket, 
                backlog, 
                lpErrno);
    SetBlockingProvider(NULL);

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPRecv
//
// Description:
//    This function receives data on a given socket and also allows for asynchronous
//    (overlapped) operation. First translate the socket handle to the lower provider
//    handle and then make the receive call. If called with overlap, post the operation
//    to our IOCP or completion routine.
//
int WSPAPI WSPRecv(
    SOCKET          s,
    LPWSABUF        lpBuffers,
    DWORD           dwBufferCount,
    LPDWORD         lpNumberOfBytesRecvd,
    LPDWORD         lpFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID   lpThreadId,
    LPINT           lpErrno)
{
    LPWSAOVERLAPPEDPLUS ProviderOverlapped;
    SOCK_INFO          *SocketContext;
    int                 ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPRecv: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }
    //
    // Check for overlapped I/O
    //
    if (lpOverlapped)
    {
        ProviderOverlapped = GetOverlappedStructure(SocketContext);
        if (!ProviderOverlapped)
        {
            UnlockSocketContext(SocketContext, lpErrno);
            dbgprint("WSPRecv: GetOverlappedStructure() returned NULL");
            *lpErrno = WSAENOBUFS;
            return SOCKET_ERROR;
        }

        ProviderOverlapped->lpCallerOverlapped = lpOverlapped;
        CopyOffset(&ProviderOverlapped->ProviderOverlapped, lpOverlapped);
        ProviderOverlapped->SockInfo           = SocketContext;
        ProviderOverlapped->CallerSocket       = s;
        ProviderOverlapped->ProviderSocket     = SocketContext->ProviderSocket;
        ProviderOverlapped->Error              = NO_ERROR;
        ProviderOverlapped->Operation          = LSP_OP_RECV;
        ProviderOverlapped->lpCallerThreadId   = lpThreadId;
        ProviderOverlapped->lpCallerCompletionRoutine       = lpCompletionRoutine;
        ProviderOverlapped->RecvArgs.lpBuffers              = CopyBuffer(lpBuffers, dwBufferCount);
        ProviderOverlapped->RecvArgs.dwBufferCount          = dwBufferCount;
        ProviderOverlapped->RecvArgs.dwNumberOfBytesRecvd   = (lpNumberOfBytesRecvd ? *lpNumberOfBytesRecvd : 0);
        ProviderOverlapped->RecvArgs.dwFlags                = (lpFlags ? *lpFlags : 0);
        ProviderOverlapped->Provider = SocketContext->Provider;

        ret = QueueOverlappedOperation(ProviderOverlapped, SocketContext);

        if (ret != NO_ERROR)
        {
            *lpErrno = ret;
            ret = SOCKET_ERROR;
        }
    }
    else
    {
        SetBlockingProvider(SocketContext->Provider);
        ret = SocketContext->Provider->NextProcTable.lpWSPRecv(
                SocketContext->ProviderSocket, 
                lpBuffers, 
                dwBufferCount,
                lpNumberOfBytesRecvd, 
                lpFlags, 
                lpOverlapped, 
                lpCompletionRoutine, 
                lpThreadId,
                lpErrno);
        SetBlockingProvider(NULL);
        if (ret != SOCKET_ERROR)
        {
            SocketContext->BytesRecv += *lpNumberOfBytesRecvd;
        }
    }

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPRecvDisconnect
//
// Description:
//    Receive data and disconnect. All we need to do is translate the socket
//    handle to the lower provider.
//
int WSPAPI WSPRecvDisconnect(
    SOCKET   s,
    LPWSABUF lpInboundDisconnectData,
    LPINT    lpErrno)
{
    SOCK_INFO *SocketContext;
    INT        ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPRecvDisconnect: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    SetBlockingProvider(SocketContext->Provider);
    ret = SocketContext->Provider->NextProcTable.lpWSPRecvDisconnect(
                SocketContext->ProviderSocket,                           
                lpInboundDisconnectData, 
                lpErrno);
    SetBlockingProvider(NULL);

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPRecvFrom
//
// Description:
//    This function receives data on a given socket and also allows for asynchronous
//    (overlapped) operation. First translate the socket handle to the lower provider
//    handle and then make the receive call. If called with overlap, post the operation
//    to our IOCP or completion routine.
//
int WSPAPI WSPRecvFrom(
    SOCKET          s,
    LPWSABUF        lpBuffers,
    DWORD           dwBufferCount,
    LPDWORD         lpNumberOfBytesRecvd,
    LPDWORD         lpFlags,
    struct sockaddr FAR * lpFrom,
    LPINT           lpFromLen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID   lpThreadId,
    LPINT           lpErrno)
{
    LPWSAOVERLAPPEDPLUS ProviderOverlapped;
    SOCK_INFO          *SocketContext;
    int                 ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPRecvFrom: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }
    //
    // Check for overlapped I/O
    //
    if (lpOverlapped)
    {
        ProviderOverlapped = GetOverlappedStructure(SocketContext);
        if (!ProviderOverlapped)
        {
            UnlockSocketContext(SocketContext, lpErrno);
            dbgprint("WSPRecvFrom: GetOverlappedStructure() returned NULL");

            *lpErrno = WSAENOBUFS;
            return SOCKET_ERROR;
        }

        ProviderOverlapped->lpCallerOverlapped = lpOverlapped;
        CopyOffset(&ProviderOverlapped->ProviderOverlapped, lpOverlapped);
        ProviderOverlapped->SockInfo           = SocketContext;
        ProviderOverlapped->CallerSocket       = s;
        ProviderOverlapped->ProviderSocket     = SocketContext->ProviderSocket;
        ProviderOverlapped->Error              = NO_ERROR;
        ProviderOverlapped->Operation          = LSP_OP_RECVFROM;
        ProviderOverlapped->lpCallerThreadId   = lpThreadId;
        ProviderOverlapped->lpCallerCompletionRoutine           = lpCompletionRoutine;
        ProviderOverlapped->RecvFromArgs.lpBuffers              = CopyBuffer(lpBuffers, dwBufferCount);
        ProviderOverlapped->RecvFromArgs.dwBufferCount          = dwBufferCount;
        ProviderOverlapped->RecvFromArgs.dwNumberOfBytesRecvd   = (lpNumberOfBytesRecvd ? *lpNumberOfBytesRecvd : 0);
        ProviderOverlapped->RecvFromArgs.dwFlags                = (lpFlags ? *lpFlags : 0);
        ProviderOverlapped->RecvFromArgs.lpFrom                 = lpFrom;
        ProviderOverlapped->RecvFromArgs.lpFromLen              = lpFromLen;
        ProviderOverlapped->Provider = SocketContext->Provider;

        ret = QueueOverlappedOperation(ProviderOverlapped, SocketContext);

        if (ret != NO_ERROR)
        {
            *lpErrno = ret;
            ret = SOCKET_ERROR;
        }
    }
    else
    {
        // Make a blocking WSPRecvFrom call
        //
        SetBlockingProvider(SocketContext->Provider);
        ret = SocketContext->Provider->NextProcTable.lpWSPRecvFrom(
                SocketContext->ProviderSocket, 
                lpBuffers, 
                dwBufferCount,
                lpNumberOfBytesRecvd, 
                lpFlags, 
                lpFrom, 
                lpFromLen, 
                lpOverlapped, 
                lpCompletionRoutine, 
                lpThreadId, 
                lpErrno);
        SetBlockingProvider(NULL);
        if (ret != SOCKET_ERROR)
        {
            SocketContext->BytesRecv += *lpNumberOfBytesRecvd;
        }
    }

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPSelect
//
// Description:
//    This function tests a set of sockets for readability, writeability, and
//    exceptions. We must translate each handle in the fd_set structures to
//    their underlying provider handles before calling the next provider's
//    WSPSelect.
//
int WSPAPI WSPSelect(
    int          nfds,
    fd_set FAR * readfds,
    fd_set FAR * writefds,
    fd_set FAR * exceptfds,
    const struct timeval FAR * timeout,
    LPINT        lpErrno)
{
    SOCK_INFO *SocketContext=NULL;
    u_int      count,
               i;
    int        HandleCount,
               ret;

    struct
    {
        SOCKET ClientSocket;
        SOCKET ProvSocket;
    } Read[FD_SETSIZE], Write[FD_SETSIZE], Except[FD_SETSIZE];

    fd_set ReadFds, WriteFds, ExceptFds;

    if ( !readfds && !writefds && !exceptfds )
    {
        *lpErrno = WSAEINVAL;
        return SOCKET_ERROR;
    }
    //
    // Translate all handles contained in the fd_set structures.
    //  For each fd_set go through and build another fd_set which contains
    //  their lower provider socket handles.
    //
    if (readfds)
    {
        FD_ZERO(&ReadFds);

        if (readfds->fd_count > FD_SETSIZE)
        {
            *lpErrno = WSAENOBUFS;
            return SOCKET_ERROR;
        }
        for (i = 0; i < readfds->fd_count; i++)
        {
            SocketContext = FindAndLockSocketContext(
                    (Read[i].ClientSocket = readfds->fd_array[i]),
                    lpErrno
                    );
            if (SocketContext == NULL)
            {
                dbgprint("WSPSelect(1): WPUQuerySocketHandleContext() failed: %d", *lpErrno);
                *lpErrno = WSAENOTSOCK;
                return SOCKET_ERROR;
            }

            Read[i].ProvSocket = SocketContext->ProviderSocket;
            FD_SET(Read[i].ProvSocket, &ReadFds);

            UnlockSocketContext(SocketContext, lpErrno);
        }
    }

    if (writefds)
    {
        FD_ZERO(&WriteFds);

        if (writefds->fd_count > FD_SETSIZE)
        {
            *lpErrno = WSAENOBUFS;
            return SOCKET_ERROR;
        }
        for (i = 0; i < writefds->fd_count; i++)
        {
            SocketContext = FindAndLockSocketContext(
                    (Write[i].ClientSocket = writefds->fd_array[i]), 
                    lpErrno
                    );
            if (SocketContext == NULL)
            {
                dbgprint("WSPSelect(2): WPUQuerySocketHandleContext() failed: %d", *lpErrno);
                *lpErrno = WSAENOTSOCK;
                return SOCKET_ERROR;
            }

            Write[i].ProvSocket = SocketContext->ProviderSocket;
            FD_SET(Write[i].ProvSocket, &WriteFds);

            UnlockSocketContext(SocketContext, lpErrno);
        }
    }

    if (exceptfds)
    {
        FD_ZERO(&ExceptFds);

        if (exceptfds->fd_count > FD_SETSIZE)
        {
            *lpErrno = WSAENOBUFS;
            return SOCKET_ERROR;
        }
        for (i = 0; i < exceptfds->fd_count; i++)
        {
            SocketContext = FindAndLockSocketContext(
                    (Except[i].ClientSocket = exceptfds->fd_array[i]), 
                    lpErrno
                    );
            if (SocketContext == NULL)
            {
                dbgprint("WSPSelect(3): WPUQuerySocketHandleContext() failed: %d", *lpErrno);
                *lpErrno = WSAENOTSOCK;
                return SOCKET_ERROR;
            }

            Except[i].ProvSocket = SocketContext->ProviderSocket;
            FD_SET(Except[i].ProvSocket, &ExceptFds);

            UnlockSocketContext(SocketContext, lpErrno);
        }
    }
    //
    // Now call the lower provider's WSPSelect with the fd_set structures we built
    //  containing the lower provider's socket handles.
    //
    if (SocketContext == NULL)
    {
        *lpErrno = WSAEINVAL;
        return SOCKET_ERROR;
    }
    SetBlockingProvider(SocketContext->Provider);
    ret = SocketContext->Provider->NextProcTable.lpWSPSelect(
            nfds, 
            (readfds ? &ReadFds : NULL), 
            (writefds ? &WriteFds : NULL), 
            (exceptfds ? &ExceptFds : NULL), 
            timeout, 
            lpErrno);
    SetBlockingProvider(NULL);
    if (ret != SOCKET_ERROR)
    {
        // Once we complete we now have to go through the fd_sets we passed and
        //  map them BACK to the application socket handles. Fun!
        //
        HandleCount = ret;

        if (readfds)
        {
            count = readfds->fd_count;
            FD_ZERO(readfds);

            for(i = 0; (i < count) && HandleCount; i++)
            {
                if (MainUpCallTable.lpWPUFDIsSet(Read[i].ProvSocket, &ReadFds))
                {
                    FD_SET(Read[i].ClientSocket, readfds);
                    HandleCount--;
                }
            }
        }

        if (writefds)
        {
            count = writefds->fd_count;
            FD_ZERO(writefds);

            for(i = 0; (i < count) && HandleCount; i++)
            {
                if (MainUpCallTable.lpWPUFDIsSet(Write[i].ProvSocket, &WriteFds))
                {
                    FD_SET(Write[i].ClientSocket, writefds);
                    HandleCount--;
                }
            }
        }

        if (exceptfds)
        {
            count = exceptfds->fd_count;
            FD_ZERO(exceptfds);

            for(i = 0; (i < count) && HandleCount; i++)
            {
                if (MainUpCallTable.lpWPUFDIsSet(Except[i].ProvSocket, &ExceptFds))
                {
                    FD_SET(Except[i].ClientSocket, exceptfds);
                    HandleCount--;
                }
            }
        }
    }
    return ret;
}

//
// Function: WSPSend
//
// Description:
//    This function sends data on a given socket and also allows for asynchronous
//    (overlapped) operation. First translate the socket handle to the lower provider
//    handle and then make the send call. If called with overlap, post the operation
//    to our IOCP or completion routine.
//
int WSPAPI WSPSend(
    SOCKET          s,
    LPWSABUF        lpBuffers,
    DWORD           dwBufferCount,
    LPDWORD         lpNumberOfBytesSent,
    DWORD           dwFlags,
    LPWSAOVERLAPPED lpOverlapped,                             
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,   
    LPWSATHREADID   lpThreadId,                                 
    LPINT           lpErrno                                             
    )
{
    INT                 ret;
    SOCK_INFO          *SocketContext;
    LPWSAOVERLAPPEDPLUS ProviderOverlapped;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPSend: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }
    //
    // Check for overlapped I/O
    // 
    if (lpOverlapped)
    {
        ProviderOverlapped = GetOverlappedStructure(SocketContext);
        if (!ProviderOverlapped)
        {
            UnlockSocketContext(SocketContext, lpErrno);

            dbgprint("WSPSend: GetOverlappedStructure() returned NULL");
            *lpErrno = WSAENOBUFS;
            return SOCKET_ERROR;
        }

        ProviderOverlapped->lpCallerOverlapped = lpOverlapped;
        CopyOffset(&ProviderOverlapped->ProviderOverlapped, lpOverlapped);
        ProviderOverlapped->SockInfo           = SocketContext;
        ProviderOverlapped->CallerSocket       = s;
        ProviderOverlapped->ProviderSocket     = SocketContext->ProviderSocket;
        ProviderOverlapped->Error              = NO_ERROR;
        ProviderOverlapped->Operation          = LSP_OP_SEND;
        ProviderOverlapped->lpCallerThreadId   = lpThreadId;
        ProviderOverlapped->lpCallerCompletionRoutine      = lpCompletionRoutine;
        ProviderOverlapped->SendArgs.lpBuffers             = CopyBuffer(lpBuffers, dwBufferCount);
        ProviderOverlapped->SendArgs.dwBufferCount         = dwBufferCount;
        ProviderOverlapped->SendArgs.dwNumberOfBytesSent   = (lpNumberOfBytesSent ? *lpNumberOfBytesSent : 0);
        ProviderOverlapped->SendArgs.dwFlags               = dwFlags;
        ProviderOverlapped->Provider = SocketContext->Provider;

        ret = QueueOverlappedOperation(ProviderOverlapped, SocketContext);

        if (ret != NO_ERROR)
        {
            *lpErrno = ret;
            ret = SOCKET_ERROR;
        }
    }
    else
    {
        // Make a blocking send call
        //
        SetBlockingProvider(SocketContext->Provider);
        ret = SocketContext->Provider->NextProcTable.lpWSPSend(
                SocketContext->ProviderSocket, 
                lpBuffers, 
                dwBufferCount,
                lpNumberOfBytesSent, 
                dwFlags, 
                lpOverlapped, 
                lpCompletionRoutine, 
                lpThreadId, 
                lpErrno);
        SetBlockingProvider(NULL);
        if (ret != SOCKET_ERROR)
        {
            SocketContext->BytesSent += *lpNumberOfBytesSent;
        }
    }

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPSendDisconnect
//
// Description:
//    Send data and disconnect. All we need to do is translate the socket
//    handle to the lower provider.
//
int WSPAPI WSPSendDisconnect(
    SOCKET   s,
    LPWSABUF lpOutboundDisconnectData,
    LPINT    lpErrno)
{
    SOCK_INFO *SocketContext;
    INT        ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPSendDisconnect: WPUQuerySocketHandleContext() failed: %d", *lpErrno);

        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    SetBlockingProvider(SocketContext->Provider);
    ret = SocketContext->Provider->NextProcTable.lpWSPSendDisconnect(
            SocketContext->ProviderSocket,
            lpOutboundDisconnectData, 
            lpErrno);
    SetBlockingProvider(NULL);

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPSendTo
//
// Description:
//    This function sends data on a given socket and also allows for asynchronous
//    (overlapped) operation. First translate the socket handle to the lower provider
//    handle and then make the send call. If called with overlap, post the operation
//    to our IOCP or completion routine.
//
int WSPAPI WSPSendTo(
    SOCKET          s,
    LPWSABUF        lpBuffers,
    DWORD           dwBufferCount,
    LPDWORD         lpNumberOfBytesSent,
    DWORD           dwFlags,
    const struct sockaddr FAR * lpTo,
    int             iToLen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID   lpThreadId,
    LPINT           lpErrno)
{
    int                 ret;
    SOCK_INFO          *SocketContext;
    LPWSAOVERLAPPEDPLUS ProviderOverlapped;

    // Check for overlapped I/O
    //    
    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPSendTo: WPUQuerySocketHandleContext() failed: %d", *lpErrno);

        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }
    //
    // Check for overlapped
    //
    if (lpOverlapped)
    {
        ProviderOverlapped = GetOverlappedStructure(SocketContext);
        if (!ProviderOverlapped)
        {
            UnlockSocketContext(SocketContext, lpErrno);
            dbgprint("WSPSendto: GetOverlappedStructure() returned NULL");

            *lpErrno = WSAENOBUFS;
            return SOCKET_ERROR;
        }

        ProviderOverlapped->lpCallerOverlapped = lpOverlapped;
        CopyOffset(&ProviderOverlapped->ProviderOverlapped, lpOverlapped);
        ProviderOverlapped->SockInfo           = SocketContext;
        ProviderOverlapped->CallerSocket       = s;
        ProviderOverlapped->ProviderSocket     = SocketContext->ProviderSocket;
        ProviderOverlapped->Error              = NO_ERROR;
        ProviderOverlapped->Operation          = LSP_OP_SENDTO;
        ProviderOverlapped->lpCallerThreadId   = lpThreadId;
        ProviderOverlapped->lpCallerCompletionRoutine        = lpCompletionRoutine;
        ProviderOverlapped->SendToArgs.lpBuffers             = CopyBuffer(lpBuffers, dwBufferCount);
        ProviderOverlapped->SendToArgs.dwBufferCount         = dwBufferCount;
        ProviderOverlapped->SendToArgs.dwNumberOfBytesSent   = (lpNumberOfBytesSent ? *lpNumberOfBytesSent : 0);
        ProviderOverlapped->SendToArgs.dwFlags               = dwFlags;
        if (iToLen <= sizeof(ProviderOverlapped->SendToArgs.To))
            CopyMemory(&ProviderOverlapped->SendToArgs.To, lpTo, iToLen);
        ProviderOverlapped->SendToArgs.iToLen                = iToLen;
        ProviderOverlapped->Provider = SocketContext->Provider;

        ret = QueueOverlappedOperation(ProviderOverlapped, SocketContext);

        if (ret != NO_ERROR)
        {
            *lpErrno = ret;
            ret = SOCKET_ERROR;
        }
    }
    else
    {
        SetBlockingProvider(SocketContext->Provider);
        ret = SocketContext->Provider->NextProcTable.lpWSPSendTo(
                SocketContext->ProviderSocket, 
                lpBuffers, 
                dwBufferCount,
                lpNumberOfBytesSent, 
                dwFlags, 
                lpTo, 
                iToLen, 
                lpOverlapped, 
                lpCompletionRoutine, 
                lpThreadId, 
                lpErrno);
        SetBlockingProvider(NULL);
        if (ret != SOCKET_ERROR)
        {
            SocketContext->BytesSent += *lpNumberOfBytesSent;
        }
    }

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPSetSockOpt
//
// Description:
//    Set a socket option. For most all options we just have to translate the
//    socket option and call the lower provider. The only special case is for
//    SO_UPDATE_ACCEPT_CONTEXT in which case a socket handle is passed as the
//    argument which we need to translate before calling the lower provider.
//
int WSPAPI WSPSetSockOpt(
    SOCKET     s,
    int        level,
    int        optname,
    const char FAR * optval,   
    int        optlen,
    LPINT      lpErrno)
{
    SOCK_INFO *SocketContext,
              *AcceptContext;
    INT        ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    if (optname == SO_UPDATE_ACCEPT_CONTEXT)
    {
        // We need to intercept this (and any other options) that pass
        //  a socket handle as an argument so we can replace it with the
        //  correct underlying provider's socket handle.
        //
        AcceptContext = FindAndLockSocketContext( *((SOCKET *)optval), lpErrno);
        if (AcceptContext == NULL)
        {
            dbgprint("WSPSetSockOpt: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
            *lpErrno = WSAENOTSOCK;
            return SOCKET_ERROR;
        }

        UnlockSocketContext(AcceptContext, lpErrno);

        SetBlockingProvider(SocketContext->Provider);
        ret = SocketContext->Provider->NextProcTable.lpWSPSetSockOpt(
                    SocketContext->ProviderSocket, 
                    level,
                    optname, 
                    (char *)&AcceptContext->ProviderSocket, 
                    optlen, 
                    lpErrno);
        SetBlockingProvider(NULL);

    }
    else
    {
        SetBlockingProvider(SocketContext->Provider);
        ret = SocketContext->Provider->NextProcTable.lpWSPSetSockOpt(
                    SocketContext->ProviderSocket, 
                    level,                 
                    optname, 
                    optval, 
                    optlen, 
                    lpErrno);
        SetBlockingProvider(NULL);
    }

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPShutdown
//
// Description:
//    This function performs a shutdown on the socket. All we need to do is 
//    translate the socket handle to the lower provider.
//
int WSPAPI WSPShutdown (
    SOCKET s,
    int    how,
    LPINT  lpErrno)
{
    SOCK_INFO *SocketContext;
    INT        ret;

    SocketContext = FindAndLockSocketContext(s, lpErrno);
    if (SocketContext == NULL)
    {
        dbgprint("WSPShutdown: WPUQuerySocketHandleContext() failed: %d", *lpErrno);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    SetBlockingProvider(SocketContext->Provider);
    ret = SocketContext->Provider->NextProcTable.lpWSPShutdown(
                SocketContext->ProviderSocket, 
                how, 
                lpErrno);
    SetBlockingProvider(NULL);

    UnlockSocketContext(SocketContext, lpErrno);

    return ret;
}

//
// Function: WSPStringToAddress
//
// Description:
//    Convert a string to an address (SOCKADDR structure).  We need to translate
//    the socket handle as well as possibly substitute the lpProtocolInfo structure
//    passed to the next provider. 
//
int WSPAPI WSPStringToAddress(
    LPWSTR              AddressString,
    INT                 AddressFamily,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,   
    LPSOCKADDR          lpAddress,
    LPINT               lpAddressLength,
    LPINT               lpErrno)
{
    WSAPROTOCOL_INFOW   *pInfo=NULL;
    PROVIDER            *Provider=NULL;
    INT                  i, ret;

    for(i=0; i < gLayerCount ;i++)
    {
        if ((gBaseInfo[i].NextProvider.iAddressFamily == lpProtocolInfo->iAddressFamily) &&
            (gBaseInfo[i].NextProvider.iSocketType == lpProtocolInfo->iSocketType) && 
            (gBaseInfo[i].NextProvider.iProtocol   == lpProtocolInfo->iProtocol))
        {
            if (lpProtocolInfo)
            {
                // In case of multiple providers check the provider flags 
                if ( (gBaseInfo[i].NextProvider.dwServiceFlags1 & ~XP1_IFS_HANDLES) != 
                     (lpProtocolInfo->dwServiceFlags1 & ~XP1_IFS_HANDLES) )
                {
                    continue;
                }
            }
            Provider = &gBaseInfo[i];
            pInfo = &gBaseInfo[i].NextProvider;
            break;
        }
    }
    if (Provider == NULL)
    {
        *lpErrno = WSAEINVAL;
        return SOCKET_ERROR;
    }
    // If we're not immediately above the base then pass the lpProtocolInfo passed
    // into us.
    //
    if (pInfo->ProtocolChain.ChainLen != BASE_PROTOCOL)
    {
        pInfo = lpProtocolInfo;
    }

    SetBlockingProvider(Provider);
    ret = Provider->NextProcTable.lpWSPStringToAddress(
                AddressString, 
                AddressFamily,
                pInfo, 
                lpAddress, 
                lpAddressLength, 
                lpErrno);
    SetBlockingProvider(NULL);

    return ret;
}

//
// Function: WSPSocket
//
// Description:
//    This function creates a socket. There are two sockets created. The first
//    socket is created by calling the lower providers WSPSocket. This is the
//    handle that we use internally within our LSP. We then create a second
//    socket with WPUCreateSocketHandle which will be returned to the calling
//    application. We will also create a socket context structure which will
//    maintain information on each socket. This context is associated with the
//    socket handle passed to the application.
//
SOCKET WSPAPI WSPSocket(
    int                 af,
    int                 type,
    int                 protocol,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    GROUP               g,
    DWORD               dwFlags,
    LPINT               lpErrno)
{
    SOCKET              NextProviderSocket = INVALID_SOCKET;
    SOCKET              NewSocket = INVALID_SOCKET;
    SOCK_INFO          *SocketContext;
    WSAPROTOCOL_INFOW  *pInfo=NULL, InfoCopy;
    PROVIDER           *Provider=NULL;
    BOOL                bAddressFamilyOkay=FALSE,
                        bSockTypeOkay=FALSE,
                        bProtocolOkay=FALSE;
    INT                 iAddressFamily,
                        iSockType, 
                        iProtocol, 
                        i;
    static int entrycount=0;

    entrycount++;

    *lpErrno = NO_ERROR;
    //
    // If a WSAPROTOCOL_INFO structure was passed in, use those socket/protocol
    //  values. Then find the underlying provider's WSAPROTOCOL_INFO structure.
    //
    iAddressFamily = (lpProtocolInfo ? lpProtocolInfo->iAddressFamily : af);
    iProtocol      = (lpProtocolInfo ? lpProtocolInfo->iProtocol   : protocol);
    iSockType      = (lpProtocolInfo ? lpProtocolInfo->iSocketType : type);

    #ifdef DEBUG
    if (lpProtocolInfo)
        dbgprint("WSPSocket: [entry %d] Provider: '%S'", entrycount, lpProtocolInfo->szProtocol);
    else
        dbgprint("WSPSocket: [entry %d] Provider: NULL", entrycount);
    #endif

    for(i=0; i < gLayerCount ;i++)
    {
        if ((iAddressFamily == AF_UNSPEC) ||
            (iAddressFamily == gBaseInfo[i].NextProvider.iAddressFamily))
        {
            bAddressFamilyOkay = TRUE;
        }
        if (iSockType == gBaseInfo[i].NextProvider.iSocketType)
        {
            bSockTypeOkay = TRUE;
        }
        if ((iProtocol == 0) || (iProtocol == gBaseInfo[i].NextProvider.iProtocol) ||
            (iProtocol == IPPROTO_RAW) || (iSockType == SOCK_RAW))
        {
            bProtocolOkay = TRUE;
        }
    }
    if (!bAddressFamilyOkay)
    {
        *lpErrno = WSAEAFNOSUPPORT;
        return INVALID_SOCKET;
    }
    if (!bSockTypeOkay)
    {
        *lpErrno = WSAESOCKTNOSUPPORT;
        return INVALID_SOCKET;
    }
    if (!bProtocolOkay)
    {
        *lpErrno = WSAEPROTONOSUPPORT;
        return INVALID_SOCKET;
    }
    // If AF_UNSPEC was passed in we need to go by the socket type and protocol
    //  if possible.
    //
    if ((iAddressFamily == AF_UNSPEC) && (iProtocol == 0))
    {
        for(i=0; i < gLayerCount ;i++)
        {
            if (gBaseInfo[i].NextProvider.iSocketType == iSockType) 
            {
                if (lpProtocolInfo)
                {
                    // In case of multiple providers check the provider flags 
                    if ( (gBaseInfo[i].NextProvider.dwServiceFlags1 & ~XP1_IFS_HANDLES) != 
                         (lpProtocolInfo->dwServiceFlags1 & ~XP1_IFS_HANDLES) )
                    {
                        continue;
                    }
                }
                Provider = &gBaseInfo[i];
                pInfo = &gBaseInfo[i].NextProvider;
                if (lpProtocolInfo)
                    pInfo->dwProviderReserved = lpProtocolInfo->dwProviderReserved;
                break;
            }
        }
    }
    else if ((iAddressFamily == AF_UNSPEC) && (iProtocol != 0))
    {
        for(i=0; i < gLayerCount ;i++)
        {
            if ((gBaseInfo[i].NextProvider.iProtocol == iProtocol) &&
                (gBaseInfo[i].NextProvider.iSocketType == iSockType) )
            {
                if (lpProtocolInfo)
                {
                    // In case of multiple providers check the provider flags 
                    if ( (gBaseInfo[i].NextProvider.dwServiceFlags1 & ~XP1_IFS_HANDLES) != 
                         (lpProtocolInfo->dwServiceFlags1 & ~XP1_IFS_HANDLES) )
                    {
                        continue;
                    }
                }
                Provider = &gBaseInfo[i];
                pInfo = &gBaseInfo[i].NextProvider;
                if (lpProtocolInfo)
                    pInfo->dwProviderReserved = lpProtocolInfo->dwProviderReserved;
                break;
            }
        }
        if (pInfo == NULL)
        {
            *lpErrno = WSAEPROTOTYPE;
            return INVALID_SOCKET;
        }
    }
    else if ((iProtocol != 0) && (iProtocol != IPPROTO_RAW) && (iSockType != SOCK_RAW))
    {
        for(i=0; i < gLayerCount ;i++)
        {
            if ((gBaseInfo[i].NextProvider.iAddressFamily == iAddressFamily) &&
                (gBaseInfo[i].NextProvider.iSocketType == iSockType) &&
                (gBaseInfo[i].NextProvider.iProtocol == iProtocol))
            {
                if (lpProtocolInfo)
                {
                    // In case of multiple providers check the provider flags 
                    if ( (gBaseInfo[i].NextProvider.dwServiceFlags1 & ~XP1_IFS_HANDLES) != 
                         (lpProtocolInfo->dwServiceFlags1 & ~XP1_IFS_HANDLES) )
                    {
                        continue;
                    }
                }
                Provider = &gBaseInfo[i];
                pInfo = &gBaseInfo[i].NextProvider;
                if (lpProtocolInfo)
                    pInfo->dwProviderReserved = lpProtocolInfo->dwProviderReserved;
                break;
            }
        }
    }
    else
    {
        for(i=0; i < gLayerCount ;i++)
        {
            if ((gBaseInfo[i].NextProvider.iAddressFamily == iAddressFamily) &&
                (gBaseInfo[i].NextProvider.iSocketType == iSockType) )
            {
                if (lpProtocolInfo)
                {
                    // In case of multiple providers check the provider flags 
                    if ( (gBaseInfo[i].NextProvider.dwServiceFlags1 & ~XP1_IFS_HANDLES) != 
                         (lpProtocolInfo->dwServiceFlags1 & ~XP1_IFS_HANDLES) )
                    {
                        continue;
                    }
                }
                Provider = &gBaseInfo[i];
                pInfo = &gBaseInfo[i].NextProvider;
                if (lpProtocolInfo)
                    pInfo->dwProviderReserved = lpProtocolInfo->dwProviderReserved;
                break;
            }
        }
    }
    if (!Provider)
    {
        *lpErrno = WSAEAFNOSUPPORT;
        return INVALID_SOCKET;
    }

    if (pInfo->ProtocolChain.ChainLen != BASE_PROTOCOL)
    {
        pInfo = lpProtocolInfo;
    }

    memcpy(&InfoCopy, pInfo, sizeof(InfoCopy));

    if (lpProtocolInfo)
        InfoCopy.dwProviderReserved = lpProtocolInfo->dwProviderReserved;

    //
    // Create the underlying provider's socket.
    //

    #ifdef DEBUGSPEW
    dbgprint("Calling the lower provider WSPSocket: '%S'", Provider->NextProvider.szProtocol);
    #endif

    SetBlockingProvider(Provider);
    NextProviderSocket = Provider->NextProcTable.lpWSPSocket(
                            af, 
                            type, 
                            protocol, 
                            &InfoCopy, //pInfo,
                            g, 
                            dwFlags, 
                            lpErrno);
    SetBlockingProvider(NULL);

    if (NextProviderSocket != INVALID_SOCKET)
    {
        // Create the context informatin to be associated with this socket
        //
        if ((SocketContext = CreateSockInfo(Provider,
                                            NextProviderSocket,
                                            NULL)) == NULL)
        {
            *lpErrno = WSAENOBUFS;
        }
        else
        {
            // Create a socket handle to pass back to app
            //  
            NewSocket = MainUpCallTable.lpWPUCreateSocketHandle(
                        Provider->LayeredProvider.ProtocolChain.ChainEntries[0], 
                        (DWORD_PTR) SocketContext, 
                        lpErrno);
            if (NewSocket == INVALID_SOCKET)
            {
                dbgprint("WSPSocket: WPUCreateSocketHandle() failed: %d", *lpErrno);
                HeapFree(hLspHeap, 0, SocketContext);
            }

        dbgprint("Lower provider socket = 0x%x  LSP Socket = 0x%x\n", NextProviderSocket, NewSocket);

            SocketContext->LayeredSocket = NewSocket;

            pInfo->dwProviderReserved = 0;
        }

        return NewSocket;
    }
    else
    {
        dbgprint("WSPSocket: NextProcTable.WSPSocket() failed: %d", *lpErrno);
    }

    return INVALID_SOCKET;
}

//
// Function: WSPStartup
//
// Description:
//    This function intializes our LSP. We maintain a ref count to keep track
//    of how many times this function has been called. On the first call we'll
//    look at the Winsock catalog to find our catalog ID and find which entries
//    we are layered over. We'll create a number of structures to keep this 
//    information handy.
//
int WSPAPI WSPStartup(
    WORD                wVersion,
    LPWSPDATA           lpWSPData,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    WSPUPCALLTABLE      UpCallTable,
    LPWSPPROC_TABLE     lpProcTable)
{
    LPWSAPROTOCOL_INFOW ProtocolInfo, 
                        pInfo;
    UINT                iBaseId;
    INT                 ret = NO_ERROR,
                        TotalProtocols,
                        Error,
                        idx,
                        i, x, z;

    EnterCriticalSection(&gCriticalSection);

    MainUpCallTable = UpCallTable;

    //
    // Load Next Provider in chain if this is the first time called
    //
    if (gEntryCount == 0)
    {
        //
        // Keep track of how many times we've been loaded
        //
        gEntryCount++;

/*
        IncrementLspUsage(hDllInstance, 1);
*/

        //
        // Remap service provider functions here
        //
        gProcTable.lpWSPAccept              = WSPAccept;
        gProcTable.lpWSPAddressToString     = WSPAddressToString;
        gProcTable.lpWSPAsyncSelect         = WSPAsyncSelect;
        gProcTable.lpWSPBind                = WSPBind;
        gProcTable.lpWSPCancelBlockingCall  = WSPCancelBlockingCall;
        gProcTable.lpWSPCleanup             = WSPCleanup;
        gProcTable.lpWSPCloseSocket         = WSPCloseSocket;
        gProcTable.lpWSPConnect             = WSPConnect;
        gProcTable.lpWSPDuplicateSocket     = WSPDuplicateSocket;
        gProcTable.lpWSPEnumNetworkEvents   = WSPEnumNetworkEvents;
        gProcTable.lpWSPEventSelect         = WSPEventSelect;
        gProcTable.lpWSPGetOverlappedResult = WSPGetOverlappedResult;
        gProcTable.lpWSPGetPeerName         = WSPGetPeerName;
        gProcTable.lpWSPGetSockOpt          = WSPGetSockOpt;
        gProcTable.lpWSPGetSockName         = WSPGetSockName;
        gProcTable.lpWSPGetQOSByName        = WSPGetQOSByName;
        gProcTable.lpWSPIoctl               = WSPIoctl;
        gProcTable.lpWSPJoinLeaf            = WSPJoinLeaf;
        gProcTable.lpWSPListen              = WSPListen;
        gProcTable.lpWSPRecv                = WSPRecv;
        gProcTable.lpWSPRecvDisconnect      = WSPRecvDisconnect;
        gProcTable.lpWSPRecvFrom            = WSPRecvFrom;
        gProcTable.lpWSPSelect              = WSPSelect;
        gProcTable.lpWSPSend                = WSPSend;
        gProcTable.lpWSPSendDisconnect      = WSPSendDisconnect;
        gProcTable.lpWSPSendTo              = WSPSendTo;
        gProcTable.lpWSPSetSockOpt          = WSPSetSockOpt;
        gProcTable.lpWSPShutdown            = WSPShutdown;
        gProcTable.lpWSPSocket              = WSPSocket;
        gProcTable.lpWSPStringToAddress     = WSPStringToAddress;
        //
        // Save off the WSP data structures for use later
        //
        memset(&gWSPData, 0, sizeof(WSPDATA));
        gWSPData.wVersion = 0x202;
        gWSPData.wHighVersion = 0x202;

        //
        // Create our private heap
        //
        hLspHeap = HeapCreate(0, 128000, 0);
        if (hLspHeap == NULL)
        {
            dbgprint("WSPStartup: HeapCreate() failed: %d", GetLastError());
        }

        //
        //  Get all protocol information in database
        //
        if ((ProtocolInfo = GetProviders(&TotalProtocols)) == NULL)
        {
            LeaveCriticalSection(&gCriticalSection);

            dbgprint("GetProviders failed");

            return  WSAEPROVIDERFAILEDINIT;
        }
        // Find out what our layered protocol catalog ID entry is
        //
        for (i = 0; i < TotalProtocols; i++)
        {
            if (memcmp (&ProtocolInfo[i].ProviderId, &ProviderGuid, sizeof (GUID))==0)
            {
                gLayerCatId = ProtocolInfo[i].dwCatalogEntryId;
                break;
            }
        }
        //
        // Find out how many protocol entries we're layered over
        //
        gLayerCount=0;
        for(x=0; x < TotalProtocols ;x++)
        {
            if (gLayerCatId == ProtocolInfo[x].ProtocolChain.ChainEntries[0])
            {
                gLayerCount++;
            }
        }

        dbgprint("Layered over %d protocols", gLayerCount);

        //
        // Allocate some space to save off the WSAPROTOCOL_INFO structures of the 
        //  underlying providers. We'll need these throughout execution.
        //
        gBaseInfo = (LPPROVIDER)HeapAlloc(hLspHeap,
                                          HEAP_ZERO_MEMORY,
                                          sizeof(PROVIDER)*gLayerCount);
        if (!gBaseInfo)
        {
            LeaveCriticalSection(&gCriticalSection);

            dbgprint("WSPStartup: HeapAlloc() failed on gBaseInfo: %d", GetLastError());

            return WSAENOBUFS;
        }
        for(i=0; i < gLayerCount ;i++)
        {
            InitializeCriticalSection(&gBaseInfo[i].ProviderCritSec);
        }
        //
        // Now copy the WSAPROTOCOL_INFO structures into our storage space
        //
        idx=0;
        for(x=0; x < TotalProtocols ;x++)
        {
            if (gLayerCatId == ProtocolInfo[x].ProtocolChain.ChainEntries[0])
            {
                memcpy(&gBaseInfo[idx].LayeredProvider,
                       &ProtocolInfo[x],
                       sizeof(WSAPROTOCOL_INFOW));

                dbgprint("Layer is %S", gBaseInfo[idx].LayeredProvider.szProtocol);

                //
                // Our LSP exists in this entries chain so grab the entry
                //  after ours (i.e. the next provider in the chain)
                //
                iBaseId = ProtocolInfo[x].ProtocolChain.ChainEntries[1];
                for(z=0; z < TotalProtocols ;z++)
                {
                    if (ProtocolInfo[z].dwCatalogEntryId == iBaseId)
                    {
                        memcpy(&gBaseInfo[idx++].NextProvider, 
                               &ProtocolInfo[z], 
                                sizeof(WSAPROTOCOL_INFOW));
                        break;
                    }
                }
            }
        }

        for(x=0; x < gLayerCount ;x++)
        {
            gBaseInfo[x].ProviderPathLen = MAX_PATH;
            if (WSCGetProviderPath(&gBaseInfo[x].NextProvider.ProviderId,
                                    gBaseInfo[x].ProviderPathW,
                                   &gBaseInfo[x].ProviderPathLen,
                                   &Error) == SOCKET_ERROR)
            {
                LeaveCriticalSection(&gCriticalSection);
                return WSAEPROVIDERFAILEDINIT;
            }

            if (ExpandEnvironmentStringsW(gBaseInfo[x].ProviderPathW,
                                          gBaseInfo[x].LibraryPathW,
                                          MAX_PATH))
            {
                if ((gBaseInfo[x].hProvider = LoadLibraryW(gBaseInfo[x].LibraryPathW)) == NULL)
                {
                    LeaveCriticalSection(&gCriticalSection);
                    return WSAEPROVIDERFAILEDINIT;
                }

                dbgprint("LoadLibrary on: %S", gBaseInfo[x].LibraryPathW);
            }
            else
            {
                // We must be on Win9x
                //
                WideCharToMultiByte(
                    CP_ACP, 
                    0, 
                    gBaseInfo[x].ProviderPathW, 
                    gBaseInfo[x].ProviderPathLen,
                    gBaseInfo[x].ProviderPathA, 
                    gBaseInfo[x].ProviderPathLen, 
                    NULL, 
                    NULL);
                if ((gBaseInfo[x].hProvider = LoadLibraryA(gBaseInfo[x].ProviderPathA)) == NULL)
                {
                    LeaveCriticalSection(&gCriticalSection);
                    return  WSAEPROVIDERFAILEDINIT;
                }
            }
            //
            // Call the WSPStartup function on the lower provider's DLL
            //
            if ((gBaseInfo[x].lpWSPStartup = (LPWSPSTARTUP)
                GetProcAddress(gBaseInfo[x].hProvider, "WSPStartup")) == NULL)
            {
                LeaveCriticalSection(&gCriticalSection);
                dbgprint("GetProcAddress failed!");
                return  WSAEPROVIDERFAILEDINIT;
            }
            pInfo = lpProtocolInfo;
            if (gBaseInfo[x].NextProvider.ProtocolChain.ChainLen == BASE_PROTOCOL)
            {
                pInfo = &gBaseInfo[x].NextProvider;
            }

            ret = gBaseInfo[x].lpWSPStartup(wVersion,
                                            lpWSPData,
                                            pInfo,
                                            UpCallTable,
                                            &gBaseInfo[x].NextProcTable);
            if (ret != 0)
            {
                dbgprint("%s->WSPStartup() failed: %d", gBaseInfo[x].NextProvider.szProtocol, ret);
            }
            dbgprint("NextProcTable.WSPStartup = 0x%X [%S]", gBaseInfo[x].lpWSPStartup, gBaseInfo[x].NextProvider.szProtocol);

            if (VerifyProcTable(&gBaseInfo[x].NextProcTable) == SOCKET_ERROR)
            {
                dbgprint("Provider '%S' returned bad proc table!", &gBaseInfo[x].NextProvider.szProtocol);
                return  WSAEPROVIDERFAILEDINIT;
            }
        }
        InitOverlappedManager();

        FreeProviders(ProtocolInfo);
    }
    else
    {
        //
        // Keep track of how many times we've been loaded
        //
        gEntryCount++;
        ret = 0;
    }
    //
    // Copy the LSP's proc table to the caller's data structures
    //
    memcpy(lpWSPData, &gWSPData, sizeof(WSPDATA));
    memcpy(lpProcTable, &gProcTable, sizeof(WSPPROC_TABLE));

    dbgprint("WSPStartup: %d", gEntryCount);

    LeaveCriticalSection(&gCriticalSection);

    return ret;
}

//
// Function: CopyOffset
//
// Description:
//    Any offset information passed by the application in its OVERLAPPED structure
//    needs to be copied down to the OVERLAPPED structure the LSP passes to the
//    lower layer. This function copies the offset fields.
//
void CopyOffset(WSAOVERLAPPED *ProviderOverlapped, WSAOVERLAPPED *UserOverlapped)
{
    ProviderOverlapped->Offset     = UserOverlapped->Offset;
    ProviderOverlapped->OffsetHigh = UserOverlapped->OffsetHigh;
}

//
// Function: CopyBuffer
//
// Description:
//    Overlapped send/recv functions pass an array of WSABUF structures to specify
//    the send/recv buffers and their lengths. The Winsock spec says that providers
//    must capture all the WSABUF structures and cannot rely on them being persistent.
//    If we're on NT then we don't have to copy as we immediately call the lower
//    provider's function (and the lower provider captures the WSABUF array). However
//    if the LSP is modified to look at the buffers after the operaiton is queued, 
//    then this routine must ALWAYS copy the WSABUF array.  For Win9x since the 
//    overlapped operation doesn't immediately execute we have to copy the array.
//
WSABUF *CopyBuffer(WSABUF *BufferArray, DWORD BufferCount)
{
    WSABUF      *buffercopy=NULL;
    DWORD        i;

    if (ghIocp == NULL)
    {
        // We're on Win9x -- we need to save off the WSABUF structures
        // because on Win9x, the overlapped operation does not execute
        // immediately and the Winsock spec says apps are free to use
        // stack based WSABUF arrays.
        
        buffercopy = (WSABUF *)HeapAlloc(hLspHeap, HEAP_ZERO_MEMORY, sizeof(WSABUF) * BufferCount);
        if (buffercopy == NULL)
        {
            wsprintf(Msg, TEXT("CopyBuffer: HeapAlloc failed: %d\r\n"), GetLastError());
            return NULL;
        }
        for(i=0; i < BufferCount ;i++)
        {
            buffercopy[i].buf = BufferArray[i].buf;
            buffercopy[i].len = BufferArray[i].len;
        }
        return buffercopy;
    }
    else
    {
        // With completion ports, we post the overlapped operation
        // immediately to the lower provider which should capture
        // the WSABUF array members itself. If your LSP needs to
        // look at the buffers after the operation is initiated,
        // you'd better always copy the WSABUF array.

        return BufferArray;
    }
}

//
// Function: FreeBuffer
//
// Description:
//    Read the description for CopyBuffer first! This routine frees the allocated
//    array of WSABUF structures. Normally, the array is copied only on Win9x 
//    systems. If your LSP needs to look at the buffers after the overlapped operation
//    is issued, you must always copy the buffers (therefore you must always delete
//    them when done).
//
void FreeBuffer(WSABUF *BufferArray)
{
    if (ghIocp == NULL)
    {
        // If we're on Win9x, the WSABUF array was copied so free it up now

        HeapFree(hLspHeap, 0, BufferArray);
    }
}

//
// Function: FreeSocketsAndMemory
//
// Description:
//    Go through each provider and close all open sockets. Then call each
//    underlying provider's WSPCleanup and free the reference to its DLL.
//
void FreeSocketsAndMemory(int *lpErrno)
{
    int     ret,
            i;

    for(i=0; i < gLayerCount ;i++)
    {
        if (gBaseInfo[i].hProvider != NULL)
        {
            CloseAndFreeSocketInfo(&gBaseInfo[i]);
            gBaseInfo[i].SocketList = NULL;

            //
            // Call the WSPCleanup of the provider's were layered over.
            //
            ret = gBaseInfo[i].NextProcTable.lpWSPCleanup(lpErrno);

            DeleteCriticalSection(&gBaseInfo[i].ProviderCritSec);

            FreeLibrary(gBaseInfo[i].hProvider);
            gBaseInfo[i].hProvider = NULL;
        }
    }
    if (gBaseInfo != NULL)
    {
        HeapFree(hLspHeap, 0, gBaseInfo);
        gBaseInfo = NULL;
    }

    if (hLspHeap != NULL)
    {
        HeapDestroy(hLspHeap);
        hLspHeap = NULL;
    }
}
