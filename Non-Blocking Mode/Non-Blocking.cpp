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
    SOCKET skServer         = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    SOCKET skClient         = INVALID_SOCKET;
    SOCKADDR_IN saClient    = {};
    int         isaLen      = sizeof(SOCKADDR_IN);
    VOID*       pskBuf        = NULL;
    VOID*       pskBufTmp     = NULL;
    int         iBufLen     = 1024;
    int         iBufLenTmp  = 0;
    int         iRecv       = 0;
    SOCKADDR_IN sa          = {AF_INET};
    sa.sin_addr.s_addr      = htonl(INADDR_ANY);
    sa.sin_port             = htons(GRS_SERVER_PORT);

    unsigned long ul = 1;
    int           nRet;
    int           iSleep = 1000;

    nRet = ioctlsocket(skServer, FIONBIO, (unsigned long *) &ul);
    if (nRet == SOCKET_ERROR)
    {
        GRS_PRINTF(_T("SOCKET设定为非阻塞模式失败,错误码:0x%08X!"),WSAGetLastError());
        goto CLEAN_UP;
    }


    if(0 != bind(skServer,(SOCKADDR*)&sa,sizeof(SOCKADDR_IN)))
    {
        GRS_PRINTF(_T("绑定到指定地址端口出错!\n"));
        goto CLEAN_UP;
    }

    if(0 != listen(skServer,SOMAXCONN))
    {
        GRS_PRINTF(_T("SOCKET进入监听模式出错!\n"));
        goto CLEAN_UP;
    }

    GRS_PRINTF(_T("进入监听模式,开始调用accept......!\n"));
    while( TRUE )
    {
        skClient = accept(skServer,(SOCKADDR*)&saClient,&isaLen);
        if(INVALID_SOCKET == skClient || WSAEWOULDBLOCK == WSAGetLastError())
        {
            GRS_PRINTF(_T("\taccept没有接受到连接,继续accept......!\n"));
            Sleep(iSleep);  //停20ms 继续下一个accept操作
            continue;
        }
        
        GRS_PRINTFA("客户端[%s : %u]连接进来......\n"
            ,inet_ntoa(saClient.sin_addr),ntohs(saClient.sin_port));

        //skClient已经是非阻塞模式了,不用再设置

        pskBuf = GRS_CALLOC(iBufLen);
        pskBufTmp = pskBuf;

        do 
        {
            iRecv = recv(skClient,(char*)pskBufTmp,iBufLen - iBufLenTmp,0);
            if( SOCKET_ERROR == iRecv || 0 == iRecv )
            {
                if( WSAEWOULDBLOCK == WSAGetLastError() )
                {
                    GRS_PRINTF(_T("\trecv没有接收到数据,继续recv......!\n"));
                    Sleep(iSleep);//停20ms
                    continue;
                }
                
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

        if( SOCKET_ERROR != iRecv && 0 != iRecv )
        {
            do 
            {
                GRS_PRINTF(_T("\tsend尝试发送数据......!\n"));
                send( skClient,(const char*)pskBuf,iBufLen,0 );
                Sleep(iSleep);//停20ms
            } while (WSAEWOULDBLOCK == WSAGetLastError());
        }

        GRS_PRINTF(_T("\t数据发送完毕,清理缓冲,关闭连接......!\n"));
        GRS_SAFEFREE(pskBuf);
        pskBufTmp = NULL;
        iBufLenTmp = 0;
        iBufLen = 1024;

        shutdown(skClient,SD_BOTH);
        closesocket(skClient);
        GRS_PRINTF(_T("\t连接关闭完成!\n"));
    }

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