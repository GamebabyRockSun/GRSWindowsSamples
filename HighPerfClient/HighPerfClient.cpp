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
    SOCKET        m_skListen;      //�����׽��־��
    long          m_lNetworkEvents;//Ͷ�ݵĲ�������(FD_READ/FD_WRITE��)
    SOCKET        m_socket;        //Ͷ�ݲ�����SOCKET���
    void*         m_pBuf;          //Ͷ�ݲ���ʱ�Ļ���
    size_t        m_szBufLen;      //���峤��
    DWORD		  m_dwTrasBytes;   //ΪWSASent��WSARecv׼���Ĳ���
    DWORD         m_dwFlags;       //ΪWSARecv׼����
};


//IOCP�̳߳��̺߳���
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
        GRS_PRINTF(_T("�޷���ʼ��Socket2ϵͳ������������Ϊ��%d��\n"),WSAGetLastError());
        return ;
    }
    if ( LOBYTE( wd.wVersion ) != SOCK_VERH ||
        HIBYTE( wd.wVersion ) != SOCK_VERL ) 
    {
        GRS_PRINTF(_T("�޷���ʼ��%d.%d�汾��Socket������\n"),SOCK_VERH,SOCK_VERL);
        WSACleanup( );
        return ; 
    }
    GRS_PRINTF(_T("Winsock���ʼ���ɹ�!\n\t��ǰϵͳ��֧����ߵ�Winsock�汾Ϊ%d.%d\n\t��ǰӦ�ü��صİ汾Ϊ%d.%d\n")
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
    //���ڰ󶨱��ص�ַ�Ľṹ
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
    //����IOCP�ں˶���,������󲢷�CPU�������߳�
    hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,NULL,si.dwNumberOfProcessors);

    //ʵ�ʴ���2��CPU�������߳�
    phIocpThread = (HANDLE*)GRS_CALLOC(2 * si.dwNumberOfProcessors * sizeof(HANDLE));
    for(DWORD i = 0; i < 2 * si.dwNumberOfProcessors; i ++)
    {
        phIocpThread[i] = GRS_BEGINTHREAD(IOCPThread,hIocp);
    }

    g_MsSockFun.LoadAllFun(INVALID_SOCKET);

    g_service.sin_family        = AF_INET;
    g_service.sin_port          = htons(GRS_SERVER_PORT);
    g_service.sin_addr.s_addr   = inet_addr("127.0.0.1");//inet_addr("192.168.2.106");//����˵�ַ;

    g_LocalIP.sin_family          = AF_INET;
    g_LocalIP.sin_addr.s_addr     = INADDR_ANY;//inet_addr("192.168.2.100");
    g_LocalIP.sin_port            = htons( (short)0 );	//ʹ��0��ϵͳ�Զ�����

    pskConnectArray = (SOCKET*)GRS_CALLOC(iMaxConnectEx * sizeof(SOCKET));
    pMyOLArray = (MYOVERLAPPED**)GRS_CALLOC(iMaxConnectEx * sizeof(MYOVERLAPPED*));

    //����AcceptEx����
    for(int i = 0; i < iMaxConnectEx; i ++)
    {
        ConnectSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED);
        //��SOCKET�������ɶ˿ڶ����
        CreateIoCompletionPort((HANDLE)ConnectSocket,hIocp,NULL,0);

        //�����ַ����
        setsockopt(ConnectSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
       
        result =::bind(ConnectSocket,(LPSOCKADDR)&g_LocalIP,sizeof(SOCKADDR_IN));

        getsockname(ConnectSocket,(SOCKADDR*)&saLocal,&iAddrLen);
        GRS_PRINTFA("SOCKET�󶨱������ַΪ[%s:%u]\n"
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
                GRS_PRINTF(_T("����ConnectExʧ��,������:%d\n"),iError);
                continue;
            }

        }

        pskConnectArray[iIndex] = ConnectSocket;
        pMyOLArray[iIndex]  = pOLConnectEx;
        ++ iIndex;
        GRS_PRINTF(_T("ConnectEx(%d)...\n"),i);
    }

    //���߳̽���ȴ�״̬
    do 
    {
        nVirtKey = GetAsyncKeyState(VK_ESCAPE); 
        if (nVirtKey & 0x8000) 
        {//��ESC���˳�
            break;
        }
        GRS_PRINTF(_T("���߳̽���ȴ�״̬......\n"));
    } while (WAIT_TIMEOUT == WaitForSingleObject(GetCurrentThread(),10000));

    MYOVERLAPPED CloseOL = {};
    //��IOCP�̳߳ط����˳���Ϣ ���ñ�־FD_CLOSE��־
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

    //�ͷ�SOCKET��MYOVERLAPPED��Դ
    for(int i = 0; i < iIndex; i ++)
    {
        closesocket(pskConnectArray[i]);
        GRS_SAFEFREE(pMyOLArray[i]);
    }

    _tsystem(_T("PAUSE"));  
    ::WSACleanup();
}

//IOCP�̳߳��̺߳���
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
            GRS_PRINTF(_T("IOCPThread: GetQueuedCompletionStatus ����ʧ��,������: 0x%08x �ڲ�������[0x%08x]\n"),
                GetLastError(), lpOverlapped->Internal);
            continue;
        }

        switch(pMyOl->m_lNetworkEvents)
        {
        case FD_CONNECT:
            {
                //GRS_PRINTF(_T("�߳�[0x%x]��ɲ���(ConnectEx),����(0x%08x)����(%ubytes)\n"),
                //    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                int nRet = ::setsockopt(
                    pMyOl->m_socket, 
                    SOL_SOCKET,
                    SO_UPDATE_CONNECT_CONTEXT,
                    NULL,
                    0
                    );

                //int iBufLen = 0;
                ////�ر��׽����ϵķ��ͻ��壬���������������
                //::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_SNDBUF,(const char*)&iBufLen,sizeof(int));
                //::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_RCVBUF,(const char*)&iBufLen,sizeof(int));

                ////ǿ�Ʒ�����ʱ�㷨�ر�,ֱ�ӷ��͵�������ȥ
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
                //GRS_PRINTF(_T("�߳�[0x%x]��ɲ���(WSASend),����(0x%08x)����(%ubytes)\n"),
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
                //GRS_PRINTF(_T("�߳�[0x%x]��ɲ���(WSARecv),����(0x%08x)����(%ubytes)\n"),
                //    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                GRS_PRINTF(_T("ECHO:%s\n"),(WCHAR*)wba[2].buf);

                //GRS_PRINTFA("ECHO:%s\n",wba[2].buf);

                shutdown(pMyOl->m_socket,SD_BOTH);

                pMyOl->m_lNetworkEvents = FD_CLOSE;
                Sleep(2000);
                //����SOCKET
                g_MsSockFun.DisconnectEx(pMyOl->m_socket,(LPOVERLAPPED)pMyOl,TF_REUSE_SOCKET,0);

            }
            break;
        case FD_CLOSE:
            {
                if( INVALID_SOCKET == pMyOl->m_socket )
                {
                    //GRS_PRINTF(_T("IOCP�߳�[0x%x]�õ��˳�֪ͨ,IOCP�߳��˳�\n"),
                    //    GetCurrentThreadId());

                    bLoop = FALSE;//�˳�ѭ��
                }
                else
                {         
                    //GRS_PRINTF(_T("�߳�[0x%x]��ɲ���(DisconnectEx),����(0x%08x)����(%ubytes)\n"),
                    //    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                    GRS_PRINTF(_T("����SOCKET�ɹ�......\n"));
                    pMyOl->m_lNetworkEvents = FD_CONNECT;
                    pMyOl->m_pBuf           = GRS_CALLOC(sizeof(SOCKADDR_IN));
                    pMyOl->m_szBufLen       = sizeof(SOCKADDR_IN);
                    CopyMemory(pMyOl->m_pBuf,&g_service,sizeof(SOCKADDR_IN));

                    //���ճɹ�,���¶������ӳ�
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
