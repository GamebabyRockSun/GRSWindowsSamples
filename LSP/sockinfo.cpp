// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.
//
// Module Name: provider.cpp
//
// Description:
//
//    This sample illustrates how to develop a layered service provider that is
//    capable of counting all bytes transmitted through a TCP/IP socket.
//
//    This file contains support functions that are common to the lsp and
//    the instlsp sample for enumerating the Winsock catalog of service
//    providers.
//    

#include "provider.h"

//
// External variable definitions
//
extern CRITICAL_SECTION gCriticalSection;
extern INT              gLayerCount;
extern PROVIDER        *gBaseInfo;
extern HANDLE           hLspHeap;

static char Msg[512];

SOCK_INFO *FindAndLockSocketContext(SOCKET s, int *lpErrno)
{
    SOCK_INFO *SocketContext=NULL;
    int        ret;

    EnterCriticalSection(&gCriticalSection);

    ret = MainUpCallTable.lpWPUQuerySocketHandleContext(
            s,
            (PDWORD_PTR) &SocketContext,
            lpErrno
            );
    if (ret == SOCKET_ERROR)
    {
        dbgprint("FindAndLockSocketContext: WPUQuerySocketHandleContext failed: %d", *lpErrno);
    }
    else
    {
        InterlockedIncrement(&SocketContext->RefCount);
    }
    LeaveCriticalSection(&gCriticalSection);

    return SocketContext;
}

void UnlockSocketContext(SOCK_INFO *context, int *lpErrno)
{
    LONG    newval;

    EnterCriticalSection(&gCriticalSection);

    newval = InterlockedDecrement(&context->RefCount);
    if ((newval == 0) && (context->bClosing))
    {
        if (MainUpCallTable.lpWPUCloseSocketHandle(context->LayeredSocket, lpErrno) == SOCKET_ERROR)
        {
            dbgprint("UnlockSocketContext: WPUCloseSocketHandle() failed: %d", *lpErrno);
        }

        context->LayeredSocket = INVALID_SOCKET;

        RemoveSocketInfo(context->Provider, context);

        dbgprint("Closing socket %d Bytes Sent [%lu] Bytes Recv [%lu]", 
                context->LayeredSocket, context->BytesSent, context->BytesRecv);

        ReleaseSocketLock(context);

        DeleteCriticalSection(&context->SockCritSec);
        dbgprint("Freeing a SOCK_INFO (SocketContext) structure");
        HeapFree(hLspHeap, 0, context);
    }

    LeaveCriticalSection(&gCriticalSection);
}

void AcquireSocketLock(SOCK_INFO *SockInfo)
{
    EnterCriticalSection(&SockInfo->SockCritSec);
}

void ReleaseSocketLock(SOCK_INFO *SockInfo)
{
    LeaveCriticalSection(&SockInfo->SockCritSec);
}

SOCK_INFO *CreateSockInfo(PROVIDER *Provider, SOCKET ProviderSocket, SOCK_INFO *Inherit)
{
    SOCK_INFO   *NewInfo=NULL;

    NewInfo = (SOCK_INFO *)HeapAlloc(hLspHeap,
                                     HEAP_ZERO_MEMORY,
                                     sizeof(SOCK_INFO));
    if (NewInfo == NULL)
    {
        dbgprint("HeapAlloc() failed: %d", GetLastError());
        return NULL;
    }
    NewInfo->ProviderSocket     = ProviderSocket;
    NewInfo->bClosing           = FALSE;
    NewInfo->dwOutstandingAsync = 0;
    NewInfo->BytesRecv          = 0;
    NewInfo->BytesSent          = 0;
    NewInfo->Provider           = Provider;
    NewInfo->hWnd               = (Inherit ? Inherit->hWnd : 0);
    NewInfo->uMsg               = (Inherit ? Inherit->uMsg : 0);

    InitializeCriticalSection(&NewInfo->SockCritSec);

    InsertSocketInfo(Provider, NewInfo);

    return NewInfo;
}

//
// Function: InsertSocketInfo
//
// Description:
//    We keep track of all the sockets created for a particulare provider.
//    This routine inserts a newly created socket (and its SOCK_INFO) into
//    the list.
//
void InsertSocketInfo(PROVIDER *provider, SOCK_INFO *sock)
{
    EnterCriticalSection(&provider->ProviderCritSec);

    if (!provider || !sock)
    {
        dbgprint("InsertSocketInfo: PROVIDER or SOCK_INFO == NULL!");
        LeaveCriticalSection(&provider->ProviderCritSec);
        return;
    }

    sock->next = sock->prev = NULL;

    sock->next = provider->SocketList;
    if (provider->SocketList)
    {
        provider->SocketList->prev = sock;
    }
    provider->SocketList = sock;

    LeaveCriticalSection(&provider->ProviderCritSec);
}

// 
// Function: RemoveSocketInfo
//
// Description:
//    This function removes a given SOCK_INFO structure from the referenced
//    provider. It doesn't free the structure, it just removes it from the 
//    list.
//
void RemoveSocketInfo(PROVIDER *provider, SOCK_INFO *si)
{
    EnterCriticalSection(&provider->ProviderCritSec);
    if (si->prev == NULL)
    {
        provider->SocketList = si->next;
        if (provider->SocketList)
            provider->SocketList->prev = NULL;
    }
    else
    {
        si->prev->next = si->next;
        if (si->next)
            si->next->prev = si->prev;
    }

    LeaveCriticalSection(&provider->ProviderCritSec);
}

void CloseAndFreeSocketInfo(PROVIDER *provider)
{
    SOCK_INFO    *si=NULL,
                 *siprev=NULL;
    struct linger linger;
    int           Error, ret;

    linger.l_onoff = 1;
    linger.l_linger = 0;

    si = provider->SocketList;
    while (si)
    {
        ret = provider->NextProcTable.lpWSPSetSockOpt(
            si->ProviderSocket,
            SOL_SOCKET,
            SO_LINGER,
            (char *)&linger,
            sizeof(linger),
            &Error);
        if (ret == SOCKET_ERROR)
        {
            dbgprint("WSPSetSockOpt(SO_LINGER) failed: %d", Error);
        }
        else
        {
            dbgprint("SO_LINGER set to abortive on socket: %d", si->ProviderSocket);
        }

        ret = provider->NextProcTable.lpWSPCloseSocket(
            si->ProviderSocket,
            &Error);
        if (ret == SOCKET_ERROR)
        {
            dbgprint("WSPCloseSocket() on handle %d failed: %d", si->ProviderSocket, Error);
        }
        else
        {
            dbgprint("Successfully closed socket %d", si->ProviderSocket);
        }

        MainUpCallTable.lpWPUCloseSocketHandle(
            si->LayeredSocket, 
            &Error);

        siprev = si;
        si = si->next;

        HeapFree(hLspHeap, 0, siprev);
    }
    return;
}

//
// Function: GetCallerSocket
//
// Description:
//    This function returns the SOCK_INFO structure for the given
//    provider socket. If provider is NULL then we'll search all
//    providers for the socket info.
//
SOCK_INFO *GetCallerSocket(PROVIDER *provider, SOCKET ProviderSock)
{
    SOCK_INFO *si;

    EnterCriticalSection(&gCriticalSection);

    if (provider)
    {
        si = provider->SocketList;
        while(si)
        {
            if (si->ProviderSocket == ProviderSock)
            {
                LeaveCriticalSection(&gCriticalSection);
                return si;
            }
            si = si->next;
        }
    }
    else
    {
        for(INT i=0; i < gLayerCount ;i++)
        {
            si = gBaseInfo[i].SocketList;
            while (si)
            {
                if (si->ProviderSocket == ProviderSock)
                {
                    LeaveCriticalSection(&gCriticalSection);
                    return si;
                }
                si = si->next;
            }
        }
    }

    LeaveCriticalSection(&gCriticalSection);
    return NULL;
}
