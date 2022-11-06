#include <tchar.h>
#define WIN32_LEAN_AND_MEAN	
#include <windows.h>
#include <strsafe.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>
#include "../IPHeader.h"
#include <AtlConv.h> //for T2A

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


addrinfo* GetAddrInfo(char *addr, char *port, int af, int type, int proto)
{
    GRS_USEPRINTF();
    addrinfo hints = {};
    addrinfo *res = NULL;
    int      rc = 0;

    hints.ai_flags      = ((addr) ? 0 : AI_PASSIVE);
    hints.ai_family     = af;
    hints.ai_socktype   = type;
    hints.ai_protocol   = proto;

    rc = getaddrinfo(
        addr,
        port,
        &hints,
        &res
        );
    if (rc != 0)
    {
        GRS_PRINTF(_T("无法取得地址 %s 的信息,错误码: %d\n"), addr, rc);
        return NULL;
    }
    return res;
}

int PrintAddress(SOCKADDR *sa, int salen)
{
    GRS_USEPRINTF();
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    int  hostlen = NI_MAXHOST;
    int servlen = NI_MAXSERV;
    int rc = 0;

    rc = getnameinfo(sa,salen,
        host,hostlen,
        serv,servlen,
        NI_NUMERICHOST | NI_NUMERICSERV
        );

    if (rc != 0)
    {
        GRS_PRINTF(_T("getnameinfo 调用失败,错误码: %d\n"), rc);
        return rc;
    }

    if ( strcmp(serv, "0") != 0 )
    {
        if (sa->sa_family == AF_INET)
        {
            GRS_PRINTFA("[%s]:%s", host, serv);
        }
        else
        {
            GRS_PRINTFA("%s:%s", host, serv);
        }
    }
    else
    {
        GRS_PRINTFA("%s", host);
    }

    return NO_ERROR;
}

USHORT CheckSum(USHORT *buffer, int size) 
{//计算校验和的方法
    unsigned long cksum=0;

    while (size > 1) 
    {
        cksum += *buffer++;
        size -= sizeof(USHORT);
    }
    if (size) 
    {
        cksum += *(UCHAR*)buffer;
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >>16);
    return (USHORT)(~cksum);
}

int _tmain(int argc,TCHAR** argv)
{
    USES_CONVERSION;
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
    if(argc < 2)
    {
        GRS_PRINTF(_T("usage: ping <host>\n"));
        GRS_PRINTF(_T("        host        Remote machine to ping\n"));
        _tsystem(_T("PAUSE"));
        return -1;
    }
    
    WSAOVERLAPPED ol        = {};
    int iPingCnt            = 10;
    int ttl                 = 128;
    addrinfo   *dest        = NULL;
    addrinfo   *local       = NULL;
    int iBufLen             = sizeof(ST_ICMP_HDR) + 32;
    BYTE pSendBuf[0xFFFF]   = {};
    ST_ICMP_HDR* pICMPHdr   = (ST_ICMP_HDR*)pSendBuf;
    BYTE pRecvBuf[0xFFFF]   = {};
    ST_IPV4_HDR* pIPHdr     = (ST_IPV4_HDR*)pRecvBuf;
    WSABUF wbRecv           = {0xFFFF,(char*)pRecvBuf};
    WSABUF wbSend           = {iBufLen,(char*)pSendBuf};
    DWORD dwRecv            = 0;
    DWORD dwFlags           = 0;
    SOCKADDR_IN saFrom      = {};
    int iFromLen            = sizeof(SOCKADDR_IN);
    DWORD dwRet             = 0;
    DWORD dwTime            = 0;

    dest    = GetAddrInfo(T2A(argv[1]),"0",AF_INET,0,0);
    if(NULL == dest)
    {
        _tsystem(_T("PAUSE"));
        return -1;
    }

    local   = GetAddrInfo(NULL,"0",AF_INET,0,0);
    if(NULL == local)
    {
        _tsystem(_T("PAUSE"));
        return -1;
    }



    SOCKET skICMP = WSASocket(AF_INET,SOCK_RAW,IPPROTO_ICMP,NULL,0,WSA_FLAG_OVERLAPPED);
    bind(skICMP,local->ai_addr,local->ai_addrlen);
    
    int rc = setsockopt(skICMP,IPPROTO_IP,IP_TTL,(char *)&ttl,sizeof(ttl));
    if (rc == SOCKET_ERROR)
    {
        GRS_PRINTF(_T("设置TTL失败,错误码: %d\n"), WSAGetLastError());
        closesocket(skICMP);
        freeaddrinfo(dest);
        freeaddrinfo(local);
        _tsystem(_T("PAUSE"));
        return -1;
    }

    pICMPHdr->icmp_type     = ICMPV4_ECHO_REQUEST_TYPE;        //Ping Type
    pICMPHdr->icmp_code     = ICMPV4_ECHO_REQUEST_CODE;
    pICMPHdr->icmp_id       = (USHORT)GetCurrentProcessId();
    pICMPHdr->icmp_checksum = 0;
    pICMPHdr->icmp_sequence = (USHORT)GetTickCount();
    pICMPHdr->icmp_timestamp= GetTickCount();

    //将ICMP包中的剩余的数据部分统一设置成一个随机值,这里用的是0x32
    memset(pSendBuf + sizeof(ST_ICMP_HDR),0x32,iBufLen - sizeof(ST_ICMP_HDR));
    ol.hEvent = WSACreateEvent();

    GRS_PRINTF(_T("%s %s\n"),argv[0],argv[1]);

    for(int i = 0; i < iPingCnt; i ++)
    {
        iFromLen = sizeof(SOCKADDR_IN);
        dwRecv = 0;
        dwFlags = 0;
        memset(pRecvBuf,0,0xFFFF);
        memset(&saFrom,0,iFromLen);
        //先发出一个接收调用等待接收数据,这样最终计算出的时间比较准确
        rc = WSARecvFrom(skICMP,&wbRecv,1,&dwRecv,&dwFlags,(SOCKADDR*)&saFrom,&iFromLen,&ol,NULL);
        if (rc == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                GRS_PRINTF(_T("WSARecvFrom 调用失败,错误码: %d\n"), WSAGetLastError());
                _tsystem(_T("PAUSE"));
                return -1;
            }
        }

        //pICMPHdr = (ST_ICMP_HDR*)pSendBuf;
        ++ pICMPHdr->icmp_sequence;  //序号自动加1
        pICMPHdr->icmp_timestamp    = GetTickCount(); //记录个时间戳
        pICMPHdr->icmp_checksum     = 0;              //一定要先清零再计算
        pICMPHdr->icmp_checksum     = CheckSum((USHORT*)pSendBuf,iBufLen);
        
        //USHORT nCheckSum = CheckSum((USHORT*)pSendBuf,iBufLen);
        //dwTime = GetTickCount();
        WSASendTo(skICMP,&wbSend,1,&dwRecv,0,dest->ai_addr,dest->ai_addrlen,NULL,NULL);
        
        dwRet = WaitForSingleObject((HANDLE)ol.hEvent,6000);
        
        if (WAIT_FAILED == dwRet)
        {
            GRS_PRINTF(_T("WaitForSingleObject 等待接收数据失败,错误码: %d\n"), GetLastError());
            _tsystem(_T("PAUSE"));
            return -1;
        }
        else if (WAIT_TIMEOUT  == dwRet)
        {
            GRS_PRINTF(_T("Request timed out.\n"));
        }
        else
        {
            dwTime = GetTickCount() - pICMPHdr->icmp_timestamp;

            rc = WSAGetOverlappedResult(skICMP,&ol,&dwRecv,FALSE,&dwFlags);
            if ( rc == FALSE )
            {
                GRS_PRINTF(_T("WSAGetOverlappedResult 获取重叠IO完成信息失败,错误码: %d\n"), WSAGetLastError());
            }

            WSAResetEvent(ol.hEvent);

            GRS_PRINTF(_T("Reply from "));

            PrintAddress((SOCKADDR *)&saFrom, iFromLen);
            
            if ( dwTime <= 0 )
            {
                GRS_PRINTF(_T(": bytes=%d time<1ms TTL=%d\n")
                    , iBufLen - sizeof(ST_ICMP_HDR),pIPHdr->ip_ttl);
            }
            else
            {
                GRS_PRINTF(_T(": bytes=%d time=%dms TTL=%d\n")
                    , iBufLen - sizeof(ST_ICMP_HDR), dwTime, pIPHdr->ip_ttl);
            }
        }  
        Sleep(1000);
    }
    


    //==========================================================================================================
    _tsystem(_T("PAUSE"));  
    freeaddrinfo(dest);
    freeaddrinfo(local);
    ::WSACleanup();
    return 0;
}
