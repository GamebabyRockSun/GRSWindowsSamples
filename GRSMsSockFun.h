#pragma once
#include <Winsock2.h>
#include <MSWSOCK.h> 	
#pragma comment( lib, "Ws2_32.lib" )
#pragma comment( lib, "Mswsock.lib" )

#ifdef _DEBUG
#define GRS_ASSERT(s) if(!(s)) {::DebugBreak();}
#else
#define GRS_ASSERT(s) 
#endif

#define GRS_USEPRINTF() TCHAR pOutBufT[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
    StringCchPrintf(pOutBufT,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBufT,lstrlen(pOutBufT),NULL,NULL);
#define GRS_PRINTFA(...) \
    StringCchPrintfA(pOutBufA,1024,__VA_ARGS__);\
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE),pOutBufA,lstrlenA(pOutBufA),NULL,NULL);

class CGRSMsSockFun
{
public:
    CGRSMsSockFun(SOCKET skTemp = INVALID_SOCKET)
    {
        LoadAllFun(skTemp);
    }
    CGRSMsSockFun(int af,int type,int protocol)
    {
        LoadAllFun(af,type,protocol);
    }
public:
    virtual ~CGRSMsSockFun(void)
    {
    }
protected:
    BOOL LoadWSAFun(SOCKET& skTemp,GUID&funGuid,void*&pFun)
    {
        GRS_USEPRINTF();
        DWORD dwBytes = 0;
        BOOL bRet = TRUE;
        pFun = NULL;
        if( INVALID_SOCKET == skTemp )
        {
            GRS_PRINTF(_T("�����˿յ�SOCKET���,�޷������չ����������!"));
            return FALSE;
        }

        if(SOCKET_ERROR == ::WSAIoctl(skTemp, 
            SIO_GET_EXTENSION_FUNCTION_POINTER, 
            &funGuid,
            sizeof(funGuid),
            &pFun, 
            sizeof(pFun), 
            &dwBytes, 
            NULL, 
            NULL))
        {
            pFun = NULL;
            return FALSE;
        }
#ifdef _DEBUG
        {
            GUID Guid = WSAID_ACCEPTEX;
            if(IsEqualGUID(Guid,funGuid))
            {
                GRS_PRINTF(_T("AcceptEx ���سɹ�!\n"));
            }
        }

        {
            GUID Guid = WSAID_CONNECTEX;
            if(IsEqualGUID(Guid,funGuid))
            {
                GRS_PRINTF(_T("ConnectEx ���سɹ�!\n"));
            }
        }

        {
            GUID Guid = WSAID_DISCONNECTEX;
            if(IsEqualGUID(Guid,funGuid))
            {
                GRS_PRINTF(_T("DisconnectEx ���سɹ�!\n"));
            }
        }

        {
            GUID Guid = WSAID_GETACCEPTEXSOCKADDRS;
            if(IsEqualGUID(Guid,funGuid))
            {
                GRS_PRINTF(_T("GetAcceptExSockaddrs ���سɹ�!\n"));
            }
        }
        {
            GUID Guid = WSAID_TRANSMITFILE;
            if(IsEqualGUID(Guid,funGuid))
            {
                GRS_PRINTF(_T("TransmitFile ���سɹ�!\n"));
            }

        }
        {
            GUID Guid = WSAID_TRANSMITPACKETS;
            if(IsEqualGUID(Guid,funGuid))
            {
                GRS_PRINTF(_T("TransmitPackets ���سɹ�!\n"));
            }

        }
        {
            GUID Guid = WSAID_WSARECVMSG;
            if(IsEqualGUID(Guid,funGuid))
            {
                GRS_PRINTF(_T("WSARecvMsg ���سɹ�!\n"));
            }
        }

#if(_WIN32_WINNT >= 0x0600)
        {
            GUID Guid = WSAID_WSASENDMSG;
            if(IsEqualGUID(Guid,funGuid))
            {
                GRS_PRINTF(_T("WSASendMsg ���سɹ�!\n"));
            }
        }
#endif

#endif
        return NULL != pFun;
    }
protected:
    LPFN_ACCEPTEX m_pfnAcceptEx;
    LPFN_CONNECTEX m_pfnConnectEx;
    LPFN_DISCONNECTEX m_pfnDisconnectEx;
    LPFN_GETACCEPTEXSOCKADDRS m_pfnGetAcceptExSockaddrs;
    LPFN_TRANSMITFILE m_pfnTransmitfile;
    LPFN_TRANSMITPACKETS m_pfnTransmitPackets;
    LPFN_WSARECVMSG m_pfnWSARecvMsg;
#if(_WIN32_WINNT >= 0x0600)
    LPFN_WSASENDMSG m_pfnWSASendMsg;
#endif
    
protected:
    BOOL LoadAcceptExFun(SOCKET &skTemp)
    {
        GUID GuidAcceptEx = WSAID_ACCEPTEX;
        return LoadWSAFun(skTemp,GuidAcceptEx,(void*&)m_pfnAcceptEx);
    }

    BOOL LoadConnectExFun(SOCKET &skTemp)
    {
        GUID GuidAcceptEx = WSAID_CONNECTEX;
        return LoadWSAFun(skTemp,GuidAcceptEx,(void*&)m_pfnConnectEx);
    }

    BOOL LoadDisconnectExFun(SOCKET&skTemp)
    {
        GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
        return LoadWSAFun(skTemp,GuidDisconnectEx,(void*&)m_pfnDisconnectEx);
    }

    BOOL LoadGetAcceptExSockaddrsFun(SOCKET &skTemp)
    {
        GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
        return LoadWSAFun(skTemp,GuidGetAcceptExSockaddrs,(void*&)m_pfnGetAcceptExSockaddrs);
    }

    BOOL LoadTransmitFileFun(SOCKET&skTemp)
    {
        GUID GuidTransmitFile = WSAID_TRANSMITFILE;
        return LoadWSAFun(skTemp,GuidTransmitFile,(void*&)m_pfnTransmitfile);
    }

    BOOL LoadTransmitPacketsFun(SOCKET&skTemp)
    {
        GUID GuidTransmitPackets = WSAID_TRANSMITPACKETS;
        return LoadWSAFun(skTemp,GuidTransmitPackets,(void*&)m_pfnTransmitPackets);
    }

    BOOL LoadWSARecvMsgFun(SOCKET&skTemp)
    {
        GUID GuidWSARecvMsg = WSAID_WSARECVMSG;
        return LoadWSAFun(skTemp,GuidWSARecvMsg,(void*&)m_pfnWSARecvMsg);
    }

#if(_WIN32_WINNT >= 0x0600)
    BOOL LoadWSASendMsgFun(SOCKET&skTemp)
    {
        GUID GuidWSASendMsg = WSAID_WSASENDMSG;
        return LoadWSAFun(skTemp,GuidWSASendMsg,(void*&)m_pfnWSASendMsg);
    }
#endif

public:
    BOOL LoadAllFun(SOCKET skTemp)
    {//ע������ط��ĵ���˳���Ǹ��ݷ���������Ҫ��������˱��ʽ������
        //�����ⰲ�ŵĵ���˳��
        GRS_USEPRINTF();
        BOOL bCreateSocket = FALSE;
        BOOL bRet          = FALSE;

        if(INVALID_SOCKET == skTemp)
        {//�������յ�SOCKET���,��ô��Ĭ����TCPЭ��������SOCKET���
            //�������ص���չ����ֻ����TCPЭ�鹤��
            skTemp = ::WSASocket(AF_INET, 
                SOCK_STREAM, 
                IPPROTO_TCP, 
                NULL, 
                0, 
                WSA_FLAG_OVERLAPPED);

            bCreateSocket = (skTemp != INVALID_SOCKET);
            if(!bCreateSocket)
            {
                GRS_PRINTF(_T("������ʱSOCKET�������,������:0x%08X\n"),WSAGetLastError());
                return FALSE;
            }
        }

        bRet = (LoadAcceptExFun(skTemp) &&
                LoadGetAcceptExSockaddrsFun(skTemp) &&
                LoadTransmitFileFun(skTemp) &&
                LoadTransmitPacketsFun(skTemp) &&
                LoadDisconnectExFun(skTemp) &&
                LoadConnectExFun(skTemp) && 
                LoadWSARecvMsgFun(skTemp));

        if(bCreateSocket)
        {
            closesocket(skTemp);
        }
        return bRet; 
    }

    BOOL LoadAllFun(int af,int type,int protocol)
    {
        GRS_USEPRINTF();
        BOOL bRet      = FALSE;
        SOCKET skTemp  = INVALID_SOCKET;

        skTemp = ::WSASocket(af, 
            type, 
            protocol, 
            NULL, 
            0, 
            WSA_FLAG_OVERLAPPED);

        if( INVALID_SOCKET == skTemp )
        {
            GRS_PRINTF(_T("������ʱSOCKET�������,������:0x%08X\n"),WSAGetLastError());
            return FALSE;
        }

        bRet = (LoadAcceptExFun(skTemp) &&
            LoadGetAcceptExSockaddrsFun(skTemp) &&
            LoadTransmitFileFun(skTemp) &&
            LoadTransmitPacketsFun(skTemp) &&
            LoadDisconnectExFun(skTemp) &&
            LoadConnectExFun(skTemp) && 
            LoadWSARecvMsgFun(skTemp));

        if(INVALID_SOCKET != skTemp)
        {
            closesocket(skTemp);
        }
        return bRet; 
    }
public:
    BOOL AcceptEx (
        SOCKET sListenSocket,
        SOCKET sAcceptSocket,
        PVOID lpOutputBuffer,
        DWORD dwReceiveDataLength,
        DWORD dwLocalAddressLength,
        DWORD dwRemoteAddressLength,
        LPDWORD lpdwBytesReceived,
        LPOVERLAPPED lpOverlapped
        )
    {
        GRS_ASSERT(NULL != m_pfnAcceptEx);
        return m_pfnAcceptEx(sListenSocket,
            sAcceptSocket,
            lpOutputBuffer,
            dwReceiveDataLength,
            dwLocalAddressLength,
            dwRemoteAddressLength,
            lpdwBytesReceived,
            lpOverlapped);
    }

    BOOL ConnectEx(
        SOCKET s,
        const struct sockaddr FAR *name,
        int namelen,
        PVOID lpSendBuffer,
        DWORD dwSendDataLength,
        LPDWORD lpdwBytesSent,
        LPOVERLAPPED lpOverlapped
        )
    {
        GRS_ASSERT(NULL != m_pfnConnectEx);
        return m_pfnConnectEx(
            s,
            name,
            namelen,
            lpSendBuffer,
            dwSendDataLength,
            lpdwBytesSent,
            lpOverlapped
            );
    }

    BOOL DisconnectEx(
        SOCKET s,
        LPOVERLAPPED lpOverlapped,
        DWORD  dwFlags,
        DWORD  dwReserved
        )
    {
        GRS_ASSERT(NULL != m_pfnDisconnectEx);
        return m_pfnDisconnectEx(s,
            lpOverlapped,
            dwFlags,
            dwReserved);
    }

    VOID GetAcceptExSockaddrs (
        PVOID lpOutputBuffer,
        DWORD dwReceiveDataLength,
        DWORD dwLocalAddressLength,
        DWORD dwRemoteAddressLength,
        sockaddr **LocalSockaddr,
        LPINT LocalSockaddrLength,
        sockaddr **RemoteSockaddr,
        LPINT RemoteSockaddrLength
        )
    {
        GRS_ASSERT(NULL != m_pfnGetAcceptExSockaddrs);
        return m_pfnGetAcceptExSockaddrs(
            lpOutputBuffer,
            dwReceiveDataLength,
            dwLocalAddressLength,
            dwRemoteAddressLength,
            LocalSockaddr,
            LocalSockaddrLength,
            RemoteSockaddr,
            RemoteSockaddrLength
            );
    }

    BOOL TransmitFile(
        SOCKET hSocket,
        HANDLE hFile,
        DWORD nNumberOfBytesToWrite,
        DWORD nNumberOfBytesPerSend,
        LPOVERLAPPED lpOverlapped,
        LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
        DWORD dwReserved
        )
    {
        GRS_ASSERT(NULL != m_pfnTransmitfile);
        return m_pfnTransmitfile(
            hSocket,
            hFile,
            nNumberOfBytesToWrite,
            nNumberOfBytesPerSend,
            lpOverlapped,
            lpTransmitBuffers,
            dwReserved
            );
    }

    BOOL TransmitPackets(
        SOCKET hSocket,                             
        LPTRANSMIT_PACKETS_ELEMENT lpPacketArray,                               
        DWORD nElementCount,                
        DWORD nSendSize,                
        LPOVERLAPPED lpOverlapped,                  
        DWORD dwFlags                               
        )
    {
        GRS_ASSERT(NULL != m_pfnTransmitPackets);
        return m_pfnTransmitPackets(
            hSocket,                             
            lpPacketArray,                               
            nElementCount,                
            nSendSize,                
            lpOverlapped,                  
            dwFlags         
            );
    }

    INT WSARecvMsg(
        SOCKET s, 
        LPWSAMSG lpMsg, 
        LPDWORD lpdwNumberOfBytesRecvd, 
        LPWSAOVERLAPPED lpOverlapped, 
        LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
        )
    {
        GRS_ASSERT(NULL != m_pfnWSARecvMsg);
        return m_pfnWSARecvMsg(
            s, 
            lpMsg, 
            lpdwNumberOfBytesRecvd, 
            lpOverlapped, 
            lpCompletionRoutine
            );
    }

#if(_WIN32_WINNT >= 0x0600)
    INT WSASendMsg(
        SOCKET s,
        LPWSAMSG lpMsg,
        DWORD dwFlags,
        LPDWORD lpNumberOfBytesSent,
        LPWSAOVERLAPPED lpOverlapped,
        LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
        )
    {
        GRS_ASSERT(NULL != m_pfnWSASendMsg);
        return m_pfnWSASendMsg(
            s, 
            lpMsg, 
            dwFlags,
            lpNumberOfBytesSent, 
            lpOverlapped, 
            lpCompletionRoutine
            );
    }
#endif
    /*WSAID_ACCEPTEX
    WSAID_CONNECTEX
    WSAID_DISCONNECTEX
    WSAID_GETACCEPTEXSOCKADDRS
    WSAID_TRANSMITFILE
    WSAID_TRANSMITPACKETS
    WSAID_WSARECVMSG
    WSAID_WSASENDMSG */

};