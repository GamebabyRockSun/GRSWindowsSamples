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
    //����IOCP�ں˶���,������󲢷�CPU�������߳�
    hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,NULL,si.dwNumberOfProcessors);

    //ʵ�ʴ���2��CPU�������߳�
    phIocpThread = (HANDLE*)GRS_CALLOC(2 * si.dwNumberOfProcessors * sizeof(HANDLE));
    for(DWORD i = 0; i < 2 * si.dwNumberOfProcessors; i ++)
    {
        phIocpThread[i] = GRS_BEGINTHREAD(IOCPThread,hIocp);
    }

    ListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED);

    //��SOCKET�������ɶ˿ڶ����
    //ע��������׽���һ��Ҫ�Ⱥ�IOCP��,����AcceptEx���޷�����IOCP����
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
    
    //����AcceptEx����
    for(int i = 0; i < iMaxAcceptEx; i ++)
    {
        AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED);
        //��SOCKET�������ɶ˿ڶ����
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
                GRS_PRINTF(_T("����AcceptExʧ��,������:%d\n"),iError);
                continue;
            }

        }

        pskAcceptArray[iIndex] = AcceptSocket;
        pMyOLArray[iIndex]  = pOLAcceptEx;
        ++ iIndex;
        GRS_PRINTF(_T("AcceptEx(%d)...\n"),i);
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

    //��IOCP�̳߳ط����˳���Ϣ ���ñ�־FD_CLOSE��־
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

    //�ͷ�SOCKET��MYOVERLAPPED��Դ
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
        case FD_ACCEPT:
            {
                GRS_PRINTF(_T("�߳�[0x%x]��ɲ���(AcceptEx),����(0x%08x)����(%ubytes)\n"),
                    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                SOCKADDR_IN* psaLocal = NULL;
                int         iLocalLen = sizeof(SOCKADDR_IN);
                SOCKADDR_IN* psaRemote = NULL;
                int         iRemoteLen = sizeof(SOCKADDR_IN);

                g_MsSockFun.GetAcceptExSockaddrs(pMyOl->m_pBuf,0
                    ,sizeof(SOCKADDR_IN) + 16,sizeof(SOCKADDR_IN) + 16
                    ,(SOCKADDR**)&psaLocal,&iLocalLen,(SOCKADDR**)&psaRemote,&iRemoteLen);
                
                GRS_PRINTFA("�ͻ���[%s:%d]���ӽ���,����ͨѶ��ַ[%s:%d]\n"
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
                //�ر��׽����ϵķ��ͻ��壬���������������
                ::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_SNDBUF,(const char*)&iBufLen,sizeof(int));
                ::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_RCVBUF,(const char*)&iBufLen,sizeof(int));
                
                //ǿ�Ʒ�����ʱ�㷨�ر�,ֱ�ӷ��͵�������ȥ
                DWORD dwNo = 0;
                ::setsockopt(pMyOl->m_socket,IPPROTO_TCP,TCP_NODELAY,(char*)&dwNo,sizeof(DWORD));

                BOOL bDontLinger = FALSE; 
                ::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_DONTLINGER,(const char*)&bDontLinger,sizeof(BOOL));

                linger sLinger = {};
                sLinger.l_onoff = 1;
                sLinger.l_linger = 0;
                ::setsockopt(pMyOl->m_socket,SOL_SOCKET,SO_LINGER,(const char*)&sLinger,sizeof(linger));

                WSABUF wsabuf = {0,NULL};
                //��������Ҫ��Ϣͨ����չOVERLAPPED�Ľṹ�����ص�������ȥ
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
                GRS_PRINTF(_T("�߳�[0x%x]��ɲ���(WSASend),����(0x%08x)����(%ubytes)\n"),
                    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                GRS_SAFEFREE(pMyOl->m_pBuf);

                shutdown(pMyOl->m_socket,SD_BOTH);
                
                pMyOl->m_lNetworkEvents = FD_CLOSE;
                //����SOCKET
                g_MsSockFun.DisconnectEx(pMyOl->m_socket,(LPOVERLAPPED)pMyOl,TF_REUSE_SOCKET,0);
            }
            break;
        case FD_READ:
            {
                GRS_PRINTF(_T("�߳�[0x%x]��ɲ���(WSARecv),����(0x%08x)����(%ubytes)\n"),
                    GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                GRS_PRINTF(_T("��ʼ��������......\n"));
                //ִ��Non-Blocking����
                DWORD dwMsgType = 0;
                DWORD dwMsgLen  = 0;
                DWORD dwTransBytes = 0;
                DWORD dwMsgTail = 0;
                
                //���հ�ͷ ������Ϣ
                WSABUF wsabuf[5] = {sizeof(DWORD),(CHAR*)&dwMsgType};
                //���հ�ͷ ������Ϣ
                wsabuf[1].buf = (CHAR*)&dwMsgLen;
                wsabuf[1].len = sizeof(DWORD);

                WSARecv(pMyOl->m_socket,&wsabuf[0],2,&dwTransBytes,&dwFlags,NULL,NULL);
                                
                //�ж��Ƿ���ָ������Ϣ����
                if(0x1 != dwMsgType)
                {//����ָ������Ϣ����,����SOCKET
                    GRS_PRINTF(_T("���յ����ݰ�ͷ��Ϣ���ʹ���,����SOCKET......\n"))
                    shutdown(pMyOl->m_socket,SD_BOTH);
                    pMyOl->m_lNetworkEvents = FD_CLOSE;
                    g_MsSockFun.DisconnectEx(pMyOl->m_socket,(LPOVERLAPPED)pMyOl,TF_REUSE_SOCKET,0);
                    break;
                }

                //�ж��������Ƿ����
                if( dwMsgLen < 1 || dwMsgLen >= 0xFFFF )
                {//���Ȳ�����,����SOCKET
                    GRS_PRINTF(_T("���յ����ݰ�ͷ��Ϣ���ȴ���,����SOCKET......\n"))
                    shutdown(pMyOl->m_socket,SD_BOTH);
                    pMyOl->m_lNetworkEvents = FD_CLOSE;
                    g_MsSockFun.DisconnectEx(pMyOl->m_socket,(LPOVERLAPPED)pMyOl,TF_REUSE_SOCKET,0);
                    break;
                }

                //���հ�����
                wsabuf[2].buf = (CHAR*)GRS_CALLOC(dwMsgLen);
                wsabuf[2].len = dwMsgLen;
                //���հ�β
                wsabuf[3].buf = (CHAR*)&dwMsgTail;
                wsabuf[3].len = sizeof(DWORD);

                WSARecv(pMyOl->m_socket,&wsabuf[2],2,&dwTransBytes,&dwFlags,NULL,NULL);

                //�ж��°�β �ض��İ�β����0x7FFF
                if( 0x7FFF!= dwMsgTail )
                {
                    GRS_PRINTF(_T("���յ���β��Ϣ�쳣......\n"));
                }

                GRS_PRINTF(_T("�������ݳɹ�,���ͷ���......\n"));
                
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
                        GRS_PRINTF(_T("��������ʧ��,����SOCKET���......\n"));
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
                    GRS_PRINTF(_T("IOCP�߳�[0x%x]�õ��˳�֪ͨ,IOCP�߳��˳�\n"),
                        GetCurrentThreadId());

                    bLoop = FALSE;//�˳�ѭ��
                }
                else
                {         
                    GRS_PRINTF(_T("�߳�[0x%x]��ɲ���(DisconnectEx),����(0x%08x)����(%ubytes)\n"),
                        GetCurrentThreadId(),pMyOl->m_pBuf,pMyOl->m_szBufLen);

                    GRS_PRINTF(_T("����SOCKET�ɹ�,����AcceptEx......\n"));

                    pMyOl->m_lNetworkEvents = FD_ACCEPT;
                    pMyOl->m_pBuf           = GRS_CALLOC(2 * (sizeof(SOCKADDR_IN) + 16));
                    pMyOl->m_szBufLen       = 2 * (sizeof(SOCKADDR_IN) + 16);

                    //���ճɹ�,���¶������ӳ�
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
