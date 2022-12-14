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
    WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS]    = {};
    SOCKET   SocketArray[WSA_MAXIMUM_WAIT_EVENTS]   = {};
    WSANETWORKEVENTS NetworkEvents                  = {};
    DWORD    dwEventTatol   = 0;
    DWORD    dwIndex        = 0;
    WSAEVENT wsaEventServer = WSACreateEvent();
    
    SOCKET skServer         = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    SOCKET skClient         = INVALID_SOCKET;
    SOCKADDR_IN saClient    = {};
    int         isaLen      = sizeof(SOCKADDR_IN);
    VOID*       pskBuf      = NULL;
    VOID*       pskBufTmp   = NULL;
    int         iBufLen     = 1024;
    int         iBufLenTmp  = 0;
    int         iRecv       = 0;

    SOCKADDR_IN sa          = {AF_INET};
    sa.sin_addr.s_addr      = htonl(INADDR_ANY);
    sa.sin_port             = htons(GRS_SERVER_PORT);

    if(0 != bind(skServer,(SOCKADDR*)&sa,sizeof(SOCKADDR_IN)))
    {
        GRS_PRINTF(_T("绑定到指定地址端口出错!\n"));
        goto CLEAN_UP;
    }

    WSAEventSelect( skServer,wsaEventServer, FD_ACCEPT | FD_CLOSE);
    SocketArray[dwEventTatol]   = skServer;
    EventArray[dwEventTatol ++] = wsaEventServer;


    if(0 != listen(skServer,SOMAXCONN) )
    {
        GRS_PRINTF(_T("SOCKET进入监听模式出错!\n"));
        goto CLEAN_UP;
    }

    while( TRUE )
    {
        dwIndex = WSAWaitForMultipleEvents(dwEventTatol, EventArray, FALSE, WSA_INFINITE, FALSE);
        if ( (dwIndex == WSA_WAIT_FAILED) || (dwIndex == WSA_WAIT_TIMEOUT) )
        {
            GRS_PRINTF( _T("WSAWaitForMultipleEvents调用失败,错误码:0x%08X"), WSAGetLastError() );
            break;
        }

        if(0 != WSAEnumNetworkEvents( SocketArray[dwIndex - WSA_WAIT_EVENT_0]
            ,EventArray[dwIndex - WSA_WAIT_EVENT_0],&NetworkEvents) ) 
        {
            GRS_PRINTF(_T("WSAEnumNetworkEvents调用失败,错误码:0x%08X"),WSAGetLastError());
            break;
        }

        //if( !WSAResetEvent(EventArray[dwIndex - WSA_WAIT_EVENT_0]) )
        //{
        //    GRS_PRINTF(_T("WSAResetEvent调用失败,错误码:0x%08X"),WSAGetLastError());
        //    break;
        //}

        if( (NetworkEvents.lNetworkEvents & FD_ACCEPT) && 0 == NetworkEvents.iErrorCode[FD_ACCEPT_BIT] )
        {//accept事件新增socket和wsaevent元素进入数组
            skClient = accept(skServer,(SOCKADDR*)&saClient,&isaLen);
            if(INVALID_SOCKET == skClient)
            {
                GRS_PRINTF(_T("accept调用失败,错误码:0x%08X"),WSAGetLastError());
                break;
            }

            GRS_PRINTFA("客户端[%s : %u]连接进来......\n"
                ,inet_ntoa(saClient.sin_addr),ntohs(saClient.sin_port));

            EventArray[dwEventTatol] = WSACreateEvent();

            WSAEventSelect( skClient,EventArray[dwEventTatol], FD_READ | FD_WRITE | FD_CLOSE);
            
            SocketArray[dwEventTatol++] = skClient;            
        }

        if( (NetworkEvents.lNetworkEvents & FD_CLOSE ) && 0 == NetworkEvents.iErrorCode[FD_CLOSE_BIT])
        {//关闭SOCKET 同时关闭对应的WSAEvent句柄,数组之后的元素全部前移
            closesocket(SocketArray[dwIndex - WSA_WAIT_EVENT_0]);
            WSACloseEvent(EventArray[dwIndex - WSA_WAIT_EVENT_0]);

            for(DWORD i = dwIndex - WSA_WAIT_EVENT_0; i < dwEventTatol; i ++ )
            {
                SocketArray[i] = SocketArray[i + 1];
                EventArray[i] = EventArray[i + 1];
            }
            -- dwEventTatol; 
        }

        if( (NetworkEvents.lNetworkEvents & FD_READ ) && 0 == NetworkEvents.iErrorCode[FD_CLOSE_BIT] ) 
        {
            pskBuf = GRS_CALLOC(iBufLen);
            pskBufTmp = pskBuf;
            do 
            {
                iRecv = recv(SocketArray[dwIndex - WSA_WAIT_EVENT_0]
                        ,(char*)pskBufTmp,iBufLen - iBufLenTmp,0);

                if( SOCKET_ERROR == iRecv || 0 == iRecv )
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

            if( SOCKET_ERROR != iRecv && 0 != iRecv )
            {
                do 
                {
                    send( SocketArray[dwIndex - WSA_WAIT_EVENT_0],(const char*)pskBuf,iBufLen,0 );
                    Sleep(20);//停20ms
                } while (WSAEWOULDBLOCK == WSAGetLastError());
            }

            GRS_SAFEFREE(pskBuf);
            pskBufTmp = NULL;
            iBufLenTmp = 0;
            iBufLen = 1024;

            shutdown(SocketArray[dwIndex - WSA_WAIT_EVENT_0],SD_BOTH);

            closesocket(SocketArray[dwIndex - WSA_WAIT_EVENT_0]);
            WSACloseEvent(EventArray[dwIndex - WSA_WAIT_EVENT_0]);

            for(DWORD i = dwIndex - WSA_WAIT_EVENT_0; i < dwEventTatol; i ++ )
            {
                SocketArray[i] = SocketArray[i + 1];
                EventArray[i] = EventArray[i + 1];
            }
            -- dwEventTatol; 
        }
    }

    //==========================================================================================================
CLEAN_UP:
    _tsystem(_T("PAUSE"));
    if(INVALID_SOCKET != skServer)
    {
        closesocket(skServer);
    }    
    if(NULL != wsaEventServer)
    {
        WSACloseEvent(wsaEventServer);
    }
    ::WSACleanup();
    return 0;
}