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
    //在这里添加Winsock的调用
    SOCKET skClient         = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    SOCKADDR_IN sa          = {AF_INET};
    sa.sin_addr.s_addr      = inet_addr("127.0.0.1");//环回地址
    sa.sin_port             = htons(GRS_SERVER_PORT);
    WCHAR pszSendMsg[]      = L"123456Hello Internet SSSSSSSSSSSSSSSSSSSSSSSSS啊啊啊啊啊啊啊啊啊啊啊啊吧大撒旦法!";
    size_t szLen = 0;
    StringCchLength(pszSendMsg,3000,&szLen);

    WCHAR pszRecvBuf[1024]  = {};
    
    if(0 != connect(skClient,(SOCKADDR*)&sa,sizeof(SOCKADDR_IN)))
    {
        GRS_PRINTF(_T("连接服务器出错!\n"));
        goto CLEAN_UP;
    }

    send(skClient,(char*)pszSendMsg,szLen*sizeof(WCHAR),0);

    recv(skClient,(char*)pszRecvBuf,1024*sizeof(WCHAR),0);

    GRS_PRINTF(_T("ECHO:%s\n"),pszRecvBuf);
    //==========================================================================================================
CLEAN_UP:
    if(INVALID_SOCKET != skClient)
    {
        shutdown(skClient,SD_BOTH);
        closesocket(skClient);
    }    
    _tsystem(_T("PAUSE"));
    ::WSACleanup();
    return 0;
}