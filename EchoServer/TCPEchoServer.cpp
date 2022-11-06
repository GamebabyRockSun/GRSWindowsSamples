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

    
    if(0 != bind(skServer,(SOCKADDR*)&sa,sizeof(SOCKADDR_IN)))
    {
        GRS_PRINTF(_T("�󶨵�ָ����ַ�˿ڳ���!\n"));
        goto CLEAN_UP;
    }

    if(0 != listen(skServer,SOMAXCONN))
    {
        GRS_PRINTF(_T("SOCKET�������ģʽ����!\n"));
        goto CLEAN_UP;
    }

    while(INVALID_SOCKET != (skClient = accept(skServer,(SOCKADDR*)&saClient,&isaLen)))
    {
        GRS_PRINTFA("�ͻ���[%s : %u]���ӽ���......\n"
            ,inet_ntoa(saClient.sin_addr),ntohs(saClient.sin_port));

        pskBuf = GRS_CALLOC(iBufLen);
        pskBufTmp = pskBuf;
        GRS_PRINTF(_T("\t��ʼ���տͻ�������......\n"));
        do 
        {
            iRecv = recv(skClient,(char*)pskBufTmp,iBufLen - iBufLenTmp,0);
            if( SOCKET_ERROR == iRecv || 0 == iRecv ) 
            {
                break;
            }

            if( iRecv >= (iBufLen - iBufLenTmp) )
            {//��ʵֻ���ܳ���==�����
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
            GRS_PRINTF(_T("\t���տͻ������ݳɹ�,��ʼ���ط�������......\n"));
            send(skClient,(const char*)pskBuf,iBufLen,0);
        }
        else
        {
            GRS_PRINTF(_T("\t���տͻ�������ʧ��,�ر�����......\n"))
        }

        GRS_SAFEFREE(pskBuf);
        pskBufTmp = NULL;
        iBufLenTmp = 0;
        iBufLen = 1024;
        
        GRS_PRINTF(_T("\t��ʼ�ر�����.....\n"));
        shutdown(skClient,SD_BOTH);
        closesocket(skClient);
        GRS_PRINTF(_T("\t�ر��������!\n"));
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