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

#define GRS_SERVER_PORT 8080
#define GRS_MCAST_IP    "224.0.0.99"

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
    SOCKET skServer         = socket(AF_INET,SOCK_DGRAM,0);
    SOCKADDR_IN saClient    = {AF_INET};
    SOCKADDR_IN saLocal     = {AF_INET};
    int         isaLen      = sizeof(SOCKADDR_IN);
    VOID*       pskBuf      = NULL;
    VOID*       pskBufTmp   = NULL;
    int         iBufLen     = 1024;
    int         iBufLenTmp  = 0;
    int         iRecv       = 0;
    ip_mreq     ipMCast     = {};
    DWORD       dwTTL       = 8;
    BOOL        bLoopBack   = FALSE;
    const int on = 1; //允许程序的多个实例运行在同一台机器上

    setsockopt(skServer, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));


    //准备本地地址和端口结构
    saLocal.sin_addr.s_addr      = htonl(INADDR_ANY);
    saLocal.sin_port             = htons(GRS_SERVER_PORT);

    if(0 != bind(skServer,(SOCKADDR*)&saLocal,sizeof(SOCKADDR_IN)))
    {
        GRS_PRINTF(_T("绑定到指定地址端口出错!\n"));
        goto CLEAN_UP;
    }

    getsockname(skServer,(SOCKADDR*)&saLocal,&isaLen);
    GRS_PRINTFA("绑定到本地IPv4地址[%s:%d]\n",inet_ntoa(saLocal.sin_addr),ntohs(saLocal.sin_port));

    //准备组播地址
    CopyMemory(&ipMCast.imr_interface.s_addr,&saLocal.sin_addr,sizeof(IN_ADDR));
    ipMCast.imr_multiaddr.s_addr = inet_addr(GRS_MCAST_IP);

    //加入组
    if( 0 != setsockopt(skServer,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char*)&ipMCast,sizeof(ip_mreq)))
    {
        GRS_PRINTF(_T("加入组[%s:%d]失败\n"),GRS_MCAST_IP,GRS_SERVER_PORT);
        goto CLEAN_UP;
    }

    //设置组播的TTL,实际也就是发送多远(穿过多少个路由,当然路由要支持MULTICAST)
    setsockopt(skServer,IPPROTO_IP,IP_MULTICAST_TTL,(char*)& dwTTL,sizeof(int));
    //设置组播是否回播,也就是自己发的消息自己要不要收到,这里是不接收回播
    setsockopt(skServer,IPPROTO_IP,IP_MULTICAST_LOOP,(char*)& bLoopBack,sizeof(bLoopBack));


    for(int i = 0 ; i< 100; i++)
    {//循环100次
        pskBuf = GRS_CALLOC(iBufLen);
        pskBufTmp = pskBuf;
        do 
        {
            iRecv = recvfrom(skServer,(char*)pskBufTmp,iBufLen - iBufLenTmp,0,(SOCKADDR*)&saClient,&isaLen);
            if(SOCKET_ERROR == iRecv || 0 == iRecv)
            {
                break;
            }

            if( iRecv >= (iBufLen - iBufLenTmp) )
            {//其实只可能出现==的情况
                iBufLenTmp = iBufLen;
                iBufLen += 1024;
                pskBuf = GRS_CREALLOC(pskBuf,iBufLen);
                pskBufTmp = (void*)((BYTE*)pskBuf + iBufLenTmp);
                continue;
            }
            else
            {
                break;
            }
        } 
        while( 1 );

        if(SOCKET_ERROR != iRecv && 0 != iRecv)
        {
            GRS_PRINTFA("接收到主机[%s : %u]的数据:%S\n"
                ,inet_ntoa(saClient.sin_addr),ntohs(saClient.sin_port),pskBuf);
        }

        GRS_SAFEFREE(pskBuf);
        pskBufTmp = NULL;
        iBufLenTmp = 0;
        iBufLen = 1024;
    }

    //退出组
    setsockopt(skServer,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char*)&ipMCast,sizeof(ip_mreq));

    //==========================================================================================================
CLEAN_UP:
    _tsystem(_T("PAUSE"));
    if(INVALID_SOCKET != skServer)
    {
        closesocket(skServer);
    }    
    ::WSACleanup();
    return 0;
}