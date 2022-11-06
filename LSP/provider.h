// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.
// 
// Module Name: provider.h
//
// Description:
//
//    This sample illustrates how to develop a layered service provider that is
//    capable of counting all bytes transmitted through a TCP/IP socket.
//
//    This file contains all datatypes and function prototypes used
//    throughout this project.
//
#ifndef _PROVIDER_H_
#define _PROVIDER_H_ 

#include <ws2spi.h>
#include <mswsock.h>

extern WSPUPCALLTABLE MainUpCallTable;

#define WM_SOCKET (WM_USER + 1)

//
// This is the socket context data that we associate with each socket
//  that is passed back to the user app. That way when another API
//  is called we can query for this context information and find out
//  the socket handle to the lower provider.
//
struct _PROVIDER;

typedef struct _SOCK_INFO
{
    SOCKET ProviderSocket;      // lower provider socket handle
    SOCKET LayeredSocket;       // app's socket handle
    DWORD  dwOutstandingAsync;  // count of outstanding async operations
    BOOL   bClosing;            // has the app closed the socket?

    volatile LONG  RefCount;

    DWORD  BytesSent;           // Byte counts
    DWORD  BytesRecv;
    HANDLE hIocp;               // associated with an IOCP?

    HWND   hWnd;                // Window (if any) associated with socket
    UINT   uMsg;                // Message for socket events

    CRITICAL_SECTION   SockCritSec;

    struct _PROVIDER  *Provider;
    struct _SOCK_INFO *prev,
                      *next;
} SOCK_INFO;

typedef struct _EXT_WSPPROC_TABLE
{
    LPFN_ACCEPTEX             lpfnAcceptEx;
    LPFN_TRANSMITFILE         lpfnTransmitFile;
    LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs;
    LPFN_TRANSMITPACKETS      lpfnTransmitPackets;
    LPFN_CONNECTEX            lpfnConnectEx;
    LPFN_DISCONNECTEX         lpfnDisconnectEx;
    LPFN_WSARECVMSG           lpfnWSARecvMsg;
} EXT_WSPPROC_TABLE;

typedef struct _PROVIDER
{
    WSAPROTOCOL_INFOW   NextProvider,           // Next provider in chain
                        LayeredProvider;        // This layered provider
    WSPPROC_TABLE       NextProcTable;          // Proc table of next provider
    EXT_WSPPROC_TABLE   NextProcTableExt;       // Proc table of next provider's extension functions

    WCHAR               ProviderPathW[MAX_PATH],
                        LibraryPathW[MAX_PATH];
    CHAR                ProviderPathA[MAX_PATH],
                        LibraryPathA[MAX_PATH];
    INT                 ProviderPathLen;
    HINSTANCE           hProvider;

    LPWSPSTARTUP        lpWSPStartup;

    SOCK_INFO          *SocketList;

    CRITICAL_SECTION    ProviderCritSec;
} PROVIDER, * LPPROVIDER;

//
// Argument list for the AcceptEx API
//
typedef struct _ACCEPTEXARGS
{
    SOCKET       sAcceptSocket,
                 sProviderAcceptSocket;
    PVOID        lpOutputBuffer;
    DWORD        dwReceiveDataLength,
                 dwLocalAddressLength,
                 dwRemoteAddressLength;
    DWORD        dwBytesReceived;
} ACCEPTEXARGS;

//
// Argument list for the TransmitFile API
//
typedef struct _TRANSMITFILEARGS
{
    HANDLE        hFile;
    DWORD         nNumberOfBytesToWrite,
                  nNumberOfBytesPerSend;
    LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers;
    DWORD         dwFlags;
} TRANSMITFILEARGS;

//
// Argument list for the ConnectEx API
//
typedef struct _CONNECTEXARGS
{
    SOCKET           s;
    SOCKADDR_STORAGE name;
    int              namelen;
    PVOID            lpSendBuffer;
    DWORD            dwSendDataLength;
    DWORD            dwBytesSent;
} CONNECTEXARGS;

//
// Argument list for TransmitPackets API
//
typedef struct _TRANSMITPACKETSARGS
{
    SOCKET          s;
    LPTRANSMIT_PACKETS_ELEMENT lpPacketArray;
    DWORD           nElementCount;
    DWORD           nSendSize;
    DWORD           dwFlags;
} TRANSMITPACKETSARGS;

//
// Argument list for DisconnectEx API
//
typedef struct _DISCONNECTEXARGS
{
    SOCKET          s;
    DWORD           dwFlags;
    DWORD           dwReserved;
} DISCONNECTEXARGS;

//
// Argument list for WSARecvMsg API
//
typedef struct _WSARECVMSGARGS
{
    SOCKET          s;
    LPWSAMSG        lpMsg;
    DWORD           dwNumberOfBytesRecvd;
} WSARECVMSGARGS;

//
// Argument list for the WSARecv API
//
typedef struct _RECVARGS
{
    LPWSABUF       lpBuffers;
    DWORD          dwBufferCount;
    DWORD          dwNumberOfBytesRecvd,
                   dwFlags;
} RECVARGS;

//
// Argument list for the WSARecvFrom API
//
typedef struct _RECVFROMARGS
{
    LPWSABUF       lpBuffers;
    DWORD          dwBufferCount;
    DWORD          dwNumberOfBytesRecvd,
                   dwFlags;
    LPSOCKADDR     lpFrom;
    LPINT          lpFromLen;
} RECVFROMARGS;

//
// Argument list for the WSASend API
//
typedef struct _SENDARGS
{
    LPWSABUF       lpBuffers;
    DWORD          dwBufferCount;
    DWORD          dwNumberOfBytesSent;
    DWORD          dwFlags;
} SENDARGS;

//
// Argument list for the WSASendTo API
//
typedef struct _SENDTOARGS
{
    LPWSABUF         lpBuffers;
    DWORD            dwBufferCount;
    DWORD            dwNumberOfBytesSent;
    DWORD            dwFlags;
    SOCKADDR_STORAGE To;
    int              iToLen;
} SENDTOARGS;

//
// Argument list for the WSAIoctl API
//
typedef struct _IOCTLARGS
{
    DWORD          dwIoControlCode;
    LPVOID         lpvInBuffer;
    DWORD          cbInBuffer;
    LPVOID         lpvOutBuffer;
    DWORD          cbOutBuffer;
    DWORD          cbBytesReturned;
} IOCTLARGS;

//
// Our OVERLAPPED structure that includes state and argument
//  information. This structure is used for all overlapped IO
//  operations. Whenever the user app requests overlapped IO
//  we intercept this and make our own call with our own 
//  structure. This way we will receive notification first
//  at which time we can perform post processesing. When we
//  are done we can then post the completion to the user app.
//
typedef struct _WSAOVERLAPPEDPLUS
{
    WSAOVERLAPPED   ProviderOverlapped;
    PROVIDER       *Provider;
    SOCK_INFO      *SockInfo;
    SOCKET          CallerSocket;
    SOCKET          ProviderSocket;
    HANDLE          Iocp;
    int             Error;

    union 
    {
        ACCEPTEXARGS        AcceptExArgs;
        TRANSMITFILEARGS    TransmitFileArgs;
        CONNECTEXARGS       ConnectExArgs;
        TRANSMITPACKETSARGS TransmitPacketsArgs;
        DISCONNECTEXARGS    DisconnectExArgs;
        WSARECVMSGARGS      WSARecvMsgArgs;
        RECVARGS            RecvArgs;
        RECVFROMARGS        RecvFromArgs;
        SENDARGS            SendArgs;
        SENDTOARGS          SendToArgs;
        IOCTLARGS           IoctlArgs;
    };

#define LSP_OP_IOCTL           1
#define LSP_OP_RECV            2
#define LSP_OP_RECVFROM        3
#define LSP_OP_SEND            4
#define LSP_OP_SENDTO          5
#define LSP_OP_TRANSMITFILE    6
#define LSP_OP_ACCEPTEX        7
#define LSP_OP_CONNECTEX       8
#define LSP_OP_DISCONNECTEX    9
#define LSP_OP_TRANSMITPACKETS 10
#define LSP_OP_WSARECVMSG      11
    int             Operation;
    LPWSATHREADID   lpCallerThreadId;
    LPWSAOVERLAPPED lpCallerOverlapped;
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCallerCompletionRoutine;
    _WSAOVERLAPPEDPLUS                *next;
} WSAOVERLAPPEDPLUS, * LPWSAOVERLAPPEDPLUS;

//
// AsyncSelect.cpp prototypes
//
HWND GetWorkerWindow();

//
// Spi.cpp prototypes
//
void CopyOffset(WSAOVERLAPPED *ProviderOverlapped, WSAOVERLAPPED *UserOverlapped);
WSABUF *CopyBuffer(WSABUF *BufferArray, DWORD BufferCount);
void FreeBuffer(WSABUF *BufferArray);
void dbgprint(char *format,...);

//
// Overlap.cpp prototypes
//
void InitOverlappedCS(void);
int  InitOverlappedManager();
int  StopOverlappedManager();
int  StopAsyncWindowManager();
int  QueueOverlappedOperation(WSAOVERLAPPEDPLUS *lpOverlapped, SOCK_INFO *Context);

//
// Provider.cpp prototypes
//
LPWSAOVERLAPPEDPLUS GetOverlappedStructure(SOCK_INFO *SocketContext);
void PutbackOverlappedStructure(WSAOVERLAPPEDPLUS *olp);

SOCK_INFO *GetCallerSocket(PROVIDER *provider, SOCKET ProviderSocket);
void       InsertSocketInfo(PROVIDER *provider, SOCK_INFO *sock);
void       RemoveSocketInfo(PROVIDER *provider, SOCK_INFO *sock);

void  CALLBACK IntermediateCompletionRoutine(DWORD dwError, DWORD cbTransferred,
                   LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);

//
// Sockinfo.cpp prototypes
//
SOCK_INFO *CreateSockInfo(PROVIDER *Provider, SOCKET ProviderSocket, SOCK_INFO *Inherit);
SOCK_INFO *FindAndLockSocketContext(SOCKET s, int *err);
void UnlockSocketContext(SOCK_INFO *context, int *err);
void AcquireSocketLock(SOCK_INFO *SockInfo);
void ReleaseSocketLock(SOCK_INFO *SockInfo);
void CloseAndFreeSocketInfo(PROVIDER *provider);

//
// Extension.cpp prototypes
//
BOOL PASCAL FAR ExtTransmitFile (
    IN SOCKET hSocket,
    IN HANDLE hFile,
    IN DWORD nNumberOfBytesToWrite,
    IN DWORD nNumberOfBytesPerSend,
    IN LPOVERLAPPED lpOverlapped,
    IN LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
    IN DWORD dwReserved);

BOOL PASCAL FAR ExtAcceptEx(
    IN SOCKET sListenSocket,
    IN SOCKET sAcceptSocket,
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPDWORD lpdwBytesReceived,
    IN LPOVERLAPPED lpOverlapped);

BOOL PASCAL FAR ExtConnectEx(
    IN SOCKET s,
    IN const struct sockaddr FAR *name,
    IN int namelen,
    IN PVOID lpSendBuffer OPTIONAL,
    IN DWORD dwSendDataLength,
    OUT LPDWORD lpdwBytesSent,
    IN LPOVERLAPPED lpOverlapped);

BOOL PASCAL FAR ExtTransmitPackets(
    SOCKET hSocket,
    LPTRANSMIT_PACKETS_ELEMENT lpPacketArray,
    DWORD nElementCount,
    DWORD nSendSize,
    LPOVERLAPPED lpOverlapped,
    DWORD dwFlags);

BOOL PASCAL FAR ExtDisconnectEx(
    IN SOCKET s,
    IN LPOVERLAPPED lpOverlapped,
    IN DWORD  dwFlags,
    IN DWORD  dwReserved);

INT PASCAL FAR ExtWSARecvMsg(
    IN SOCKET s,
    IN OUT LPWSAMSG lpMsg,
    OUT LPDWORD lpdwNumberOfBytesRecvd,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

#endif
