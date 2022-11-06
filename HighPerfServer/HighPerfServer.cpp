#include <tchar.h>
#define WIN32_LEAN_AND_MEAN	
#include <windows.h>
#include <strsafe.h>
#include <Winsock2.h>

#include "../GRSMsSockFun.h"

#pragma comment( lib, "Ws2_32.lib" )

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_CREALLOC(p,sz)	HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_BEGINTHREAD(Fun,Param) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Fun,Param,0,NULL)

#define SOCK_VERH 2
#define SOCK_VERL 2

#define GRS_SERVER_PORT 8080
#define DATA_BUFSIZE 4096

struct MYOVERLAPPED 
{
    WSAOVERLAPPED m_wsaol;
    SOCKET        m_skListen;      //监听套接字句柄
    long          m_lNetworkEvents;//投递的操作类型(FD_READ/FD_WRITE等)
    SOCKET        m_socket;        //投递操作的SOCKET句柄
    void*         m_pBuf;          //投递操作时的缓冲
    size_t        m_szBufLen;      //缓冲长度
    DWORD		  m_dwTrasBytes;   //为WSASent和WSARecv准备的参数
    DWORD         m_dwFlags;       //为WSARecv准备的
};


//IOCP线程池线程函数
DWORD WINAPI IOCPThread(LPVOID lpParameter);

CGRSMsSockFun g_MsSockFun;

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
    
    int iMaxAcceptEx            = 500;
    int iIndex                  = 0;
    HANDLE  hIocp               = NULL;
    HANDLE* phIocpThread        = NULL;
    SHORT   nVirtKey            = 0;
    WSABUF  DataBuf             = {};
    BYTE buffer[DATA_BUFSIZE]   = {};

    MYOVERLAPPED AcceptOverlapped = {};
    MYOVERLAPPED *pOLAcceptEx = NULL;
    SOCKET ListenSocket, AcceptSocket;
    SOCKET* pskAcceptArray = NULL;
    MYOVERLAPPED** pMyOLArray = NULL;

    SYSTEM_INFO si = {};
    GetSystemInfo(&si);
    //创建IOCP内核对象,允许最大并发CPU个数个线程
    hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,NULL,si.dwNumberOfProcessors);

    //实际创建2倍CPU个数个线程
    phIocpThread = (HANDLE*)GRS_CALLOC(2 * si.dwNumberOfProcessors * sizeof(HANDLE));
    for(DWORD i = 0; i < 2 * si.dwNumberOfProcessors; i ++)
    {
        phIocpThread[i] = GRS_BEGINTHREAD(IOCPThread,hIocp);
    }

    ListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED);

    //将SOCKET句柄与完成端口对象绑定
    //注意监听的套接字一定要先和IOCP绑定,否则AcceptEx就无法利用IOCP处理
    CreateIoCompletionPort((HANDLE)ListenSocket,hIocp,NULL,0);

    g_MsSockFun.LoadAllFun(ListenSocket);

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_port = htons(GRS_SERVER_PORT);
    service.sin_addr.s_addr = INADDR_ANY;

    bind(ListenSocket, (SOCKADDR *) &service, sizeof(SOCKADDR));

    listen(ListenSocket, SOMAXCONN);
    GRS_PRINTF(_T("Listening...\n"));

    pskAcceptArray = (SOCKET*)GRS_CALLOC(iMaxAcceptEx * sizeof(SOCKET));
    pMyOLArray = (MYOVERLAPPED**)GRS_CALLOC(iMaxAcceptEx * sizeof(MYOVERLAPPED*));
    
    //发起AcceptEx调用
    for(int i = 0; i < iMaxAcceptEx; i ++)
    {
        AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED);
        //将SOCKET句柄与完成端口对象绑定
        CreateIoCompletionPort((HANDLE)AcceptSocket,hIocp,NULL,0);

    
        pOLAcceptEx = (MYOVERLAPPED*)GRS_CALLOC(sizeof(MYOVERLAPPED));
        pOLAcceptEx->m_skListen         = ListenSocket;    
        pOLAcceptEx->m_socket           = AcceptSocket;
        pOLAcceptEx->m_lNetworkEvents   = FD_ACCEPT;
        pOLAcceptEx->m_pBuf             = GRS_CALLOC(2 * (sizeof(SOCKADDR_IN) + 16));
        pOLAcceptEx->m_szBufLen         = 2 * (sizeof(SOCKADDR_IN) + 16);

        if(!g_MsSockFun.AcceptEx(ListenSocket,AcceptSocket
            ,pOLAcceptEx->m_pBuf,0,sizeof(SOCKADDR_IN)+16,sizeof(SOCKADDR_IN) + 16
            ,&pOLAcceptEx->m_dwFlags,(LPOVERLAPPED)&pOLAcceptEx->m_wsaol))
        {
            int iError = WSAGetLastError();
            if( ERROR_IO_PENDING != iError 
                && WSAECONNRESET != iError )
            {
                if(INVALID_SOCKET != AcceptSocket)
                {
                    ::closesocket(AcceptSocket);
                    AcceptSocket = INVALID_SOCKET;
                }
                GRS_SAFEFREE(pOLAcceptEx->m_pBuf);
                GRS_SAFEFREE(pOLAcceptEx);        
                GRS_PRINTF(_T("调用AcceptEx失败,错误码:%d\n"),iError);
                continue;
            }

        }

        pskAcceptArray[iIndex] = AcceptSocket;
        pMyOLArray[iIndex]  = pOLAcceptEx;
        ++ iIndex;
        GRS_PRINTF(_T("AcceptEx(%d)...\n"),i);
    }

    //主线程进入等待状态
    do 
    {
        nVirtKey = GetAsyncKeyState(VK_ESCAPE); 
        if (nVirtKey & 0x8000) 
        {//按ESC键退出
            break;
        }
        GRS_PRINTF(_T("主线程进入等待状态......\n"));
    } while (WAIT_TIMEOUT == WaitForSingleObject(GetCurrentThread(),10000));

    //向IOCP线程池发送退出消息 利用标志FD_CLOSE标志
    AcceptOverlapped.m_socket = INVALID_SOCKET;
    AcceptOverlapped.m_lNetworkEvents = FD_CLOSE;
    for(DWORD i = 0; i < 2 * si.dwNumberOfProcessors; i ++)
    {
        PostQueuedCompletionStatus(hIocp,0,0,(LPOVERLAPPED)&AcceptOverlapped);
    }
    WaitForMultipleObjects(2*si.dwNumberOfProcessors,phIocpThread,TRUE,INFINITE);

    for(DWORD i = 0; i < 2 * si.dwNumberOfProcessors; i ++)
    {
        CloseHandle(phIocpThread[i]);
    }
    GRS_SAFEFREE(phIocpThread);

    CloseHandle(hIocp);

    //释放SOCKET及MYOVERLAPPED资源
    if(INVALID_SOCKET != ListenSocket)
    {
        closesocket(ListenSocket);
    } 

    for(int i = 0; i < iIndex; i ++)
    {
        closesocket(pskAcceptArray[i]);
        GRS_SAFEFREE(pMyOLArray[i]);
    }

    _tsystem(_T("PAUSE"));
    ::WSACleanup();

}

//IOCP线程池线程函数
DWORD WINAPI IOCPThread(LPVOID lpParameter)
{
    GRS_USEPRINTF();
    ULONG_PTR       Key                 = NULL;
    OVERLAPPED*     lpOverlapped        = NULL;
    MYOVERLAPPED*   pMyOl               = NULL;
    DWORD           dwBytesTransfered   = 0;
    DWORD           dwFlags             = 0;
    BOOL            bRet                = TRUE;

    HANDLE hIocp = (HANDLE)lpParameter;
    BOOL bLoop = TRUE;

    while (bLoop)
    {
        bRet = GetQueuedCompletionStatus(hIocp,&dwBytesTransfered,(PULONG_PTR)&Key,&lpOverlapped,INFINITE);
        pMyOl = CONTAINING_RECORD(lpOverlapped,MYOVERLAPPED,m_wsaol);

        if( FALSE == bRet )
        {
            GRS_PRINTF(_T("IOCPThread: GetQueuedCompletionStatus 调用失败,错误码: 0x%08x 内部错误码[0x%08x]\n"),
                GetLastError(), lpOverlapped->Internal);
            continue;
        }

        switch(pMyOl->m_lNetworkEvents)
        {
        case FD_ACCEPT:
            {
                GRS_PRINTF(_T("线程[0x%x]完成操作(AcceptEx),缓冲(0x%08x)长度(%ubytes)\n"),
                    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                SOCKADDR_IN* psaLocal = NULL;
                int         iLocalLen = sizeof(SOCKADDR_IN);
                SOCKADDR_IN* psaRemote = NULL;
                int         iRemoteLen = sizeof(SOCKADDR_IN);

                g_MsSockFun.GetAcceptExSockaddrs(pMyOl->m_pBuf,0
                    ,sizeof(SOCKADDR_IN) + 16,sizeof(SOCKADDR_IN) + 16
                    ,(SOCKADDR**)&psaLocal,&iLocalLen,(SOCKADDR**)&psaRemote,&iRemoteLen);
                
                GRS_PRINTFA("客户端[%s:%d]连接进入,本地通讯地址[%s:%d]\n"
                    ,inet_ntoa(psaRemote->sin_addr),ntohs(psaRemote->sin_port)
                    ,inet_ntoa(psaLocal->sin_addr),ntohs(psaLocal->sin_port));

                GRS_SAFEFREE(pMyOl->m_pBuf);

                int nRet = ::setsockopt(
                    pMyOl->m_socket, 
                    SOL_SOCKET,
                    SO_UPDATE_ACCEPT_CONTEXT,
                    (char *)&pMyOl->m_skListen,
                    sizeof(SOCKET)
                    );
                
                int iBufLen = 0;
                //关闭套接字上的发送缓冲，这样可以提高性能
                ::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_SNDBUF,(const char*)&iBufLen,sizeof(int));
                ::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_RCVBUF,(const char*)&iBufLen,sizeof(int));
                
                //强制发送延时算法关闭,直接发送到网络上去
                DWORD dwNo = 0;
                ::setsockopt(pMyOl->m_socket,IPPROTO_TCP,TCP_NODELAY,(char*)&dwNo,sizeof(DWORD));

                BOOL bDontLinger = FALSE; 
                ::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_DONTLINGER,(const char*)&bDontLinger,sizeof(BOOL));

                linger sLinger = {};
                sLinger.l_onoff = 1;
                sLinger.l_linger = 0;
                ::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_LINGER,(const char*)&sLinger,sizeof(linger));

                WSABUF wsabuf = {0,NULL};
                //将下列重要信息通过扩展OVERLAPPED的结构带到回调过程中去
                pMyOl->m_lNetworkEvents   = FD_READ;
                pMyOl->m_pBuf             = NULL;
                pMyOl->m_szBufLen         = 0;
                
                if ( WSARecv(pMyOl->m_socket, &wsabuf, 1, &pMyOl->m_dwTrasBytes
                    , &pMyOl->m_dwFlags, (WSAOVERLAPPED*)pMyOl, NULL) == SOCKET_ERROR)
                {
                    int iErrorCode = WSAGetLastError();
                    if (iErrorCode != WSA_IO_PENDING)
                    {
                        GRS_PRINTF(_T("Error occurred at WSARecv(%d)\n"),iErrorCode);
                    }
                }
            }
            break;
        case FD_WRITE:
            {
                GRS_PRINTF(_T("线程[0x%x]完成操作(WSASend),缓冲(0x%08x)长度(%ubytes)\n"),
                    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                GRS_SAFEFREE(pMyOl->m_pBuf);

                shutdown(pMyOl->m_socket,SD_BOTH);
                
                pMyOl->m_lNetworkEvents = FD_CLOSE;
                //回收SOCKET
                g_MsSockFun.DisconnectEx(pMyOl->m_socket,(LPOVERLAPPED)pMyOl,TF_REUSE_SOCKET,0);
            }
            break;
        case FD_READ:
            {
                GRS_PRINTF(_T("线程[0x%x]完成操作(WSARecv),缓冲(0x%08x)长度(%ubytes)\n"),
                    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                GRS_PRINTF(_T("开始接收数据......\n"));
                //执行Non-Blocking接收
                DWORD dwMsgType = 0;
                DWORD dwMsgLen  = 0;
                DWORD dwTransBytes = 0;
                DWORD dwMsgTail = 0;
                
                //接收包头 类型信息
                WSABUF wsabuf[5] = {sizeof(DWORD),(CHAR*)&dwMsgType};
                //接收包头 长度信息
                wsabuf[1].buf = (CHAR*)&dwMsgLen;
                wsabuf[1].len = sizeof(DWORD);

                WSARecv(pMyOl->m_socket,&wsabuf[0],2,&dwTransBytes,&dwFlags,NULL,NULL);
                                
                //判定是否是指定的消息类型
                if(0x1 != dwMsgType)
                {//不是指定的消息类型,回收SOCKET
                    GRS_PRINTF(_T("接收的数据包头消息类型错误,回收SOCKET......\n"))
                    shutdown(pMyOl->m_socket,SD_BOTH);
                    pMyOl->m_lNetworkEvents = FD_CLOSE;
                    g_MsSockFun.DisconnectEx(pMyOl->m_socket,(LPOVERLAPPED)pMyOl,TF_REUSE_SOCKET,0);
                    break;
                }

                //判定包长度是否合理
                if( dwMsgLen < 1 || dwMsgLen >= 0xFFFF )
                {//长度不合理,回收SOCKET
                    GRS_PRINTF(_T("接收的数据包头消息长度错误,回收SOCKET......\n"))
                    shutdown(pMyOl->m_socket,SD_BOTH);
                    pMyOl->m_lNetworkEvents = FD_CLOSE;
                    g_MsSockFun.DisconnectEx(pMyOl->m_socket,(LPOVERLAPPED)pMyOl,TF_REUSE_SOCKET,0);
                    break;
                }

                //接收包数据
                wsabuf[2].buf = (CHAR*)GRS_CALLOC(dwMsgLen);
                wsabuf[2].len = dwMsgLen;
                //接收包尾
                wsabuf[3].buf = (CHAR*)&dwMsgTail;
                wsabuf[3].len = sizeof(DWORD);

                WSARecv(pMyOl->m_socket,&wsabuf[2],2,&dwTransBytes,&dwFlags,NULL,NULL);

                //判定下包尾 特定的包尾总是0x7FFF
                if( 0x7FFF!= dwMsgTail )
                {
                    GRS_PRINTF(_T("接收到包尾信息异常......\n"));
                }

                GRS_PRINTF(_T("接收数据成功,发送返回......\n"));
                
                pMyOl->m_pBuf           = wsabuf[2].buf;
                pMyOl->m_szBufLen       = wsabuf[2].len;
                pMyOl->m_lNetworkEvents = FD_WRITE;
                pMyOl->m_dwFlags        = 0;
      
                if(SOCKET_ERROR == WSASend(pMyOl->m_socket,wsabuf,4,&pMyOl->m_dwTrasBytes,pMyOl->m_dwFlags
                    ,(LPWSAOVERLAPPED)pMyOl,NULL))
                {
                    int iErrorCode = WSAGetLastError();
                    if (iErrorCode != WSA_IO_PENDING)
                    {
                        GRS_PRINTF(_T("Error occurred at WSASend(),Error Code:%d\n"),iErrorCode);
                        GRS_PRINTF(_T("发送数据失败,回收SOCKET句柄......\n"));
                        GRS_SAFEFREE(pMyOl->m_pBuf);
                        shutdown(pMyOl->m_socket,SD_BOTH);
                        pMyOl->m_lNetworkEvents = FD_CLOSE;
                        g_MsSockFun.DisconnectEx(pMyOl->m_socket,(LPOVERLAPPED)pMyOl,TF_REUSE_SOCKET,0);
                        break;
                    }
                }

            }
            break;
        case FD_CLOSE:
            {
                if( INVALID_SOCKET == pMyOl->m_socket )
                {
                    GRS_PRINTF(_T("IOCP线程[0x%x]得到退出通知,IOCP线程退出\n"),
                        GetCurrentThreadId());

                    bLoop = FALSE;//退出循环
                }
                else
                {         
                    GRS_PRINTF(_T("线程[0x%x]完成操作(DisconnectEx),缓冲(0x%08x)长度(%ubytes)\n"),
                        GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                    GRS_PRINTF(_T("回收SOCKET成功,重新AcceptEx......\n"));

                    pMyOl->m_lNetworkEvents = FD_ACCEPT;
                    pMyOl->m_pBuf           = GRS_CALLOC(2 * (sizeof(SOCKADDR_IN) + 16));
                    pMyOl->m_szBufLen       = 2 * (sizeof(SOCKADDR_IN) + 16);

                    //回收成功,重新丢入连接池
                    g_MsSockFun.AcceptEx(pMyOl->m_skListen,pMyOl->m_socket
                        ,pMyOl->m_pBuf,0,sizeof(SOCKADDR_IN)+16,sizeof(SOCKADDR_IN) + 16
                        ,&pMyOl->m_dwFlags,(LPOVERLAPPED)pMyOl);
                }
  
            }
            break;
        default:
            {
                bLoop = FALSE;
            }
            break;
        }

    }

    return 0;
}
