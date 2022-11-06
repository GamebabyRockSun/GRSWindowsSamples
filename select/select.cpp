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

#define MAX_BUFFER_LEN 256

class CClientContext
{
private:

    int               m_nTotalBytes;
    int               m_nSentBytes;
    SOCKET            m_Socket;
    BYTE              m_szBuffer[MAX_BUFFER_LEN];
    CClientContext   *m_pNext;

public:
    void SetTotalBytes(int n)
    {
        m_nTotalBytes = n;
    }

    int GetTotalBytes()
    {
        return m_nTotalBytes;
    }

    void SetSentBytes(int n)
    {
        m_nSentBytes = n;
    }

    void IncrSentBytes(int n)
    {
        m_nSentBytes += n;
    }

    int GetSentBytes()
    {
        return m_nSentBytes;
    }

    void SetSocket(SOCKET s)
    {
        m_Socket = s;
    }

    SOCKET GetSocket()
    {
        return m_Socket;
    }

    void SetBuffer(BYTE* szBuffer,SIZE_T szLen)
    {
        CopyMemory(m_szBuffer,szBuffer,min(szLen,MAX_BUFFER_LEN));
    }

    void GetBuffer(BYTE* szBuffer,SIZE_T szLen)
    {
        CopyMemory(szBuffer, m_szBuffer,min(szLen,MAX_BUFFER_LEN));
    }

    BYTE* GetBuffer()
    {
        return m_szBuffer;
    }

    void ZeroBuffer()
    {
        ZeroMemory(m_szBuffer, MAX_BUFFER_LEN);
    }

    CClientContext* GetNext()
    {
        return m_pNext;
    }

    void SetNext(CClientContext *pNext)
    {
        m_pNext = pNext;
    }

    //Constructor
    CClientContext()
    {
        m_Socket =  SOCKET_ERROR;
        ZeroMemory(m_szBuffer, MAX_BUFFER_LEN);
        m_nTotalBytes = 0;
        m_nSentBytes = 0;
        m_pNext = NULL;
    }

    //destructor
    ~CClientContext()
    {
        closesocket(m_Socket);
    }
};

//Head of the client context singly linked list
CClientContext    *g_pClientContextHead = NULL;

//Global FD Sets
fd_set g_ReadSet, g_WriteSet, g_ExceptSet;

//global functions
void AcceptConnections(SOCKET ListenSocket);
void InitSets(SOCKET ListenSocket);
int GetSocketSpecificError(SOCKET Socket);
CClientContext* GetClientContextHead();
void AddClientContextToList(CClientContext *pClientContext);
CClientContext* DeleteClientContext(CClientContext *pClientContext);

int _tmain(int argc, TCHAR *argv[])
{
    GRS_USEPRINTF();
    WORD wVer = MAKEWORD(SOCK_VERH,SOCK_VERL);
    WSADATA wd = {};
    int err = 0;
    SOCKET ListenSocket = INVALID_SOCKET; 
    int    nPortNo      = 0;
    sockaddr_in ServerAddress = {AF_INET};
    
    if (argc < 2) 
    {
        GRS_PRINTF(_T("\n命令格式: %s port."), argv[0]);
        goto error;
    }

    err = ::WSAStartup(wVer,&wd);
    if(0 != err)
    {
        GRS_PRINTF(_T("无法初始化Socket2系统环境，错误码为：0x%08X！\n"),WSAGetLastError());
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



    //Create a socket
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (INVALID_SOCKET == ListenSocket) 
    {
        GRS_PRINTF(_T("\n创建监听SOCKET句柄错误,错误码: 0x%08X."), WSAGetLastError());
        goto error;
    }
    else
    {
        GRS_PRINTF(_T("\n监听SOCKET创建成功."));
    }
    
    nPortNo = _ttoi(argv[1]);

    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_addr.s_addr = INADDR_ANY;
    ServerAddress.sin_port = htons(nPortNo);

    if (SOCKET_ERROR == bind(ListenSocket, (SOCKADDR *)&ServerAddress, sizeof(ServerAddress))) 
    {
        closesocket(ListenSocket);

        GRS_PRINTF(_T("\n无法绑定到指定地址端口,错误码0x%08x."),WSAGetLastError());
        goto error;
    }
    else
    {
        GRS_PRINTF(_T("\n绑定成功!"));
    }

    if (SOCKET_ERROR == listen(ListenSocket,SOMAXCONN))
    {
        closesocket(ListenSocket);

        GRS_PRINTF(_T("\nSOCKET置为监听模式失败,错误码:0x%08X."),WSAGetLastError());
        goto error;
    }
    else
    {
        GRS_PRINTF(_T("\n开始监听."));
    }

    //This function will take are of multiple clients using select()/accept()
    AcceptConnections(ListenSocket);

    //Close open sockets
    closesocket(ListenSocket);


    _tsystem(_T("PAUSE"));
    WSACleanup();
    return 0;
    //==========================================================================================================
error:
    _tsystem(_T("PAUSE"));
    ::WSACleanup();
    return 1;
}

//Initialize the Sets
void InitSets(SOCKET ListenSocket) 
{
    //Initialize
    FD_ZERO(&g_ReadSet);
    FD_ZERO(&g_WriteSet);
    FD_ZERO(&g_ExceptSet);

    //Assign the ListenSocket to Sets
    FD_SET(ListenSocket, &g_ReadSet);
    FD_SET(ListenSocket, &g_ExceptSet);

    //Iterate the client context list and assign the sockets to Sets
    CClientContext   *pClientContext  = GetClientContextHead();

    while(pClientContext)
    {
        if(pClientContext->GetSentBytes() < pClientContext->GetTotalBytes())
        {
            //We have data to send
            FD_SET(pClientContext->GetSocket(), &g_WriteSet);
        }
        else
        {
            //We can read on this socket
            FD_SET(pClientContext->GetSocket(), &g_ReadSet);
        }

        //Add it to Exception Set
        FD_SET(pClientContext->GetSocket(), &g_ExceptSet); 

        //Move to next node on the list
        pClientContext = pClientContext->GetNext();
    }
}


//This function will loop on while it will manage multiple clients using select()
void AcceptConnections(SOCKET ListenSocket)
{
    GRS_USEPRINTF();

    while (true)
    {
        InitSets(ListenSocket);

        if (select(0, &g_ReadSet, &g_WriteSet, &g_ExceptSet, 0) > 0) 
        {
            if (FD_ISSET(ListenSocket, &g_ReadSet)) 
            {
                sockaddr_in ClientAddress;
                int nClientLength = sizeof(ClientAddress);

                SOCKET Socket = accept(ListenSocket, (SOCKADDR*)&ClientAddress, &nClientLength);

                if (INVALID_SOCKET == Socket)
                {
                    GRS_PRINTF(_T("\naccept调用失败,错误码: 0x%08X."), GetSocketSpecificError(ListenSocket));
                }

                GRS_PRINTFA("\n客户端连接进来: [%s : %u]"
                    , inet_ntoa(ClientAddress.sin_addr),ntohs(ClientAddress.sin_port)); 

                //设置为非阻塞模式
                u_long nNoBlock = 1;
                ioctlsocket(Socket, FIONBIO, &nNoBlock);
                
                CClientContext   *pClientContext  = new CClientContext;
                pClientContext->SetSocket(Socket);

                AddClientContextToList(pClientContext);
            }

            if (FD_ISSET(ListenSocket, &g_ExceptSet)) 
            {
                GRS_PRINTF(_T("\naccept调用失败,错误码: 0x%08X."), GetSocketSpecificError(ListenSocket));
                continue;
            }

            CClientContext   *pClientContext  = GetClientContextHead();

            while (pClientContext)
            {
                if (FD_ISSET(pClientContext->GetSocket(), &g_ReadSet))
                {
                    int nBytes = recv(pClientContext->GetSocket(), (char*)pClientContext->GetBuffer(), MAX_BUFFER_LEN, 0);

                    if ((0 == nBytes) || (SOCKET_ERROR == nBytes))
                    {
                        if (0 != nBytes)
                        {
                            GRS_PRINTF(_T("\n接收客户端数据失败,错误码: 0x%08X.")
                                , GetSocketSpecificError(pClientContext->GetSocket()));
                        }
                        pClientContext = DeleteClientContext(pClientContext);
                        continue;
                    }

                    pClientContext->SetTotalBytes(nBytes);
                    pClientContext->SetSentBytes(0);

                    GRS_PRINTF(_T("\n客户端发来信息: %s"), (TCHAR*)pClientContext->GetBuffer());
                }

                if ( FD_ISSET(pClientContext->GetSocket(), &g_WriteSet) )
                {
                    int nBytes = 0;

                    if (0 < (pClientContext->GetTotalBytes() - pClientContext->GetSentBytes()))
                    {
                        nBytes = send(pClientContext->GetSocket(),(char*)(pClientContext->GetBuffer() + pClientContext->GetSentBytes()), (pClientContext->GetTotalBytes() - pClientContext->GetSentBytes()), 0);

                        if (SOCKET_ERROR == nBytes)
                        {
                            GRS_PRINTF(_T("\n发送数据到客户端失败,错误码: 0x%08X.")
                                , GetSocketSpecificError(pClientContext->GetSocket()));
                            pClientContext = DeleteClientContext(pClientContext);
                            continue;
                        }
                        if (nBytes == (pClientContext->GetTotalBytes() - pClientContext->GetSentBytes()))
                        {
                            pClientContext->SetTotalBytes(0);
                            pClientContext->SetSentBytes(0);
                        }
                        else
                        {
                            pClientContext->IncrSentBytes(nBytes);
                        }
                    }
                }

                if (FD_ISSET(pClientContext->GetSocket(), &g_ExceptSet))
                {
                    GRS_PRINTF(_T("\n客户端SOCKET发生错误: 0x%08X.")
                        , GetSocketSpecificError(pClientContext->GetSocket()));
                    pClientContext = DeleteClientContext(pClientContext);
                    continue;
                }
                pClientContext = pClientContext->GetNext();
            }//while
        }
        else 
        {
            GRS_PRINTF(_T("\nselect函数调用失败,错误码: 0x%08X."), WSAGetLastError());
            return;
        }
    }
}

int GetSocketSpecificError(SOCKET Socket)
{
    int nOptionValue = 0;
    int nOptionValueLength = sizeof(nOptionValue);

    //获取指定SOCKET句柄上的错误码
    getsockopt(Socket, SOL_SOCKET, SO_ERROR, (char*)&nOptionValue, &nOptionValueLength);

    return nOptionValue;
}

CClientContext* GetClientContextHead()
{
    return g_pClientContextHead;
}

void AddClientContextToList(CClientContext *pClientContext)
{
    pClientContext->SetNext(g_pClientContextHead);
    g_pClientContextHead = pClientContext;
}

CClientContext * DeleteClientContext(CClientContext *pClientContext)
{
    if (pClientContext == g_pClientContextHead) 
    {
        CClientContext *pTemp = g_pClientContextHead;
        g_pClientContextHead = g_pClientContextHead->GetNext();
        delete pTemp;
        return g_pClientContextHead;
    }

    CClientContext *pPrev = g_pClientContextHead;
    CClientContext *pCurr = g_pClientContextHead->GetNext();

    while (pCurr)
    {
        if (pCurr == pClientContext)
        {
            CClientContext *pTemp = pCurr->GetNext();
            pPrev->SetNext(pTemp);
            delete pCurr;
            return pTemp;
        }

        pPrev = pCurr;
        pCurr = pCurr->GetNext();
    }

    return NULL;
}
