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

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define SOCK_VERH 2
#define SOCK_VERL 2

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
    //==========================================================================================================
    LPWSAPROTOCOL_INFO pProtocol = NULL;
    DWORD              dwBufLen = 0;
    int                iProtocolCnt = 0;
    SOCKET             skTmp        = INVALID_SOCKET;

    WSAEnumProtocols(0,pProtocol,&dwBufLen);

    pProtocol = (LPWSAPROTOCOL_INFO)GRS_CALLOC(dwBufLen);

    iProtocolCnt = WSAEnumProtocols(0,pProtocol,&dwBufLen);

    for(int i = 0; i < iProtocolCnt;i ++)
    {
        GRS_PRINTF(_T("[%2d] %-40s\n\tЭ������:\n"),i,pProtocol[i].szProtocol);
        if(XP1_CONNECTIONLESS & pProtocol[i].dwServiceFlags1)
        {
            GRS_PRINTF(_T("\t\t������Э��\n"));
        }

        if(XP1_GUARANTEED_DELIVERY & pProtocol[i].dwServiceFlags1)
        {
            GRS_PRINTF(_T("\t\t�ṩ�ɿ�����\n"));
        }

        if(XP1_GUARANTEED_ORDER  & pProtocol[i].dwServiceFlags1)
        {
            GRS_PRINTF(_T("\t\t�ṩ������\n"));
        }

        if(XP1_MESSAGE_ORIENTED  & pProtocol[i].dwServiceFlags1)
        {
            GRS_PRINTF(_T("\t\t������Ϣ\n"));
        }

        if(XP1_PSEUDO_STREAM  & pProtocol[i].dwServiceFlags1)
        {
            GRS_PRINTF(_T("\t\tα��\n"));
        }

        if(XP1_SUPPORT_BROADCAST  & pProtocol[i].dwServiceFlags1)
        {
            GRS_PRINTF(_T("\t\t֧�ֹ㲥\n"));
        }

        if(XP1_SUPPORT_MULTIPOINT  & pProtocol[i].dwServiceFlags1)
        {
            GRS_PRINTF(_T("\t\t֧�ֶಥ(�鲥)\n"));
        }

        if(XP1_QOS_SUPPORTED  & pProtocol[i].dwServiceFlags1)
        {
            GRS_PRINTF(_T("\t\t֧��QoS\n"));
        }

        if(0 == pProtocol[i].iNetworkByteOrder)
        {
            GRS_PRINTF(_T("\t\tBig endian\n"));
        }
        else
        {
            GRS_PRINTF(_T("\t\tLittle endian\n"));
        }

        if(0 != pProtocol[i].dwMessageSize && 0x1 != pProtocol[i].dwMessageSize && 0xFFFFFFFF != pProtocol[i].dwMessageSize )
        {
            GRS_PRINTF(_T("\t\t�����Ϣ�ֽ���:%u Bytes\n"),pProtocol[i].dwMessageSize);
        }

        //ʹ��WSASocket������SOCKET
        skTmp = WSASocket(0,0,0,&pProtocol[i],NULL,0);
        closesocket(skTmp);
        skTmp = INVALID_SOCKET;
        //ʹ��socket������
        skTmp = socket(pProtocol[i].iAddressFamily,pProtocol[i].iSocketType,pProtocol[i].iProtocol);
        closesocket(skTmp);
        skTmp = INVALID_SOCKET;
    }
    _tsystem(_T("PAUSE"));
    GRS_SAFEFREE(pProtocol);
    //==========================================================================================================
    ::WSACleanup();
    return 0;
}