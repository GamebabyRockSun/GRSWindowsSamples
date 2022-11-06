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

#define DATA_BUFSIZE 4096


struct MYOVERLAPPED 
{
    WSAOVERLAPPED m_wsaol;
    long          m_lNetworkEvents;//Ͷ�ݵĲ�������(FD_READ/FD_WRITE��)
    SOCKET        m_socket;        //Ͷ�ݲ�����SOCKET���
    void*         m_pBuf;          //Ͷ�ݲ���ʱ�Ļ���
    size_t        m_szBufLen;      //���峤��
    DWORD		  m_dwTrasBytes;   //ΪWSASent��WSARecv׼���Ĳ���
    DWORD         m_dwFlags;       //ΪWSARecv׼����
};


void CALLBACK CompletionROUTINE(
                                IN DWORD dwError, 
                                IN DWORD cbTransferred, 
                                IN LPWSAOVERLAPPED lpOverlapped, 
                                IN DWORD dwFlags
                                );




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

    WSABUF DataBuf = {};
    BYTE buffer[DATA_BUFSIZE] = {};

    WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS] = {};
    MYOVERLAPPED AcceptOverlapped = {};
    SOCKET ListenSocket, AcceptSocket;

    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    u_short port = 8080;
    char* ip;
    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_port = htons(port);

    hostent* thisHost;
    thisHost = gethostbyname("");
    ip = inet_ntoa (*(struct in_addr *)*thisHost->h_addr_list);

    service.sin_addr.s_addr = INADDR_ANY;//inet_addr(ip);

    bind(ListenSocket, (SOCKADDR *) &service, sizeof(SOCKADDR));

    listen(ListenSocket, 1);
    GRS_PRINTF(_T("Listening...\n"));

    AcceptSocket = accept(ListenSocket, NULL, NULL);
    GRS_PRINTF(_T("Client Accepted...\n"));

    DataBuf.len = DATA_BUFSIZE;
    DataBuf.buf = (CHAR*)buffer;

    //��������Ҫ��Ϣͨ����չOVERLAPPED�Ľṹ�����ص�������ȥ
    AcceptOverlapped.m_lNetworkEvents = FD_READ;
    AcceptOverlapped.m_pBuf = buffer;
    AcceptOverlapped.m_szBufLen = DATA_BUFSIZE;
    AcceptOverlapped.m_socket = AcceptSocket;
    

    if (WSARecv(AcceptSocket, &DataBuf, 1, &AcceptOverlapped.m_dwTrasBytes
        , &AcceptOverlapped.m_dwFlags, (WSAOVERLAPPED*)&AcceptOverlapped.m_wsaol, CompletionROUTINE) == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            GRS_PRINTF(_T("Error occurred at WSARecv()\n"));
        }
    }
//..................
    //���߳̽���ɾ���״̬
    do 
    {
        GRS_PRINTF(_T("���߳̽���ɾ���״̬......\n"));
    } while (WAIT_IO_COMPLETION  == SleepEx(INFINITE,TRUE));
   
    _tsystem(_T("PAUSE"));
    if(INVALID_SOCKET != ListenSocket)
    {
        closesocket(ListenSocket);
    }    
    ::WSACleanup();
}


void CALLBACK CompletionROUTINE(DWORD dwError,DWORD cbTransferred,LPWSAOVERLAPPED lpOverlapped,DWORD dwFlags)
{
    GRS_USEPRINTF();
    //ʹ�ú�CONTAINING_RECORD�õ���ȷ��MYOVERLAPPEDָ��
    //���ں�CONTAINING_RECORD�÷����������һ���ֵ�ʮ�Ľ�
    MYOVERLAPPED* pMyOl = CONTAINING_RECORD(lpOverlapped,MYOVERLAPPED,m_wsaol);

    if(0 != dwError || 0 == cbTransferred)
    {
        GRS_PRINTF(_T("��������ʧ��,������:0x%08X\n"),dwError);
        closesocket(pMyOl->m_socket);
        return;
    }
    
    if( FD_READ == pMyOl->m_lNetworkEvents )
    {
        GRS_PRINTF(_T("�������ݳɹ�,���ͷ���......\n"));
        pMyOl->m_lNetworkEvents = FD_WRITE;
        pMyOl->m_dwFlags = 0;
        WSABUF wsabuf = {cbTransferred,(CHAR*)pMyOl->m_pBuf};

        if(SOCKET_ERROR == WSASend(pMyOl->m_socket,&wsabuf,1,&pMyOl->m_dwTrasBytes,0
            ,(LPWSAOVERLAPPED)&pMyOl->m_wsaol,CompletionROUTINE))
        {
            int iErrorCode = WSAGetLastError();
            if (iErrorCode != WSA_IO_PENDING)
            {
                GRS_PRINTF(_T("Error occurred at WSASend(),Error Code:0x%08X\n"),iErrorCode);
                GRS_PRINTF(_T("��������ʧ��,�ر�SOCKET���......\n"));
                closesocket(pMyOl->m_socket);
            }
            
        }

        return;
    }

    if( FD_WRITE == pMyOl->m_lNetworkEvents )
    {
        GRS_PRINTF(_T("�������ݳɹ�,�ر�SOCKET���......\n"));
        closesocket(pMyOl->m_socket);
    }
}