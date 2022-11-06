#include <tchar.h>
#define WIN32_LEAN_AND_MEAN	
#include <windows.h>
#include <strsafe.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>

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

int _tmain()
{
    GRS_USEPRINTF();
    WORD wVer = MAKEWORD(SOCK_VERH,SOCK_VERL);
    WSADATA wd;
    int err = ::WSAStartup(wVer,&wd);
    if(0 != err)
    {
        GRS_PRINTF(_T("无法初始化Socket2系统环境，错误码为：%d！\n"),WSAGetLastError());
        return 1;
    }
    if ( LOBYTE( wd.wVersion ) != SOCK_VERH ||
        HIBYTE( wd.wVersion ) != SOCK_VERL ) 
    {
        GRS_PRINTF(_T("无法初始化%d.%d版本的Socket环境！\n"),SOCK_VERH,SOCK_VERL);
        WSACleanup( );
        return 2; 
    }
    GRS_PRINTF(_T("Winsock库初始化成功!\n\t当前系统中支持最高的Winsock版本为%d.%d\n\t当前应用加载的版本为%d.%d\n")
        ,LOBYTE(wd.wHighVersion),HIBYTE(wd.wHighVersion)
        ,LOBYTE(wd.wVersion),HIBYTE(wd.wVersion));
    //==========================================================================================================
    SOCKET skRaw = INVALID_SOCKET;

    skRaw = WSASocket(AF_INET,SOCK_RAW,IPPROTO_RAW,NULL,0,WSA_FLAG_OVERLAPPED);
    closesocket(skRaw);
    skRaw = INVALID_SOCKET;

    skRaw = WSASocket(AF_INET,SOCK_RAW,IPPROTO_IP,NULL,0,WSA_FLAG_OVERLAPPED);
    closesocket(skRaw);
    skRaw = INVALID_SOCKET;

    skRaw = WSASocket(AF_INET,SOCK_RAW,IPPROTO_UDP,NULL,0,WSA_FLAG_OVERLAPPED);
    closesocket(skRaw);
    skRaw = INVALID_SOCKET;

    skRaw = WSASocket(AF_INET,SOCK_RAW,IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED);
    closesocket(skRaw);
    skRaw = INVALID_SOCKET;

    skRaw = WSASocket(AF_INET,SOCK_RAW,IPPROTO_ICMP,NULL,0,WSA_FLAG_OVERLAPPED);
    closesocket(skRaw);
    skRaw = INVALID_SOCKET;

    skRaw = WSASocket(AF_INET,SOCK_RAW,IPPROTO_IGMP,NULL,0,WSA_FLAG_OVERLAPPED);
    closesocket(skRaw);
    skRaw = INVALID_SOCKET;

    skRaw = WSASocket(AF_INET6,SOCK_RAW,IPPROTO_ICMPV6,NULL,0,WSA_FLAG_OVERLAPPED);
    closesocket(skRaw);
    skRaw = INVALID_SOCKET;

    skRaw = WSASocket(AF_INET6,SOCK_RAW,IPPROTO_IPV6,NULL,0,WSA_FLAG_OVERLAPPED);
    closesocket(skRaw);
    skRaw = INVALID_SOCKET;
    //==========================================================================================================

    _tsystem(_T("PAUSE"));  
    ::WSACleanup();
    return 0;
}