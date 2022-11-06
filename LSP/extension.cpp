// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1998  Microsoft Corporation.  All Rights Reserved.
//
// Module Name: extension.cpp
//
// Description:
//
//    This sample illustrates how to develop a layered service provider that is
//    capable of counting all bytes transmitted through a TCP/IP socket.
//
//    This file contains all of the Winsock extension functions that can
//    be monitored by a service provider. This is done by intercepting any
//    WSAIoctl calls with the SIO_GET_EXTENSION_FUNCTION_POINTER (see spi.cpp
//    and WSPIoctl for more info). We substitute our own function pointer so
//    that an application calls into us. Currently we intercept only TransmitFile
//    and AcceptEx.
//

#include "provider.h"

//
// Used to output debug messages
//
static TCHAR Msg[512];

//
// Function: ExtTransmitFile
//
// Description:
//    This is our provider's TransmitFile function. When an app calls WSAIoctl
//    to request the function pointer to TransmitFile, we intercept the call
//    and return a pointer to our extension function instead.
//
BOOL PASCAL FAR ExtTransmitFile (
    IN SOCKET hSocket,
    IN HANDLE hFile,
    IN DWORD nNumberOfBytesToWrite,
    IN DWORD nNumberOfBytesPerSend,
    IN LPOVERLAPPED lpOverlapped,
    IN LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
    IN DWORD dwFlags)
{
	SOCK_INFO          *SocketContext;
	LPWSAOVERLAPPEDPLUS ProviderOverlapped;
	int                 Errno,
                        ret;

    SocketContext = FindAndLockSocketContext(hSocket, &Errno);
    if (SocketContext == NULL)
    {
        dbgprint("ExtTransmitFile: WPUQuerySocketHandleContext() failed: %d", Errno);
        WSASetLastError(Errno);
		return FALSE;
    }

    if (!SocketContext->Provider->NextProcTableExt.lpfnTransmitFile)
    {
        UnlockSocketContext(SocketContext, &Errno);
        dbgprint("Next proc table TransmitFile == NULL!");
        WSASetLastError(WSAEFAULT);
	    return FALSE;
    }

	// Check for overlapped I/O
	
	if (lpOverlapped)
	{
		ProviderOverlapped = GetOverlappedStructure(SocketContext);
        if (!ProviderOverlapped)
        {
            UnlockSocketContext(SocketContext, &Errno);
            dbgprint("ExtTransmitFile: GetOverlappedStructure() returned NULL!");
            WSASetLastError(WSAENOBUFS);
            return FALSE;
        }
        //
        // Save off the arguments and setup the overlapped structure
        //
        ProviderOverlapped->lpCallerOverlapped = lpOverlapped;
        CopyOffset(&ProviderOverlapped->ProviderOverlapped, lpOverlapped);
        ProviderOverlapped->SockInfo           = SocketContext;
        ProviderOverlapped->CallerSocket       = hSocket;
        ProviderOverlapped->ProviderSocket     = SocketContext->ProviderSocket;
        ProviderOverlapped->Error              = NO_ERROR;
        ProviderOverlapped->Operation          = LSP_OP_TRANSMITFILE;
        ProviderOverlapped->lpCallerThreadId   = NULL;
        ProviderOverlapped->lpCallerCompletionRoutine              = NULL;
        ProviderOverlapped->TransmitFileArgs.hFile                 = hFile;
        ProviderOverlapped->TransmitFileArgs.nNumberOfBytesToWrite = nNumberOfBytesToWrite;
        ProviderOverlapped->TransmitFileArgs.nNumberOfBytesPerSend = nNumberOfBytesPerSend;
        ProviderOverlapped->TransmitFileArgs.lpTransmitBuffers     = lpTransmitBuffers;
        ProviderOverlapped->TransmitFileArgs.dwFlags               = dwFlags;
        ProviderOverlapped->Provider = SocketContext->Provider;

        ret = QueueOverlappedOperation(ProviderOverlapped, SocketContext);

        if (ret != NO_ERROR)
        {
            WSASetLastError(ret);
            ret = FALSE;
        }
        else
        {
            ret = TRUE;
        }
	}
	else
	{
		ret = SocketContext->Provider->NextProcTableExt.lpfnTransmitFile(
			SocketContext->ProviderSocket,
			hFile,
			nNumberOfBytesToWrite,
			nNumberOfBytesPerSend,
			NULL,
			lpTransmitBuffers,
			dwFlags);
	}

    UnlockSocketContext(SocketContext, &Errno);

    return ret;
}

//
// Function: ExtAcceptEx
//
// Description:
//    This is our provider's AcceptEx function. When an app calls WSAIoctl
//    to request the function pointer to AcceptEx, we intercept the call
//    and return a pointer to our extension function instead.
//
BOOL PASCAL FAR ExtAcceptEx(
	IN SOCKET sListenSocket,
	IN SOCKET sAcceptSocket,
	IN PVOID lpOutputBuffer,
	IN DWORD dwReceiveDataLength,
	IN DWORD dwLocalAddressLength,
	IN DWORD dwRemoteAddressLength,
	OUT LPDWORD lpdwBytesReceived,
	IN LPOVERLAPPED lpOverlapped)
{
	LPWSAOVERLAPPEDPLUS ProviderOverlapped;
	SOCK_INFO          *ListenSocketContext=NULL,
	                   *AcceptSocketContext=NULL;
	int                 Errno,
                        ret;


    //
    // Query the socket context for the listening socket
    //
    ListenSocketContext = FindAndLockSocketContext(sListenSocket, &Errno);
    if (ListenSocketContext == NULL)
    {
        dbgprint("AcceptExExt: WPUQuerySocketHandleContext "
                  "on listen socket failed: %d", Errno);
        Errno = WSAENOTSOCK;
        WSASetLastError(Errno);
		return FALSE;
    }
    //
    // Also need to query the socket context for the accept socket
    //
    AcceptSocketContext = FindAndLockSocketContext(sAcceptSocket, &Errno);
    if (AcceptSocketContext == NULL)
    {
        UnlockSocketContext(ListenSocketContext, &Errno);

        dbgprint("AcceptExExt: WPUQuerySocketHandleContext "
                  "on accept socket failed: %d", Errno);
        Errno = WSAENOTSOCK;
        WSASetLastError(Errno);
		return FALSE;
    }

    if (!ListenSocketContext->Provider->NextProcTableExt.lpfnAcceptEx)
    {
        UnlockSocketContext(ListenSocketContext, &Errno);
        UnlockSocketContext(AcceptSocketContext, &Errno);

        dbgprint("Lower provider AcceptEx == NULL!");
        WSASetLastError(WSAEFAULT);
		return FALSE;
    }

	// Check for overlapped I/O

	if (lpOverlapped)
	{
		ProviderOverlapped = GetOverlappedStructure(ListenSocketContext);
        if (!ProviderOverlapped)
        {
            UnlockSocketContext(ListenSocketContext, &Errno);
            UnlockSocketContext(AcceptSocketContext, &Errno);

            dbgprint("ExtAcceptEx: GetOverlappedStructre() returne NULL!");
            WSASetLastError(WSAENOBUFS);
            return FALSE;
        }
        // Save off the paramters and initalize the overlapped structure
        //
        ProviderOverlapped->lpCallerOverlapped = lpOverlapped;
        CopyOffset(&ProviderOverlapped->ProviderOverlapped, lpOverlapped);
        ProviderOverlapped->SockInfo           = ListenSocketContext;
        ProviderOverlapped->CallerSocket       = sListenSocket;
        ProviderOverlapped->ProviderSocket     = ListenSocketContext->ProviderSocket;
        ProviderOverlapped->Error              = NO_ERROR;
        ProviderOverlapped->Operation          = LSP_OP_ACCEPTEX;
        ProviderOverlapped->lpCallerThreadId   = NULL;
        ProviderOverlapped->lpCallerCompletionRoutine          = NULL;
        ProviderOverlapped->AcceptExArgs.sAcceptSocket         = sAcceptSocket;
        ProviderOverlapped->AcceptExArgs.sProviderAcceptSocket = AcceptSocketContext->ProviderSocket;
        ProviderOverlapped->AcceptExArgs.lpOutputBuffer        = lpOutputBuffer;
        ProviderOverlapped->AcceptExArgs.dwReceiveDataLength   = dwReceiveDataLength;
        ProviderOverlapped->AcceptExArgs.dwLocalAddressLength  = dwLocalAddressLength;
        ProviderOverlapped->AcceptExArgs.dwRemoteAddressLength = dwRemoteAddressLength;
        ProviderOverlapped->AcceptExArgs.dwBytesReceived       = (lpdwBytesReceived ? *lpdwBytesReceived : 0);
        ProviderOverlapped->Provider = AcceptSocketContext->Provider;

        ret = QueueOverlappedOperation(ProviderOverlapped, ListenSocketContext);

        if (ret != NO_ERROR)
        {
            WSASetLastError(ret);
            ret = FALSE;
        }
        else
        {
            ret = TRUE;
        }
	}
	else
	{
		ret = ListenSocketContext->Provider->NextProcTableExt.lpfnAcceptEx(
			ListenSocketContext->ProviderSocket,
			AcceptSocketContext->ProviderSocket,
			lpOutputBuffer,
			dwReceiveDataLength,
			dwLocalAddressLength,
			dwRemoteAddressLength,
			lpdwBytesReceived,
			NULL);
	}

    UnlockSocketContext(ListenSocketContext, &Errno);
    UnlockSocketContext(AcceptSocketContext, &Errno);

    return ret;
}

BOOL PASCAL FAR ExtConnectEx(
    IN SOCKET s,
    IN const struct sockaddr FAR *name,
    IN int namelen,
    IN PVOID lpSendBuffer OPTIONAL,
    IN DWORD dwSendDataLength,
    OUT LPDWORD lpdwBytesSent,
    IN LPOVERLAPPED lpOverlapped)
{
    SOCK_INFO           *SocketContext=NULL;
    LPWSAOVERLAPPEDPLUS  ProviderOverlapped=NULL;
    int                  Errno,
                         ret;

    SocketContext = FindAndLockSocketContext(s, &Errno);
    if (SocketContext == NULL)
    {
        dbgprint("ExtConnectEx: WPUQuerySocketHandleContext() failed: %d", Errno);
        return FALSE;
    }

    if (!SocketContext->Provider->NextProcTableExt.lpfnConnectEx)
    {
        UnlockSocketContext(SocketContext, &Errno);

        dbgprint("Next proc table ConnectEx == NULL!");
        WSASetLastError(WSAEFAULT);
        return FALSE;
    }

    // Check for overlapped I/O

    if (lpOverlapped)
    {
        ProviderOverlapped = GetOverlappedStructure(SocketContext);
        if (!ProviderOverlapped)
        {
            UnlockSocketContext(SocketContext, &Errno);

            dbgprint("ExtConnectEx: GetOverlappedStructure() returned NULL");
            WSASetLastError(WSAENOBUFS);
            return FALSE;
        }
        //
        // Save off the arguments and setup the overlapped structure
        //
        ProviderOverlapped->lpCallerOverlapped = lpOverlapped;
        CopyOffset(&ProviderOverlapped->ProviderOverlapped, lpOverlapped);
        ProviderOverlapped->SockInfo           = SocketContext;
        ProviderOverlapped->CallerSocket       = s;
        ProviderOverlapped->ProviderSocket     = SocketContext->ProviderSocket;
        ProviderOverlapped->Error              = NO_ERROR;
        ProviderOverlapped->Operation          = LSP_OP_CONNECTEX;
        ProviderOverlapped->lpCallerThreadId   = NULL;
        ProviderOverlapped->lpCallerCompletionRoutine      = NULL;
        ProviderOverlapped->ConnectExArgs.s                = s;
        if (namelen <= sizeof(ProviderOverlapped->ConnectExArgs.name))
            CopyMemory(&ProviderOverlapped->ConnectExArgs.name, name, namelen);
        ProviderOverlapped->ConnectExArgs.namelen          = namelen;
        ProviderOverlapped->ConnectExArgs.lpSendBuffer     = lpSendBuffer;
        ProviderOverlapped->ConnectExArgs.dwSendDataLength = dwSendDataLength;
        ProviderOverlapped->ConnectExArgs.dwBytesSent      = (lpdwBytesSent ? *lpdwBytesSent : 0);
        ProviderOverlapped->Provider           = SocketContext->Provider;

        ret = QueueOverlappedOperation(ProviderOverlapped, SocketContext);

        if (ret != NO_ERROR)
        {
            WSASetLastError(ret);
            ret = FALSE;
        }
        else
        {
            ret = TRUE;
        }
    }
    else
    {
        ret = SocketContext->Provider->NextProcTableExt.lpfnConnectEx(
            SocketContext->ProviderSocket,
            name,
            namelen,
            lpSendBuffer,
            dwSendDataLength,
            lpdwBytesSent,
            NULL);
    }
    UnlockSocketContext(SocketContext, &Errno);

    return ret;
}

BOOL PASCAL FAR ExtTransmitPackets(
    SOCKET hSocket,
    LPTRANSMIT_PACKETS_ELEMENT lpPacketArray,
    DWORD nElementCount,
    DWORD nSendSize,
    LPOVERLAPPED lpOverlapped,
    DWORD dwFlags)
{
    SOCK_INFO           *SocketContext=NULL;
    LPWSAOVERLAPPEDPLUS  ProviderOverlapped=NULL;
    int                  Errno,
                         ret;

    SocketContext = FindAndLockSocketContext(hSocket, &Errno);
    if (SocketContext == NULL)
    {
        dbgprint("ExtTransmitPackets: WPUQuerySocketHandleContext() failed: %d", Errno);
        WSASetLastError(Errno);
        return FALSE;
    }

    if (!SocketContext->Provider->NextProcTableExt.lpfnTransmitPackets)
    {
        UnlockSocketContext(SocketContext, &Errno);
        dbgprint("Next proc table TransmitPackets == NULL!");
        WSASetLastError(WSAEFAULT);
        return FALSE;
    }
    //
    // Check for overlapped I/O
    //
    if (lpOverlapped)
    {
        ProviderOverlapped = GetOverlappedStructure(SocketContext);
        if (!ProviderOverlapped)
        {
            UnlockSocketContext(SocketContext, &Errno);
            dbgprint("ExtTransmitPackets: GetOverlappedStructure() returned NULL");
            WSASetLastError(WSAENOBUFS);
            return FALSE;
        }
        //
        // Save off the arguments and setup the overlapped structure
        //
        ProviderOverlapped->lpCallerOverlapped = lpOverlapped;
        CopyOffset(&ProviderOverlapped->ProviderOverlapped, lpOverlapped);
        ProviderOverlapped->SockInfo           = SocketContext;
        ProviderOverlapped->CallerSocket       = hSocket;
        ProviderOverlapped->ProviderSocket     = SocketContext->ProviderSocket;
        ProviderOverlapped->Error              = NO_ERROR;
        ProviderOverlapped->Operation          = LSP_OP_TRANSMITPACKETS;
        ProviderOverlapped->lpCallerThreadId   = NULL;
        ProviderOverlapped->lpCallerCompletionRoutine           = NULL;
        ProviderOverlapped->TransmitPacketsArgs.s               = hSocket;
        ProviderOverlapped->TransmitPacketsArgs.lpPacketArray   = lpPacketArray;
        ProviderOverlapped->TransmitPacketsArgs.nElementCount   = nElementCount;
        ProviderOverlapped->TransmitPacketsArgs.nSendSize       = nSendSize;
        ProviderOverlapped->TransmitPacketsArgs.dwFlags         = dwFlags;
        ProviderOverlapped->Provider           = SocketContext->Provider;

        ret = QueueOverlappedOperation(ProviderOverlapped, SocketContext);

        if (ret != NO_ERROR)
        {
            WSASetLastError(ret);
            ret = FALSE;
        }
        else
        {
            ret = TRUE;
        }

    }
    else
    {
        ret = SocketContext->Provider->NextProcTableExt.lpfnTransmitPackets(
            SocketContext->ProviderSocket,
            lpPacketArray,
            nElementCount,
            nSendSize,
            NULL,
            dwFlags);
    }
    UnlockSocketContext(SocketContext, &Errno);

    return ret;
}

BOOL PASCAL FAR ExtDisconnectEx(
    IN SOCKET s,
    IN LPOVERLAPPED lpOverlapped,
    IN DWORD  dwFlags,
    IN DWORD  dwReserved)
{
    SOCK_INFO           *SocketContext=NULL;
    LPWSAOVERLAPPEDPLUS  ProviderOverlapped=NULL;
    int                  Errno,
                         ret;

    SocketContext = FindAndLockSocketContext(s, &Errno);
    if (SocketContext == NULL)
    {
        dbgprint("ExtDisconnectEx: WPUQuerySocketHandleContext() failed: %d", Errno);
        WSASetLastError(Errno);
        return FALSE;
    }

    if (!SocketContext->Provider->NextProcTableExt.lpfnDisconnectEx)
    {
        UnlockSocketContext(SocketContext, &Errno);
        dbgprint("Next proc table DisconnectEx == NULL!");
        WSASetLastError(WSAEFAULT);
        return FALSE;
    }

    // Check for overlapped I/O

    if (lpOverlapped)
    {
        ProviderOverlapped = GetOverlappedStructure(SocketContext);
        if (!ProviderOverlapped)
        {
            UnlockSocketContext(SocketContext, &Errno);
            dbgprint("ExtDisconnectEx: GetOverlappedStructure() returned NULL");
            WSASetLastError(WSAENOBUFS);
            return FALSE;
        }
        //
        // Save off the arguments and setup the overlapped structure
        //
        ProviderOverlapped->lpCallerOverlapped = lpOverlapped;
        CopyOffset(&ProviderOverlapped->ProviderOverlapped, lpOverlapped);
        ProviderOverlapped->SockInfo           = SocketContext;
        ProviderOverlapped->CallerSocket       = s;
        ProviderOverlapped->ProviderSocket     = SocketContext->ProviderSocket;
        ProviderOverlapped->Error              = NO_ERROR;
        ProviderOverlapped->Operation          = LSP_OP_DISCONNECTEX;
        ProviderOverlapped->lpCallerThreadId   = NULL;
        ProviderOverlapped->lpCallerCompletionRoutine           = NULL;
        ProviderOverlapped->DisconnectExArgs.s                  = s;
        ProviderOverlapped->DisconnectExArgs.dwFlags            = dwFlags;
        ProviderOverlapped->DisconnectExArgs.dwReserved         = dwReserved;
        ProviderOverlapped->Provider           = SocketContext->Provider;
 
        ret = QueueOverlappedOperation(ProviderOverlapped, SocketContext);

        if (ret != NO_ERROR)
        {
            WSASetLastError(ret);
            ret = FALSE;
        }
        else
        {
            ret = TRUE;
        }
    }
    else
    {
        ret = SocketContext->Provider->NextProcTableExt.lpfnDisconnectEx(
            SocketContext->ProviderSocket,
            lpOverlapped,
            dwFlags,
            dwReserved);
    }
    UnlockSocketContext(SocketContext, &Errno);

    return ret;
}

INT PASCAL FAR ExtWSARecvMsg(
    IN SOCKET s,
    IN OUT LPWSAMSG lpMsg,
    OUT LPDWORD lpdwNumberOfBytesRecvd,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    SOCK_INFO           *SocketContext=NULL;
    LPWSAOVERLAPPEDPLUS  ProviderOverlapped=NULL;
    int                  Errno,
                         ret;

    SocketContext = FindAndLockSocketContext(s, &Errno);
    if (SocketContext == NULL)
    {
        dbgprint("ExtWSARecvmsg: WPUQuerySocketHandleContext() failed: %d", Errno);
        WSASetLastError(Errno);
        return FALSE;
    }

    if (!SocketContext->Provider->NextProcTableExt.lpfnWSARecvMsg)
    {
        UnlockSocketContext(SocketContext, &Errno);
        dbgprint("Next proc table WSARecvMsg == NULL!");
        WSASetLastError(WSAEFAULT);
        return FALSE;
    }
    //
    // Check for overlapped I/O
    //
    if (lpOverlapped)
    {
        ProviderOverlapped = GetOverlappedStructure(SocketContext);
        if (!ProviderOverlapped)
        {
            UnlockSocketContext(SocketContext, &Errno);
            dbgprint("ExtWSARecvMsg: GetOverlappedStructure() returned NULL");
            WSASetLastError(WSAENOBUFS);
            return FALSE;
        }
        //
        // Save off the arguments and setup the overlapped structure
        //
        ProviderOverlapped->lpCallerOverlapped = lpOverlapped;
        CopyOffset(&ProviderOverlapped->ProviderOverlapped, lpOverlapped);
        ProviderOverlapped->SockInfo           = SocketContext;
        ProviderOverlapped->CallerSocket       = s;
        ProviderOverlapped->ProviderSocket     = SocketContext->ProviderSocket;
        ProviderOverlapped->Error              = NO_ERROR;
        ProviderOverlapped->Operation          = LSP_OP_WSARECVMSG;
        ProviderOverlapped->lpCallerThreadId   = NULL;
        ProviderOverlapped->lpCallerCompletionRoutine           = lpCompletionRoutine;
        ProviderOverlapped->WSARecvMsgArgs.s                    = s;
        ProviderOverlapped->WSARecvMsgArgs.lpMsg                = lpMsg;
        ProviderOverlapped->WSARecvMsgArgs.dwNumberOfBytesRecvd = (lpdwNumberOfBytesRecvd ? *lpdwNumberOfBytesRecvd : 0);
        ProviderOverlapped->Provider           = SocketContext->Provider;

        ret = QueueOverlappedOperation(ProviderOverlapped, SocketContext);

        if (ret != NO_ERROR)
        {
            WSASetLastError(ret);
            ret = FALSE;
        }
        else
        {
            ret = TRUE;
        }
    }
    else
    {
        ret = SocketContext->Provider->NextProcTableExt.lpfnWSARecvMsg(
            SocketContext->ProviderSocket,
            lpMsg,
            lpdwNumberOfBytesRecvd,
            NULL,
            NULL);
    }

    UnlockSocketContext(SocketContext, &Errno);

    return ret;
}
