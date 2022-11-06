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

#define DEFAULT_MTU           1496          // default MTU size
#define DEFAULT_TTL           8             // default TTL value

#define MAX_PACKET            65535         // maximum datagram size
#define MAX_PACKET_FRAGMENTS ((MAX_PACKET / DEFAULT_MTU) + 1)

#define DEFAULT_PORT          5150          // default port to send to
#define DEFAULT_COUNT         5             // default number of messages to send
#define DEFAULT_MESSAGE       "This is a test"  // default message

#define FRAGMENT_HEADER_PROTOCOL    44      // protocol value for IPv6 fragmentation header

char      gDestAddress[512]     = {};             // IP address to send to
char      gDestPort[512]        = {};             // port to send to
char      gMessage[MAX_PACKET]  = {};             // Message to send as UDP payload
int       gSendSize             = 0;              // Data size of message to send

void usage(TCHAR *progname)
{
    GRS_USEPRINTF();
    GRS_PRINTF(_T("usage: %s \n")
        _T("    -da addr   To (recipient) port number\n")
        _T("    -dp int    To (recipient) IP address\n")
        _T("    -m  str    String message to fill packet data with\n")
        _T("    -z  int    Size of message to send\n"),
        progname
        );
    _tsystem(_T("PAUSE"));
    ExitProcess(1);
}

//
// Function: ValidateArgs
//
// Description:
//    Parse the command line arguments and set some global flags to
//    indicate what actions to perform.
//
void ValidateArgs(int argc, TCHAR **argv)
{
    USES_CONVERSION;
    int                i;

    StringCchCopyA(gMessage,MAX_PACKET, DEFAULT_MESSAGE);
    for(i=1; i < argc ;i++)
    {
        if ((argv[i][0] == '-') || (argv[i][0] == '/'))
        {
            switch (tolower(argv[i][1]))
            {
            case 'd':                   // destination address
                if (i+1 > argc)
                {
                    usage(argv[0]);
                }

                if (tolower(argv[i][2]) == 'a')
                {
                    ++i;
                    StringCchCopyA(gDestAddress,512,T2A(argv[i]));

                }
                else if (tolower(argv[i][2]) == 'p')
                {
                    ++i;
                    StringCchCopyA(gDestPort,512,T2A(argv[i]));
                }
                else
                {
                    usage(argv[0]);
                    break;
                }
                break;
            case 'm':
                if (i+1 >= argc)
                {
                    usage(argv[0]);
                }
                ++i;
                StringCchCopyA(gMessage,MAX_PACKET,T2A(argv[i]));
                break;
            case 'z':                   // Send size
                if (i+1 >= argc)
                {
                    usage(argv[0]);
                }
                ++i;
                gSendSize = _ttoi(argv[i]);
                break;
            default:
                usage(argv[0]);
                break;
            }
        }
    }

    // If no data size was given, initialize it to the message supplied
    if (gSendSize == 0)
    {
        gSendSize = strlen(gMessage);
    }

    return;
}

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

// 
// Function: checksum
//
// Description:
//    This function calculates the 16-bit one's complement sum
//    for the supplied buffer.
//
USHORT checksum(USHORT *buffer, int size)
{
    unsigned long cksum=0;

    while (size > 1)
    {
        cksum += *buffer++;
        size  -= sizeof(USHORT);   
    }
    if (size)
    {
        cksum += *(UCHAR*)buffer;   
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >>16); 

    return (USHORT)(~cksum); 
}

//
// Function: InitIpv4Header
//
// Description:
//    Initialize the IPv4 header with the version, header length,
//    total length, ttl, protocol value, and source and destination
//    addresses.
//
int InitIpv4Header(
                   char *buf, 
                   SOCKADDR *src, 
                   SOCKADDR *dest, 
                   int ttl,
                   int proto,
                   int payloadlen
                   )
{
    ST_IPV4_HDR* v4hdr=NULL;

    v4hdr = (ST_IPV4_HDR *)buf;

    v4hdr->ip_ver               = 0x4;
    v4hdr->ip_hdrlen            = (sizeof(ST_IPV4_HDR) / sizeof(unsigned long));
    v4hdr->ip_tos               = 0;
    v4hdr->ip_totallength       = htons(sizeof(ST_IPV4_HDR) + sizeof(ST_UDP_HDR) + payloadlen);
    v4hdr->ip_id                = 0;
    v4hdr->ip_offset            = 0;
    v4hdr->ip_ttl               = (unsigned char)ttl;
    v4hdr->ip_protocol          = (unsigned char)proto;
    v4hdr->ip_checksum          = 0;
    v4hdr->ip_srcaddr.s_addr    = ((SOCKADDR_IN *)src)->sin_addr.s_addr;
    v4hdr->ip_destaddr.s_addr   = ((SOCKADDR_IN *)dest)->sin_addr.s_addr;

    v4hdr->ip_checksum    = checksum((unsigned short *)v4hdr, sizeof(ST_IPV4_HDR));

    return sizeof(ST_IPV4_HDR);
}

// 
// Function: InitUdpHeader
//
// Description:
//    Setup the UDP header which is fairly simple. Grab the ports and
//    stick in the total payload length.
//
int InitUdpHeader(
                  char *buf, 
                  SOCKADDR *src,
                  SOCKADDR *dest, 
                  int       payloadlen
                  )
{
    ST_UDP_HDR *udphdr=NULL;

    udphdr = (ST_UDP_HDR *)buf;

    // Port numbers are already in network byte order
    udphdr->src_portno = ((SOCKADDR_IN *)src)->sin_port;
    udphdr->dst_portno = ((SOCKADDR_IN *)dest)->sin_port;

    udphdr->udp_length = htons(sizeof(ST_UDP_HDR) + payloadlen);

    return sizeof(ST_UDP_HDR);
}

//
// Function: ComputeUdpPseudoHeaderChecksumV4
//
// Description:
//    Compute the UDP pseudo header checksum. The UDP checksum is based
//    on the following fields:
//       o source IP address
//       o destination IP address
//       o 8-bit zero field
//       o 8-bit protocol field
//       o 16-bit UDP length
//       o 16-bit source port
//       o 16-bit destination port
//       o 16-bit UDP packet length
//       o 16-bit UDP checksum (zero)
//       o UDP payload (padded to the next 16-bit boundary)
//    This routine copies these fields to a temporary buffer and computes
//    the checksum from that.
//
void ComputeUdpPseudoHeaderChecksumV4(
                                      void          *iphdr,
                                      ST_UDP_HDR    *udphdr,
                                      char          *payload,
                                      int           payloadlen
                                      )
{
    GRS_USEPRINTF();

    ST_IPV4_HDR*    v4hdr           = NULL;
    unsigned long   zero            = 0;
    char            buf[MAX_PACKET] = {};
    char*           ptr             = NULL;
    int             chksumlen       = 0;
    int             i               = 0;

    ptr = buf;

    v4hdr = (ST_IPV4_HDR *)iphdr;

    // Include the source and destination IP addresses
    memcpy(ptr, &v4hdr->ip_srcaddr,  sizeof(v4hdr->ip_srcaddr));  
    ptr += sizeof(v4hdr->ip_srcaddr);
    chksumlen += sizeof(v4hdr->ip_srcaddr);

    memcpy(ptr, &v4hdr->ip_destaddr, sizeof(v4hdr->ip_destaddr)); 
    ptr += sizeof(v4hdr->ip_destaddr);
    chksumlen += sizeof(v4hdr->ip_destaddr);

    // Include the 8 bit zero field
    memcpy(ptr, &zero, 1);
    ptr++;
    chksumlen += 1;

    // Protocol
    memcpy(ptr, &v4hdr->ip_protocol, sizeof(v4hdr->ip_protocol)); 
    ptr += sizeof(v4hdr->ip_protocol);
    chksumlen += sizeof(v4hdr->ip_protocol);

    // UDP length
    memcpy(ptr, &udphdr->udp_length, sizeof(udphdr->udp_length)); 
    ptr += sizeof(udphdr->udp_length);
    chksumlen += sizeof(udphdr->udp_length);

    // UDP source port
    memcpy(ptr, &udphdr->src_portno, sizeof(udphdr->src_portno)); 
    ptr += sizeof(udphdr->src_portno);
    chksumlen += sizeof(udphdr->src_portno);

    // UDP destination port
    memcpy(ptr, &udphdr->dst_portno, sizeof(udphdr->dst_portno)); 
    ptr += sizeof(udphdr->dst_portno);
    chksumlen += sizeof(udphdr->dst_portno);

    // UDP length again
    memcpy(ptr, &udphdr->udp_length, sizeof(udphdr->udp_length)); 
    ptr += sizeof(udphdr->udp_length);
    chksumlen += sizeof(udphdr->udp_length);

    // 16-bit UDP checksum, zero 
    memcpy(ptr, &zero, sizeof(unsigned short));
    ptr += sizeof(unsigned short);
    chksumlen += sizeof(unsigned short);

    // payload
    memcpy(ptr, payload, payloadlen);
    ptr += payloadlen;
    chksumlen += payloadlen;

    // pad to next 16-bit boundary
    for(i=0 ; i < payloadlen%2 ; i++, ptr++)
    {
        GRS_PRINTF(_T("pad one byte\n"));
        *ptr = 0;
        ptr++;
        chksumlen++;
    }

    // Compute the checksum and put it in the UDP header
    udphdr->udp_checksum = checksum((USHORT *)buf, chksumlen);

    return;
}

//
// Function: memfill
//
// Description:
//    Fills a block of memory with a given string pattern.
//
void memfill(
             char *dest,
             int   destlen,
             char *data,
             int   datalen
             )
{
    char *ptr     = NULL;
    int   copylen = 0;

    ptr = dest;
    while (destlen > 0)
    {
        copylen = ((destlen > datalen) ? datalen : destlen);
        memcpy(ptr, data, copylen);

        destlen -= copylen;
        ptr += copylen;
    }
}

//
// Function: PacketizeIpv4
//
// Description:
//    This routine takes the data buffer and packetizes it for IPv4.
//    Since the IPv4 stack takes care of fragmentation for us, this
//    routine simply initializes the IPv4 and UDP headers. The data
//    is returned in an array of WSABUF structures.
//
WSABUF *PacketizeIpv4(
    struct addrinfo *src,
    struct addrinfo *dest,
    char*   payload, 
    int     payloadlen
    )
{
    
    static WSABUF Packets[MAX_PACKET_FRAGMENTS] = {};
    int           iphdrlen                      = 0;
    int           udphdrlen                     = 0;
    GRS_USEPRINTF();

    // Allocate memory for the packet
    Packets[0].buf = (char *)GRS_CALLOC(sizeof(ST_IPV4_HDR) + sizeof(ST_UDP_HDR) + payloadlen);
    if (Packets[0].buf == NULL)
    {
        GRS_PRINTF(_T("PacetizeV4: HeapAlloc failed: %d\n"), GetLastError());
        ExitProcess(-1);
    }

    Packets[0].len = sizeof(ST_IPV4_HDR) + sizeof(ST_UDP_HDR) + payloadlen;

    // Initialize the v4 header
    iphdrlen = InitIpv4Header(
        Packets[0].buf, 
        src->ai_addr, 
        dest->ai_addr, 
        DEFAULT_TTL, 
        IPPROTO_UDP, 
        payloadlen
        );

    // Initialize the UDP header
    udphdrlen = InitUdpHeader(
        &Packets[0].buf[iphdrlen], 
        src->ai_addr, 
        dest->ai_addr, 
        payloadlen
        );

    // Compute the UDP checksum
    ComputeUdpPseudoHeaderChecksumV4(
        Packets[0].buf, 
        (ST_UDP_HDR *)&Packets[0].buf[iphdrlen], 
        payload, 
        payloadlen
        );

    // Copy the payload to the end of the header
    memcpy(&Packets[0].buf[iphdrlen + udphdrlen], payload, payloadlen);

    // Zero out the next WSABUF structure which indicates the end of
    //    the packets -- caller must free the buffers
    Packets[1].buf = NULL;
    Packets[1].len = 0;

    return Packets;
}

// 
// Function: main
//
// Description:
//    First, parse command line arguments and load Winsock. Then 
//    create the raw socket and then set the IP_HDRINCL option.
//    Following this assemble the IP and UDP packet headers by
//    assigning the correct values and calculating the checksums.
//    Then fill in the data and send to its destination.
//
int _cdecl _tmain(int argc, TCHAR **argv)
{
    GRS_USEPRINTF();
    WSADATA     wsd         = {};
    SOCKET      s           = INVALID_SOCKET;
    DWORD       bytes       = 0;
    WSABUF      *wbuf       = NULL;
    addrinfo    *ressrc     = NULL;
    addrinfo    *resdest    = NULL;
    addrinfo    *resbind    = NULL;
    int         packets     = 0;
    int         rc          = 0;
    int         i           = 0;
    int         j           = 0;
    DWORD       dwSendCnt   = 10;

    if(argc < 2)
    {
        usage(_T("RowUDP"));
        return -1;
    }

    // Parse command line arguments and print them out
    ValidateArgs(argc, argv);

    srand(GetTickCount());

    if (WSAStartup(MAKEWORD(2,2), &wsd) != 0)
    {
        GRS_PRINTF(_T("WSAStartup() failed: %d\n"), GetLastError());
        return -1;
    }

    // Convert the source and destination addresses/ports
    ressrc = GetAddrInfo("ASUS-PC", "7770", AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (ressrc == NULL)
    {
        GRS_PRINTFA("无法获取本地地址信息\n");
        _tsystem(_T("PAUSE"));
        return -1;
    }

    resdest = GetAddrInfo(gDestAddress, gDestPort, ressrc->ai_family, ressrc->ai_socktype, ressrc->ai_protocol);
    if (resdest == NULL)
    {
        GRS_PRINTFA("无法解析目标地址 '%s' 和端口 '%s'\n", 
            gDestAddress, gDestPort);
        _tsystem(_T("PAUSE"));
        return -1;
    }

    //  Creating a raw socket
    s = socket(ressrc->ai_family, SOCK_RAW,  ressrc->ai_protocol);
    if (s == INVALID_SOCKET)
    {
        GRS_PRINTF(_T("socket failed: %d\n"), WSAGetLastError());
        _tsystem(_T("PAUSE"));
        return -1;
    }


    char   *payload=NULL;
    payload = (char *)GRS_CALLOC(gSendSize);
    if (payload == NULL)
    {
        GRS_PRINTF(_T("HeapAlloc failed: %d\n"), GetLastError());
        _tsystem(_T("PAUSE"));
        return -1;
    }
    //填充缓冲数据
    memfill(payload, gSendSize, gMessage, strlen(gMessage));

    // Enable the IP header include option 
    int optval      = 1;
    rc = setsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *)&optval, sizeof(optval));
    if (rc == SOCKET_ERROR)
    {
        GRS_PRINTF(_T("setsockopt: IP_HDRINCL failed: %d\n"), WSAGetLastError());
        _tsystem(_T("PAUSE"));
        return -1;
    }

    //生成IPV4 UDP包
    wbuf = PacketizeIpv4(ressrc,resdest,payload,gSendSize);
    // Count how many packets there are
    i=0;
    packets=0;
    while (wbuf[i].buf)
    {
        GRS_PRINTF(_T("packet %d buf 0x%p len %d\n"),
            i, wbuf[i].buf, wbuf[i].len);
        packets++;
        i++;
    }

    // Apparently, this SOCKADDR_IN structure makes no difference.
    // Whatever we put as the destination IP addr in the IP
    // header is what goes. Specifying a different dest in remote
    // will be ignored.
    for( i = 0; i < (int)dwSendCnt; i++ )
    {
        for(j=0; j < packets ;j++)
        {
            rc = sendto(
                s,
                wbuf[j].buf,
                wbuf[j].len,
                0,
                resdest->ai_addr,
                resdest->ai_addrlen
                );
            bytes = rc;
            if (rc == SOCKET_ERROR)
            {
                GRS_PRINTF(_T("sendto() failed: %d\n"), WSAGetLastError());
                break;
            }
            else
            {
                GRS_PRINTF(_T("sent %d bytes\n"), bytes);
            }
        }
    }

    // Free the packet buffers
    for(i=0; i < packets ;i++)
    {
        GRS_SAFEFREE(wbuf[i].buf);
    }


    closesocket(s) ;
    _tsystem(_T("PAUSE"));
    WSACleanup() ;

    return 0;
}
