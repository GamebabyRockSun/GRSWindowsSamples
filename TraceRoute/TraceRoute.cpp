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

int ReverseLookup(SOCKADDR *sa, int salen, char *buf, int buflen)
{
    GRS_USEPRINTF();
    char    host[NI_MAXHOST] = {};
    int     hostlen = NI_MAXHOST,
        rc;

    rc = getnameinfo(
        sa,
        salen,
        host,
        hostlen,
        NULL,
        0,
        0
        );
    if (rc != 0)
    {
        GRS_PRINTF(_T("getnameinfo 调用失败,错误码: %d\n"), rc);
        return rc;
    }

    StringCchCopyA(buf,buflen,host);

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
        GRS_PRINTF(_T("usage: TraceRoute <host>\n"));
        GRS_PRINTF(_T("        host        Remote machine to TraceRoute\n"));
        _tsystem(_T("PAUSE"));
        return -1;
    }

    WSAOVERLAPPED ol        = {};
    int iHopCnt             = 128;
    int ttl                 = 128;
    int iRouteCnt           = 0;
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
    SOCKADDR_IN saPrev      = {};   //上一跳的地址
    DWORD dwRet             = 0;
    DWORD dwTime            = 0;
    int   iIPHdrLen         = 0;
    char  szHopName[512]    = {};
    int   iHopNameLen       = 512;

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

    //初始化ICMP报文头
    pICMPHdr->icmp_type     = ICMPV4_ECHO_REQUEST_TYPE;        //Ping Type
    pICMPHdr->icmp_code     = ICMPV4_ECHO_REQUEST_CODE;
    pICMPHdr->icmp_id       = (USHORT)GetCurrentProcessId();
    pICMPHdr->icmp_checksum = 0;
    pICMPHdr->icmp_sequence = (USHORT)GetTickCount();
    pICMPHdr->icmp_timestamp= GetTickCount();


    //将ICMP包中的剩余的数据部分统一设置成一个随机值,这里用的是0x32
    memset(pSendBuf + sizeof(ST_ICMP_HDR),0x32,iBufLen - sizeof(ST_ICMP_HDR));
    ol.hEvent = WSACreateEvent();

    GRS_PRINTF(_T("TraceRoute %s ["),argv[1]);
    PrintAddress(dest->ai_addr, dest->ai_addrlen);
    GRS_PRINTF(_T("]\n"));

    //设置TTL等于1 注意设置为0 将包括自己的IP在列表中作为起跳节点
    ttl = 1; //ttl 从1开始起跳
    int rc = setsockopt(skICMP,IPPROTO_IP,IP_TTL,(char *)&ttl,sizeof(ttl));
    if (rc == SOCKET_ERROR)
    {
        GRS_PRINTF(_T("设置TTL失败,错误码: %d\n"), WSAGetLastError());
        _tsystem(_T("PAUSE"));
        return -1;
    }

    for(int i = 0; i < iHopCnt; i ++)
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

        pICMPHdr = (ST_ICMP_HDR*)pSendBuf;
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

            iIPHdrLen = pIPHdr->ip_hdrlen * 4;

            if( IPPROTO_ICMP == pIPHdr->ip_protocol )
            {
                pICMPHdr = (ST_ICMP_HDR*)&pRecvBuf[iIPHdrLen];
                if ((pICMPHdr->icmp_type != ICMPV4_TIMEOUT) &&
                    (pICMPHdr->icmp_type != ICMPV4_ECHO_REPLY_TYPE) &&
                    (pICMPHdr->icmp_code != ICMPV4_ECHO_REPLY_CODE) )
                {
                    //GRS_PRINTF(_T("接收到ICMP 消息类型 %d 而不是TTL过期消息!\n"), pICMPHdr->icmp_type);
                }
                else
                {

                    if( saPrev.sin_addr.s_addr != saFrom.sin_addr.s_addr )
                    {
                        ++ iRouteCnt;
                        GRS_PRINTFA("%d   %d ms", iRouteCnt, dwTime);
                        ReverseLookup((SOCKADDR *)&saFrom, iFromLen, szHopName, iHopNameLen);
                        GRS_PRINTFA("   %s [",szHopName);
                        PrintAddress((SOCKADDR *)&saFrom, iFromLen);
                        GRS_PRINTFA("]\n");
                        
                        saPrev.sin_addr.s_addr = saFrom.sin_addr.s_addr;
                    }

                    //不解析地址 直接跳到下一个节点
                    //GRS_PRINTFA("%d   %d ms  ", ttl, dwTime);
                    //PrintAddress((SOCKADDR *)&saFrom, iFromLen);
                    //GRS_PRINTFA("\n");

                    //TTL累加,发向下一个路由
                    ++ttl;
                }

                if( ((SOCKADDR_IN*)dest->ai_addr)->sin_addr.s_addr == saFrom.sin_addr.s_addr )
                {//已经抵达目标地址,不用再探测路由
                    break;
                }
            }   
            else
            {
                GRS_PRINTF(_T("TraceRoute失败,收到非ICMP 报文 %d,探测终止!\n"),pIPHdr->ip_protocol);
                break;
            }
        }  

      
        rc = setsockopt(skICMP,IPPROTO_IP,IP_TTL,(char *)&ttl,sizeof(ttl));
        if (rc == SOCKET_ERROR)
        {
            GRS_PRINTF(_T("设置TTL失败,错误码: %d\n"), WSAGetLastError());
            _tsystem(_T("PAUSE"));
            return -1;
        }

        //Sleep(1000);
    }

    GRS_PRINTF(_T("追踪结束,共扫描到%d个路由节点\n"),iRouteCnt);


    //==========================================================================================================
    _tsystem(_T("PAUSE"));  
    freeaddrinfo(dest);
    freeaddrinfo(local);
    ::WSACleanup();
    return 0;
}
