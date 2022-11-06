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
        GRS_PRINTF(_T("�޷���ʼ��Socket2ϵͳ������������Ϊ��%d��\n"),WSAGetLastError());
        return 1;
    }
    if ( LOBYTE( wd.wVersion ) != SOCK_VERH ||
        HIBYTE( wd.wVersion ) != SOCK_VERL ) 
    {
        GRS_PRINTF(_T("�޷���ʼ��%d.%d�汾��Socket������\n"),SOCK_VERH,SOCK_VERL);
        WSACleanup( );
        return 2; 
    }
    GRS_PRINTF(_T("Winsock���ʼ���ɹ�!\n\t��ǰϵͳ��֧����ߵ�Winsock�汾Ϊ%d.%d\n\t��ǰӦ�ü��صİ汾Ϊ%d.%d\n")
        ,LOBYTE(wd.wHighVersion),HIBYTE(wd.wHighVersion)
        ,LOBYTE(wd.wVersion),HIBYTE(wd.wVersion));
    //==========================================================================================================
    //���������Winsock�ĵ���
    SOCKET skClient         = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    SOCKADDR_IN sa          = {AF_INET};
    sa.sin_addr.s_addr      = inet_addr("127.0.0.1");//�ػ���ַ
    sa.sin_port             = htons(GRS_SERVER_PORT);

    ULONG nMsgType          = 0x1;
    DWORD dwMsgLen          = 0;
    WCHAR pszSendMsg[]      = _T("Hello Internet!");
    size_t szLen = 0;
    StringCchLength(pszSendMsg,3000,&szLen);
    ULONG nMsgTail          = 0x7FFF;

    DWORD dwTransBytes      = 0;
    DWORD dwFlags           = 0;

    WSABUF  wba[5]  = {};

    WCHAR pszRecvBuf[1024]  = {};

    if(0 != connect(skClient,(SOCKADDR*)&sa,sizeof(SOCKADDR_IN)))
    {
        GRS_PRINTF(_T("���ӷ���������!\n"));
        goto CLEAN_UP;
    }
    
    //���Զ���Э����
    wba[0].buf = (CHAR*)&nMsgType;
    wba[0].len = sizeof(ULONG);

    dwMsgLen = (szLen + 1) * sizeof(WCHAR);
    wba[1].buf = (CHAR*)&dwMsgLen;
    wba[1].len = sizeof(size_t);

    wba[2].buf = (CHAR*)pszSendMsg;
    wba[2].len = (szLen + 1) * sizeof(WCHAR);

    wba[3].buf = (CHAR*)&nMsgTail;
    wba[3].len = sizeof(nMsgTail);

    WSASend(skClient,wba,4,&dwTransBytes,0,NULL,NULL);

    nMsgType = 0;
    szLen = 0;
    nMsgTail = 0;

    
    WSARecv(skClient,wba,2,&dwTransBytes,&dwFlags,NULL,NULL);

    if( 0x1 != nMsgType )
    {
        GRS_PRINTF(_T("����˷�����Ϣ����,��Ϣ���Ͳ���ȷ!\n"));
        goto CLEAN_UP;
    }

    wba[2].buf = (CHAR*)pszRecvBuf;
    wba[2].len = dwMsgLen;
    
    WSARecv(skClient,&wba[2],2,&dwTransBytes,&dwFlags,NULL,NULL);

    if( 0x7FFF != nMsgTail )
    {
        GRS_PRINTF(_T("����˷�����Ϣ����,��β���Ͳ���ȷ!\n"));
        goto CLEAN_UP;
    }

    GRS_PRINTF(_T("ECHO:%s\n"),pszRecvBuf);
    //==========================================================================================================
CLEAN_UP:
    _tsystem(_T("PAUSE"));
    if(INVALID_SOCKET != skClient)
    {
        shutdown(skClient,SD_BOTH);
        closesocket(skClient);
    }    
    ::WSACleanup();
    return 0;
}