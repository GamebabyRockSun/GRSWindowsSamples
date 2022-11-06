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


struct MYOVERLAPPED 
{
    WSAOVERLAPPED m_wsaol;
    long          m_lNetworkEvents;//投递的操作类型(FD_READ/FD_WRITE等)
    SOCKET        m_socket;        //投递操作的SOCKET句柄
    void*         m_pBuf;          //投递操作时的缓冲
    size_t        m_szBufLen;      //缓冲长度
    DWORD		  m_dwTrasBytes;   //为WSASent和WSARecv准备的参数
    DWORD         m_dwFlags;       //为WSARecv准备的
};


void CALLBACK CompletionROUTINE(
                                IN DWORD dwError, 
                                IN DWORD cbTransferred, 
                                IN LPWSAOVERLAPPED lpOverlapped, 
                                IN DWORD dwFlags
                                );




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

    WSABUF DataBuf = {};
    BYTE buffer[DATA_BUFSIZE] = {};

    WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS] = {};
    MYOVERLAPPED AcceptOverlapped = {};
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

    DataBuf.len = DATA_BUFSIZE;
    DataBuf.buf = (CHAR*)buffer;

    //将下列重要信息通过扩展OVERLAPPED的结构带到回调过程中去
    AcceptOverlapped.m_lNetworkEvents = FD_READ;
    AcceptOverlapped.m_pBuf = buffer;
    AcceptOverlapped.m_szBufLen = DATA_BUFSIZE;
    AcceptOverlapped.m_socket = AcceptSocket;
    

    if (WSARecv(AcceptSocket, &DataBuf, 1, &AcceptOverlapped.m_dwTrasBytes
        , &AcceptOverlapped.m_dwFlags, (WSAOVERLAPPED*)&AcceptOverlapped.m_wsaol, CompletionROUTINE) == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            GRS_PRINTF(_T("Error occurred at WSARecv()\n"));
        }
    }
//..................
    //主线程进入可警告状态
    do 
    {
        GRS_PRINTF(_T("主线程进入可警告状态......\n"));
    } while (WAIT_IO_COMPLETION  == SleepEx(INFINITE,TRUE));
   
    _tsystem(_T("PAUSE"));
    if(INVALID_SOCKET != ListenSocket)
    {
        closesocket(ListenSocket);
    }    
    ::WSACleanup();
}


void CALLBACK CompletionROUTINE(DWORD dwError,DWORD cbTransferred,LPWSAOVERLAPPED lpOverlapped,DWORD dwFlags)
{
    GRS_USEPRINTF();
    //使用宏CONTAINING_RECORD得到正确的MYOVERLAPPED指针
    //关于宏CONTAINING_RECORD用法介绍详见第一部分第十四讲
    MYOVERLAPPED* pMyOl = CONTAINING_RECORD(lpOverlapped,MYOVERLAPPED,m_wsaol);

    if(0 != dwError || 0 == cbTransferred)
    {
        GRS_PRINTF(_T("操作调用失败,错误码:0x%08X\n"),dwError);
        closesocket(pMyOl->m_socket);
        return;
    }
    
    if( FD_READ == pMyOl->m_lNetworkEvents )
    {
        GRS_PRINTF(_T("接收数据成功,发送返回......\n"));
        pMyOl->m_lNetworkEvents = FD_WRITE;
        pMyOl->m_dwFlags = 0;
        WSABUF wsabuf = {cbTransferred,(CHAR*)pMyOl->m_pBuf};

        if(SOCKET_ERROR == WSASend(pMyOl->m_socket,&wsabuf,1,&pMyOl->m_dwTrasBytes,0
            ,(LPWSAOVERLAPPED)&pMyOl->m_wsaol,CompletionROUTINE))
        {
            int iErrorCode = WSAGetLastError();
            if (iErrorCode != WSA_IO_PENDING)
            {
                GRS_PRINTF(_T("Error occurred at WSASend(),Error Code:0x%08X\n"),iErrorCode);
                GRS_PRINTF(_T("发送数据失败,关闭SOCKET句柄......\n"));
                closesocket(pMyOl->m_socket);
            }
            
        }

        return;
    }

    if( FD_WRITE == pMyOl->m_lNetworkEvents )
    {
        GRS_PRINTF(_T("发送数据成功,关闭SOCKET句柄......\n"));
        closesocket(pMyOl->m_socket);
    }
}