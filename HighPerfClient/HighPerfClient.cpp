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

#define GRS_SERVER_PORT 5000
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

SOCKADDR_IN   g_service     = {};
SOCKADDR_IN   g_LocalIP     = {};

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

    int iMaxConnectEx           = 1000;
    int iIndex                  = 0;
    HANDLE  hIocp               = NULL;
    HANDLE* phIocpThread        = NULL;
    SHORT   nVirtKey            = 0;
    WSABUF  DataBuf             = {};
    BYTE buffer[DATA_BUFSIZE]   = {};
    //用于绑定本地地址的结构
    int result = 0;

    MYOVERLAPPED *pOLConnectEx  = NULL;
    SOCKET ConnectSocket        = INVALID_SOCKET;
    SOCKET* pskConnectArray     = NULL;
    MYOVERLAPPED** pMyOLArray   = NULL;

    SOCKADDR_IN saLocal         = {};
    int         iAddrLen        = sizeof(SOCKADDR_IN);
    const int   on              = 1; 

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

    g_MsSockFun.LoadAllFun(INVALID_SOCKET);

    g_service.sin_family        = AF_INET;
    g_service.sin_port          = htons(GRS_SERVER_PORT);
    g_service.sin_addr.s_addr   = inet_addr("127.0.0.1");//inet_addr("192.168.2.106");//服务端地址;

    g_LocalIP.sin_family          = AF_INET;
    g_LocalIP.sin_addr.s_addr     = INADDR_ANY;//inet_addr("192.168.2.100");
    g_LocalIP.sin_port            = htons( (short)0 );	//使用0让系统自动分配

    pskConnectArray = (SOCKET*)GRS_CALLOC(iMaxConnectEx * sizeof(SOCKET));
    pMyOLArray = (MYOVERLAPPED**)GRS_CALLOC(iMaxConnectEx * sizeof(MYOVERLAPPED*));

    //发起AcceptEx调用
    for(int i = 0; i < iMaxConnectEx; i ++)
    {
        ConnectSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED);
        //将SOCKET句柄与完成端口对象绑定
        CreateIoCompletionPort((HANDLE)ConnectSocket,hIocp,NULL,0);

        //允许地址重用
        setsockopt(ConnectSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
       
        result =::bind(ConnectSocket,(LPSOCKADDR)&g_LocalIP,sizeof(SOCKADDR_IN));

        getsockname(ConnectSocket,(SOCKADDR*)&saLocal,&iAddrLen);
        GRS_PRINTFA("SOCKET绑定被分配地址为[%s:%u]\n"
            ,inet_ntoa(saLocal.sin_addr),ntohs(saLocal.sin_port));

        pOLConnectEx = (MYOVERLAPPED*)GRS_CALLOC(sizeof(MYOVERLAPPED));

        pOLConnectEx->m_skListen        = INVALID_SOCKET;    
        pOLConnectEx->m_socket          = ConnectSocket;
        pOLConnectEx->m_lNetworkEvents  = FD_CONNECT;
        pOLConnectEx->m_pBuf            = GRS_CALLOC(sizeof(SOCKADDR_IN));
        pOLConnectEx->m_szBufLen        = sizeof(SOCKADDR_IN);

        CopyMemory(pOLConnectEx->m_pBuf,&g_service,sizeof(SOCKADDR_IN));

        if(!g_MsSockFun.ConnectEx(ConnectSocket
            ,(SOCKADDR*)pOLConnectEx->m_pBuf,sizeof(SOCKADDR)
            ,NULL,0,&pOLConnectEx->m_dwTrasBytes,(LPOVERLAPPED)pOLConnectEx))
        {
            int iError = WSAGetLastError();
            if( ERROR_IO_PENDING != iError 
                && WSAECONNRESET != iError )
            {
                if(INVALID_SOCKET != ConnectSocket)
                {
                    ::closesocket(ConnectSocket);
                    ConnectSocket = INVALID_SOCKET;
                }
                GRS_SAFEFREE(pOLConnectEx->m_pBuf);
                GRS_SAFEFREE(pOLConnectEx);        
                GRS_PRINTF(_T("调用ConnectEx失败,错误码:%d\n"),iError);
                continue;
            }

        }

        pskConnectArray[iIndex] = ConnectSocket;
        pMyOLArray[iIndex]  = pOLConnectEx;
        ++ iIndex;
        GRS_PRINTF(_T("ConnectEx(%d)...\n"),i);
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

    MYOVERLAPPED CloseOL = {};
    //向IOCP线程池发送退出消息 利用标志FD_CLOSE标志
    CloseOL.m_socket = INVALID_SOCKET;
    CloseOL.m_lNetworkEvents = FD_CLOSE;
    for(DWORD i = 0; i < 2 * si.dwNumberOfProcessors; i ++)
    {
        PostQueuedCompletionStatus(hIocp,0,0,(LPOVERLAPPED)&CloseOL);
    }
    WaitForMultipleObjects(2*si.dwNumberOfProcessors,phIocpThread,TRUE,INFINITE);

    for(DWORD i = 0; i < 2 * si.dwNumberOfProcessors; i ++)
    {
        CloseHandle(phIocpThread[i]);
    }
    GRS_SAFEFREE(phIocpThread);

    CloseHandle(hIocp);

    //释放SOCKET及MYOVERLAPPED资源
    for(int i = 0; i < iIndex; i ++)
    {
        closesocket(pskConnectArray[i]);
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

    ULONG nMsgType          = 0x1;
    DWORD dwMsgLen          = 0;
    WCHAR pszSendMsg[]      = _T("Hello Internet!");
    size_t szLen = 0;
    StringCchLength(pszSendMsg,30,&szLen);
    ULONG nMsgTail          = 0x7FFF;

    WSABUF  wba[5]  = {};
    WCHAR   pszRecvBuf[1024]  = {};

    wba[0].buf = (CHAR*)&nMsgType;
    wba[0].len = sizeof(ULONG);

    dwMsgLen = (szLen + 1) * sizeof(WCHAR);
    wba[1].buf = (CHAR*)&dwMsgLen;
    wba[1].len = sizeof(size_t);

    wba[2].buf = (CHAR*)pszSendMsg;
    wba[2].len = (szLen + 1) * sizeof(WCHAR);

    wba[3].buf = (CHAR*)&nMsgTail;
    wba[3].len = sizeof(nMsgTail);

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
        case FD_CONNECT:
            {
                //GRS_PRINTF(_T("线程[0x%x]完成操作(ConnectEx),缓冲(0x%08x)长度(%ubytes)\n"),
                //    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                int nRet = ::setsockopt(
                    pMyOl->m_socket, 
                    SOL_SOCKET,
                    SO_UPDATE_CONNECT_CONTEXT,
                    NULL,
                    0
                    );

                //int iBufLen = 0;
                ////关闭套接字上的发送缓冲，这样可以提高性能
                //::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_SNDBUF,(const char*)&iBufLen,sizeof(int));
                //::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_RCVBUF,(const char*)&iBufLen,sizeof(int));

                ////强制发送延时算法关闭,直接发送到网络上去
                //DWORD dwNo  = 0;
                //::setsockopt(pMyOl->m_socket,IPPROTO_TCP,TCP_NODELAY,(char*)&dwNo,sizeof(DWORD));

                //BOOL bDontLinger = FALSE; 
                //::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_DONTLINGER,(const char*)&bDontLinger,sizeof(BOOL));

                //linger sLinger = {};
                //sLinger.l_onoff = 1;
                //sLinger.l_linger = 0;
                //::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_LINGER,(const char*)&sLinger,sizeof(linger));


                nMsgType    = 0x1;
                nMsgTail    = 0x7FFF;

                wba[0].buf  = (CHAR*)&nMsgType;
                wba[0].len  = sizeof(ULONG);

                wba[1].buf  = (CHAR*)&dwMsgLen;
                wba[1].len  = sizeof(size_t);

                wba[2].buf  = (CHAR*)pszSendMsg;
                wba[2].len  = (szLen + 1) * sizeof(WCHAR);

                wba[3].buf  = (CHAR*)&nMsgTail;
                wba[3].len  = sizeof(nMsgTail);

                GRS_SAFEFREE(pMyOl->m_pBuf);
                pMyOl->m_szBufLen = 0;
                pMyOl->m_lNetworkEvents = FD_WRITE;
                
                WSASend(pMyOl->m_socket,wba,4,&pMyOl->m_dwTrasBytes,0,(LPOVERLAPPED)&pMyOl->m_wsaol,NULL);

            }
            break;
        case FD_WRITE:
            {
                //GRS_PRINTF(_T("线程[0x%x]完成操作(WSASend),缓冲(0x%08x)长度(%ubytes)\n"),
                //    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                nMsgType = 0;
                ZeroMemory(pszRecvBuf,1024 * sizeof(WCHAR));
                wba[2].buf = (CHAR*)pszRecvBuf;
                wba[2].len = 1024 * sizeof(WCHAR);
                nMsgTail = 0;

                pMyOl->m_lNetworkEvents = FD_READ;

                WSARecv(pMyOl->m_socket,wba,4,&pMyOl->m_dwTrasBytes,&pMyOl->m_dwFlags,(LPOVERLAPPED)&pMyOl->m_wsaol,NULL);
            }
            break;
        case FD_READ:
            {
                //GRS_PRINTF(_T("线程[0x%x]完成操作(WSARecv),缓冲(0x%08x)长度(%ubytes)\n"),
                //    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                GRS_PRINTF(_T("ECHO:%s\n"),(WCHAR*)wba[2].buf);

                //GRS_PRINTFA("ECHO:%s\n",wba[2].buf);

                shutdown(pMyOl->m_socket,SD_BOTH);

                pMyOl->m_lNetworkEvents = FD_CLOSE;
                Sleep(2000);
                //回收SOCKET
                g_MsSockFun.DisconnectEx(pMyOl->m_socket,(LPOVERLAPPED)pMyOl,TF_REUSE_SOCKET,0);

            }
            break;
        case FD_CLOSE:
            {
                if( INVALID_SOCKET == pMyOl->m_socket )
                {
                    //GRS_PRINTF(_T("IOCP线程[0x%x]得到退出通知,IOCP线程退出\n"),
                    //    GetCurrentThreadId());

                    bLoop = FALSE;//退出循环
                }
                else
                {         
                    //GRS_PRINTF(_T("线程[0x%x]完成操作(DisconnectEx),缓冲(0x%08x)长度(%ubytes)\n"),
                    //    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                    GRS_PRINTF(_T("回收SOCKET成功......\n"));
                    pMyOl->m_lNetworkEvents = FD_CONNECT;
                    pMyOl->m_pBuf           = GRS_CALLOC(sizeof(SOCKADDR_IN));
                    pMyOl->m_szBufLen       = sizeof(SOCKADDR_IN);
                    CopyMemory(pMyOl->m_pBuf,&g_service,sizeof(SOCKADDR_IN));

                    //回收成功,重新丢入连接池
                    g_MsSockFun.ConnectEx(pMyOl->m_socket
                        ,(SOCKADDR*)pMyOl->m_pBuf,sizeof(SOCKADDR_IN)
                        ,NULL,0,&pMyOl->m_dwTrasBytes,(LPOVERLAPPED)&pMyOl->m_wsaol);
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
