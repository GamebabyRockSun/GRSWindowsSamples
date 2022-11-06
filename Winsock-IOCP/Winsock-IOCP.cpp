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

#define GRS_BEGINTHREAD(Fun,Param) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Fun,Param,0,NULL)

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


//IOCP线程池线程函数
DWORD WINAPI IOCPThread(LPVOID lpParameter);

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
    HANDLE  hIocp               = NULL;
    HANDLE* phIocpThread        = NULL;
    SHORT   nVirtKey            = 0;
    WSABUF  DataBuf             = {};
    BYTE buffer[DATA_BUFSIZE]   = {};

    MYOVERLAPPED AcceptOverlapped = {};
    SOCKET ListenSocket, AcceptSocket;

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

    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_port = htons(GRS_SERVER_PORT);
    service.sin_addr.s_addr = INADDR_ANY;

    bind(ListenSocket, (SOCKADDR *) &service, sizeof(SOCKADDR));

    listen(ListenSocket, 1);
    GRS_PRINTF(_T("Listening...\n"));

    AcceptSocket = accept(ListenSocket, NULL, NULL);
    GRS_PRINTF(_T("Client Accepted...\n"));

    //将SOCKET句柄与完成端口对象绑定,第二次调用CreateIoCompletionPort
    CreateIoCompletionPort((HANDLE)AcceptSocket,hIocp,NULL,0);

    DataBuf.len = DATA_BUFSIZE;
    DataBuf.buf = (CHAR*)buffer;

    //将下列重要信息通过扩展OVERLAPPED的结构带到回调过程中去
    AcceptOverlapped.m_lNetworkEvents   = FD_READ;
    AcceptOverlapped.m_pBuf             = buffer;
    AcceptOverlapped.m_szBufLen         = DATA_BUFSIZE;
    AcceptOverlapped.m_socket           = AcceptSocket;

    //注意下面这句也会将AcceptSocket强制置为OVERLAPPED工作方式
    if ( WSARecv(AcceptSocket, &DataBuf, 1, &AcceptOverlapped.m_dwTrasBytes
        , &AcceptOverlapped.m_dwFlags, (WSAOVERLAPPED*)&AcceptOverlapped, NULL) == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            GRS_PRINTF(_T("Error occurred at WSARecv()\n"));
        }
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
    } while (WAIT_TIMEOUT == WaitForSingleObject(GetCurrentThread(),1000));

    for(DWORD i = 0; i < 2 * si.dwNumberOfProcessors; i ++)
    {
        //向IOCP线程池发送退出消息 利用标志FD_CLOSE标志
        AcceptOverlapped.m_lNetworkEvents = FD_CLOSE;
        PostQueuedCompletionStatus(hIocp,0,0,(LPOVERLAPPED)&AcceptOverlapped);
    }
    WaitForMultipleObjects(2*si.dwNumberOfProcessors,phIocpThread,TRUE,INFINITE);

    for(DWORD i = 0; i < 2 * si.dwNumberOfProcessors; i ++)
    {
        CloseHandle(phIocpThread[i]);
    }
    GRS_SAFEFREE(phIocpThread);

    CloseHandle(hIocp);

    _tsystem(_T("PAUSE"));
    if(INVALID_SOCKET != ListenSocket)
    {
        closesocket(ListenSocket);
    }    
    ::WSACleanup();
}

//IOCP线程池线程函数
DWORD WINAPI IOCPThread(LPVOID lpParameter)
{
    GRS_USEPRINTF();
    ULONG_PTR    Key = NULL;
    OVERLAPPED* lpOverlapped = NULL;
    MYOVERLAPPED* pMyOl = NULL;
    DWORD dwBytesTransfered = 0;
    DWORD dwFlags = 0;
    BOOL bRet = TRUE;

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
        case FD_WRITE:
            {
                GRS_PRINTF(_T("线程[0x%x]得到IO完成通知,完成操作(WSASend),缓冲(0x%08x)长度(%ubytes)\n"),
                    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);
                closesocket(pMyOl->m_socket);
            }
            break;
        case FD_READ:
            {
                GRS_PRINTF(_T("接收数据成功,发送返回......\n"));
                pMyOl->m_lNetworkEvents = FD_WRITE;
                pMyOl->m_dwFlags = 0;
                WSABUF wsabuf = {dwBytesTransfered,(CHAR*)pMyOl->m_pBuf};

                if(SOCKET_ERROR == WSASend(pMyOl->m_socket,&wsabuf,1,&pMyOl->m_dwTrasBytes,pMyOl->m_dwFlags
                    ,(LPWSAOVERLAPPED)pMyOl,NULL))
                {
                    int iErrorCode = WSAGetLastError();
                    if (iErrorCode != WSA_IO_PENDING)
                    {
                        GRS_PRINTF(_T("Error occurred at WSASend(),Error Code:0x%08X\n"),iErrorCode);
                        GRS_PRINTF(_T("发送数据失败,关闭SOCKET句柄......\n"));
                        closesocket(pMyOl->m_socket);
                    }
                }

            }
            break;
        case FD_CLOSE:
            {
                GRS_PRINTF(_T("IOCP线程[0x%x]得到退出通知,IOCP线程退出\n"),
                    GetCurrentThreadId());

                bLoop = FALSE;//退出循环
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
