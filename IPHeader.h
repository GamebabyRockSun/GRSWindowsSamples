#pragma once
// Align on a 1-byte boundary
#include <pshpack1.h>

typedef struct _ST_IPV4_HDR
{
    unsigned char  ip_hdrlen:4;      // 4-bit header length (in 32-bit words)     
    unsigned char  ip_ver:4;         // 4-bit IPv4 version

    unsigned char  ip_tosrcv:1;
    unsigned char  ip_tos:4;           // IP type of service
    unsigned char  ip_tospri:3;           // IP type of service

    unsigned short ip_totallength;   // Total length
    unsigned short ip_id;            // Unique identifier 

    unsigned short ip_offset:13;        // Fragment offset field
    unsigned short ip_FM:1;          // FM
    unsigned short ip_DM:1;          // DM
    unsigned short ip_rsv0:1;        // Reserve 0
    
    unsigned char  ip_ttl;           // Time to live
    unsigned char  ip_protocol;      // Protocol(TCP,UDP etc)
    unsigned short ip_checksum;      // IP checksum
    in_addr        ip_srcaddr;       // Source address
    in_addr        ip_destaddr;      // Source address
} 
ST_IPV4_HDR, *PST_IPV4_HDR;

typedef struct _ST_IPV4_OPTION_HDR
{
    unsigned char   opt_code;           // option type
    unsigned char   opt_len;            // length of the option header
    unsigned char   opt_ptr;            // offset into options
    unsigned long   opt_addr[9];        // list of IPv4 addresses
} 
ST_IPV4_OPTION_HDR, *PST_IPV4_OPTION_HDR;

typedef struct _ST_ICMP_HDR
{
    //前三个字段才是ICMP协议中一般要求具有的字段
    unsigned char   icmp_type;
    unsigned char   icmp_code;
    unsigned short  icmp_checksum;

    //这两个字段主要为PING命令而设置,不同的类型和代码要求的后续字段是不同的
    unsigned short  icmp_id;
    unsigned short  icmp_sequence;
    //这个纯粹已经算是数据字段了,加在这里的目的是方便计算PING的时间
    unsigned long   icmp_timestamp;
} 
ST_ICMP_HDR, *PST_ICMP_HDR;

// IPv6 protocol header
typedef struct _ST_IPV6_HDR
{
    unsigned long   ipv6_ver:4;            // 4-bit IPv6 version
    unsigned long   ipv6_tc:8;             // 8-bit traffic class
    unsigned long   ipv6_flow:20;          // 20-bit flow label
    unsigned short  ipv6_payloadlen;       // payload length
    unsigned char   ipv6_nexthdr;          // next header protocol value
    unsigned char   ipv6_hoplimit;         // TTL 
    struct in6_addr ipv6_srcaddr;          // Source address
    struct in6_addr ipv6_destaddr;         // Destination address
} 
ST_IPV6_HDR, *PST_IPV6_HDR;

// IPv6 fragment header
typedef struct _ST_IPV6_FRAGMENT_HDR
{
    unsigned char   ipv6_frag_nexthdr;
    unsigned char   ipv6_frag_reserved;
    unsigned short  ipv6_frag_offset;
    unsigned long   ipv6_frag_id;
} 
ST_IPV6_FRAGMENT_HDR, *PST_IPV6_FRAGMENT_HDR;

typedef struct _ST_ICMPV6_HDR 
{
    unsigned char   icmp6_type;
    unsigned char   icmp6_code;
    unsigned short  icmp6_checksum;
} 
ST_ICMPV6_HDR,*PST_ICMPV6_HDR;

typedef struct _ST_ICMPV6_ECHO_REQUEST
{
    unsigned short  icmp6_echo_id;
    unsigned short  icmp6_echo_sequence;
} 
ST_ICMPV6_ECHO_REQUEST,*PST_ICMPV6_ECHO_REQUEST;

typedef struct _ST_UDP_HDR
{
    unsigned short src_portno;       // Source port no.
    unsigned short dst_portno;       // Dest. port no.
    unsigned short udp_length;       // Udp packet length
    unsigned short udp_checksum;     // Udp checksum (optional)
} 
ST_UDP_HDR, *PST_UDP_HDR;

typedef struct _ST_TCP_HDR 
{
    unsigned short src_port ;
    unsigned short dst_port ;
    unsigned long  tcp_seq ;
    unsigned long  tcp_ack_seq ;  
    unsigned short tcp_res1 : 4 ;
    unsigned short tcp_doff : 4 ;
    unsigned short tcp_fin : 1 ;
    unsigned short tcp_syn : 1 ;
    unsigned short tcp_rst : 1 ;
    unsigned short tcp_psh : 1 ;
    unsigned short tcp_ack : 1 ;
    unsigned short tcp_urg : 1 ;
    unsigned short tcp_res2 : 2 ;
    unsigned short tcp_window ;  
    unsigned short tcp_checksum ;
    unsigned short tcp_urg_ptr ;
} 
ST_TCP_HDR,*PST_TCP_HDR;


// IPv4 option for record route
#define IP_RECORD_ROUTE     0x7

// ICMP6 protocol value (used in the socket call and IPv6 header)
#define IPPROTO_ICMP6       58

// ICMP types and codes
#define ICMPV4_ECHO_REQUEST_TYPE   8
#define ICMPV4_ECHO_REQUEST_CODE   0
#define ICMPV4_ECHO_REPLY_TYPE     0
#define ICMPV4_ECHO_REPLY_CODE     0
#define ICMPV4_MINIMUM_HEADER      8

#define ICMPV4_DESTUNREACH    3
#define ICMPV4_SRCQUENCH      4
#define ICMPV4_REDIRECT       5
#define ICMPV4_ECHO           8
#define ICMPV4_TIMEOUT       11
#define ICMPV4_PARMERR       12

// ICPM6 types and codes
#define ICMPV6_ECHO_REQUEST_TYPE   128
#define ICMPV6_ECHO_REQUEST_CODE   0
#define ICMPV6_ECHO_REPLY_TYPE     129
#define ICMPV6_ECHO_REPLY_CODE     0
#define ICMPV6_TIME_EXCEEDED_TYPE  3
#define ICMPV6_TIME_EXCEEDED_CODE  0

// Restore byte alignment back to default
#include <poppack.h>
