#include <tchar.h>
#define WIN32_LEAN_AND_MEAN	
#include <windows.h>
#include <strsafe.h>
#include <Winsock2.h>

#pragma comment( lib, "Ws2_32.lib" )

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_CREALLOC(p,sz)	HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_PRINTFA(...) \
    StringCchPrintfA(pOutBufA,1024,__VA_ARGS__);\
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE),pOutBufA,lstrlenA(pOutBufA),NULL,NULL);

#define SOCK_VERH 2
#define SOCK_VERL 2

#define GRS_SERVER_PORT 8080

#define DATA_BUFSIZE 4096

void _tmain() 
{
    GRS_USEPRINTF();
    WORD wVer = MAKEWORD(SOCK_VERH,SOCK_VERL);
    WSADATA wd;
    int err = ::WSAStartup(wVer,&wd);
    if(0 != err)
    {
        GRS_PRINTF(_T("无法初始化Socket2系统环境，错误码为：%d！\n"),WSAGetLastError());
        return ;
    }
    if ( LOBYTE( wd.wVersion ) != SOCK_VERH ||
        HIBYTE( wd.wVersion ) != SOCK_VERL ) 
    {
        GRS_PRINTF(_T("无法初始化%d.%d版本的Socket环境！\n"),SOCK_VERH,SOCK_VERL);
        WSACleanup( );
        return ; 
    }
    GRS_PRINTF(_T("Winsock库初始化成功!\n\t当前系统中支持最高的Winsock版本为%d.%d\n\t当前应用加载的版本为%d.%d\n")
        ,LOBYTE(wd.wHighVersion),HIBYTE(wd.wHighVersion)
        ,LOBYTE(wd.wVersion),HIBYTE(wd.wVersion));
    //==========================================================================================================
    WSABUF DataBuf;
    char buffer[DATA_BUFSIZE] = {};
    DWORD EventTotal = 0, 
        RecvBytes = 0, 
        Flags = 0, 
        BytesTransferred = 0, 
        CallBack = 0;
    WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS] = {};
    WSAOVERLAPPED AcceptOverlapped;
    SOCKET ListenSocket, AcceptSocket;

    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    u_short port = 8080;
    char* ip;
    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_port = htons(port);

    hostent* thisHost;
    thisHost = gethostbyname("");
    ip = inet_ntoa (*(struct in_addr *)*thisHost->h_addr_list);

    service.sin_addr.s_addr = INADDR_ANY;//inet_addr(ip);

    bind(ListenSocket, (SOCKADDR *) &service, sizeof(SOCKADDR));

    listen(ListenSocket, 1);
    GRS_PRINTF(_T("Listening...\n"));

    AcceptSocket = accept(ListenSocket, NULL, NULL);
    GRS_PRINTF(_T("Client Accepted...\n"));

    EventArray[EventTotal] = WSACreateEvent();
    ZeroMemory(&AcceptOverlapped, sizeof(WSAOVERLAPPED));
    AcceptOverlapped.hEvent = EventArray[EventTotal];
    DataBuf.len = DATA_BUFSIZE;
    DataBuf.buf = buffer;
    EventTotal++;


    while (1) 
    {
        if (WSARecv(AcceptSocket, &DataBuf, 1, &RecvBytes, &Flags, &AcceptOverlapped, NULL) == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                GRS_PRINTF(_T("Error occurred at WSARecv()\n"));
            }
        }

        DWORD Index;

        Index = WSAWaitForMultipleEvents(EventTotal, EventArray, FALSE, WSA_INFINITE, FALSE);
        
        WSAResetEvent(EventArray[Index - WSA_WAIT_EVENT_0]);
        WSAGetOverlappedResult(AcceptSocket, &AcceptOverlapped, &BytesTransferred, FALSE, &Flags);

        if (BytesTransferred == 0)
        {
            GRS_PRINTF(_T("Closing Socket %d\n"), AcceptSocket);
            closesocket(AcceptSocket);
            WSACloseEvent(EventArray[Index - WSA_WAIT_EVENT_0]);
            _tsystem(_T("PAUSE"));
            return;
        }

        if (WSASend(AcceptSocket, &DataBuf, 1, &RecvBytes, Flags, &AcceptOverlapped, NULL) == SOCKET_ERROR)
        {
            GRS_PRINTF(_T("WSASend() is busted\n"));
        }

        Flags = 0;
        ZeroMemory(&AcceptOverlapped, sizeof(WSAOVERLAPPED));

        AcceptOverlapped.hEvent = EventArray[Index - WSA_WAIT_EVENT_0];

        DataBuf.len = DATA_BUFSIZE;
        DataBuf.buf = buffer;
    }

    _tsystem(_T("PAUSE"));
    if(INVALID_SOCKET != ListenSocket)
    {
        closesocket(ListenSocket);
    }    
    ::WSACleanup();
}
