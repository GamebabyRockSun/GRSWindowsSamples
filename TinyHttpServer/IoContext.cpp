#include "StdAfx.h"
#include "IoContext.h"
#include "IoBuffer.h"
#include "IOCP.h"

#define TIMEOUT_MS 3000

extern CIOCP * m_pIocp;
extern MIMETYPES MimeTypes;
extern string m_strCurDir;

CIoContext::~CIoContext()
{
	CloseSocket();
}

CIoContext::CIoContext(SOCKET sock)
{
	m_sIo = sock;
	Init();
}

CIoContext::CIoContext()
{
	CreateSocket();
	Init();
}

bool CIoContext::CreateSocket()
{
	if ((m_sIo = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0,
		WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) 
	{
		return false;
	}

	//Init();

	return true;
}

void CIoContext::CloseSocket()
{
	StopConThread();
	closesocket(m_sIo);
}

void CIoContext::Init()
{
	const char chOpt = 1;
	setsockopt(m_sIo, IPPROTO_TCP, TCP_NODELAY, &chOpt, sizeof(char));

	unsigned long ul = 1;
	ioctlsocket(m_sIo, FIONBIO, (unsigned long*)&ul);

	m_hConThreadId = 0;
}

bool CIoContext::AssociateWithIocp()
{
	return CreateIoCompletionPort((HANDLE)m_sIo, m_pIocp->m_hCompletionPort, 
		(DWORD)this, 0) == m_pIocp->m_hCompletionPort;
}

void CIoContext::ProcessIoMsg(CIoBuffer * pIoBuffer)
{
	switch(pIoBuffer->GetType())
	{
	case IOAccepted:
		AsyncAccept();
		OnAccepted(pIoBuffer);
		break;

	case IORecved:
		if(pIoBuffer->GetIoSize()==0)
		{
			delete pIoBuffer;
			OnClosed();
			return;
		}

		if(!AsyncRecv())
		{
			OnClosed();
			return;
		}
		
		OnRecved(pIoBuffer);
		break;

	case IOSent:
		OnSent(pIoBuffer);
		break;

	case IOFileTransmitted:
		OnFileTransmitted(pIoBuffer);
		break;

	default:
		delete pIoBuffer;
	}
}

void CIoContext::OnAccepted(CIoBuffer * pIoBuffer)
{
    if( pIoBuffer == NULL )
    {
        //closesocket(*reinterpret_cast<SOCKET*>(pIoBuffer->GetBuffer()));
        //delete pIoBuffer;
        return;
    }

	CIoContext * pIoContext = new CIoContext(*reinterpret_cast<SOCKET*>(pIoBuffer->GetBuffer()));

	setsockopt(pIoContext->m_sIo, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,  
		( char* )&(m_sIo), sizeof( m_sIo ) );

	SOCKADDR* remote;
	SOCKADDR* local;
	int remote_len = sizeof(SOCKADDR_IN); 
	int local_len = sizeof(SOCKADDR_IN);
	DWORD dwRet=0;
	GetAcceptExSockaddrs(pIoBuffer->GetBuffer()+sizeof(SOCKET),
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&local,
		&local_len,
		&remote,
		&remote_len);
	
	memcpy(&pIoContext->m_addrRemote, remote, sizeof(SOCKADDR_IN));

	delete pIoBuffer;

	pIoContext->AssociateWithIocp();
	pIoContext->AsyncRecv();
}

void CIoContext::OnConnected(CIoBuffer * pIoBuffer)
{
	// AssociateWithIocp();
	AsyncRecv();
	delete pIoBuffer;
}

bool CIoContext::AsyncConnect(const char * szIp, int nPort)
{
	SOCKADDR_IN AddrSvr;

	AddrSvr.sin_family = AF_INET;
	AddrSvr.sin_port = htons(nPort);
	AddrSvr.sin_addr.s_addr = inet_addr(szIp);	

	int iRet = WSAConnect(m_sIo, (SOCKADDR *)&AddrSvr, sizeof(SOCKADDR_IN), NULL, NULL, NULL, NULL);
	
	if(iRet==0)
	{
		CIoBuffer * pIoBuffer = new CIoBuffer(IOConnected);
		OnConnected(pIoBuffer);
		return true;
	}

	if(WSAGetLastError()!=WSAEWOULDBLOCK)
	{
		// SetErr(ERR_SOCKET);
		return false;
	}
	
	if(m_hConThreadId==0)
	{
		DWORD dwThreadId;
		m_hConThreadId = (HANDLE)CreateThread(NULL, 0, ThreadConnect, (LPVOID)this, 0, &dwThreadId);
	}
	else if(ResumeThread(m_hConThreadId)==0xFFFFFFFF)
	{
		// SetErr(ERR_WINDOW);
		return false;
	}

	return true;	 
}

DWORD CIoContext::ThreadConnect(LPVOID pParam)
{
	CIoContext * pThis = (CIoContext*)pParam;

	while(1)
	{
		CIoBuffer * pIoBuffer = new CIoBuffer(IOConnected);

		struct timeval timeout;
		fd_set r;
		int ret;
		
		FD_ZERO(&r);
		FD_SET(pThis->m_sIo, &r);
		timeout.tv_sec = 0; 
		timeout.tv_usec = TIMEOUT_MS*1000;
		ret = select(0, 0, &r, 0, &timeout);

		if ( ret <= 0 )
		{
			// pThis->SetErr(ERR_SOCKET);
			CancelIo((HANDLE)pThis->m_sIo);
			pThis->OnIoFailed(pIoBuffer);
		}
		else
			pThis->OnConnected(pIoBuffer);	

		::SuspendThread(::GetCurrentThread());
	}

	return 1;
}

bool CIoContext::AsyncAccept()
{
	SOCKET sAccept;
	if ((sAccept = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0,
		WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) 
	{
		return false;
	}

	DWORD dwBytesRecved = 0;
	CIoBuffer * pIoBuffer = new CIoBuffer(IOAccepted);
	pIoBuffer->Reserve(sizeof(SOCKET) + ( sizeof(SOCKADDR_IN) + 16) * 2 );
	pIoBuffer->AddData(sAccept);

	BOOL b = AcceptEx(m_sIo, sAccept, pIoBuffer->GetBuffer()+sizeof(SOCKET), 0, 
		sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, &dwBytesRecved, (LPOVERLAPPED)&(pIoBuffer->m_ol));
	
	if(b || WSAGetLastError()==ERROR_IO_PENDING)
	{
		return true;
	}

	delete pIoBuffer;
	return false;
}

bool CIoContext::AsyncRecv()
{
	DWORD dwIoSize=0;
	ULONG ulFlags = MSG_PARTIAL;
	CIoBuffer * pIoBuffer = new CIoBuffer(IORecved);
	pIoBuffer->Reserve(MAX_RECV_SIZE);

	int nRet = WSARecv(m_sIo,
					pIoBuffer->GetWSABuffer(),
					1,
					&dwIoSize,
					&ulFlags,
					&pIoBuffer->m_ol, 
					NULL);
	
	if(nRet==0 || WSAGetLastError()==WSA_IO_PENDING) 
	{
		return true;
	}

	delete pIoBuffer;

	return false;
}

void CIoContext::OnRecved(CIoBuffer* pOverlapBuf)
{
	pOverlapBuf->GetBuffer()[pOverlapBuf->GetIoSize()] = 0;
	m_pIocp->AddJob(CJob(this, pOverlapBuf));
}

bool CIoContext::AsyncSend(CIoBuffer * pIoBuffer)
{
	DWORD dwIoSize=0;
	ULONG ulFlags = MSG_PARTIAL;

	pIoBuffer->SetType(IOSent);
	pIoBuffer->m_wsabuf.buf = pIoBuffer->GetBuffer();
	pIoBuffer->m_wsabuf.len = pIoBuffer->GetLen();

	int nRet = WSASend(m_sIo, 
					pIoBuffer->GetWSABuffer(),
					1,
					&dwIoSize, 
					ulFlags,
					&pIoBuffer->m_ol, 
					NULL);

	if(nRet==0 || WSAGetLastError()==WSA_IO_PENDING) 
	{
		return true;
	}
	delete pIoBuffer;
	return false;
}

bool CIoContext::AsyncTransmitFile(const char * strFilePath, bool bKeepAlive)
{
	HANDLE hFile;
	hFile = CreateFile(strFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, 
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hFile==INVALID_HANDLE_VALUE)
		return false;

	DWORD dwFileSize = GetFileSize(hFile, NULL);

	if(dwFileSize<=0)
		return false;

	CIoBuffer * pIoBuffer = new CIoBuffer(IOFileTransmitted);
	pIoBuffer->AddData((UINT)hFile);

	pIoBuffer->m_bKeepAlive = bKeepAlive;

	BOOL b = TransmitFile(m_sIo, hFile, dwFileSize, 0, &pIoBuffer->m_ol, NULL, TF_WRITE_BEHIND);

	if(b || GetLastError()==ERROR_IO_PENDING)
	{
		return true;
	}

	CloseHandle(hFile);

	delete pIoBuffer;

	return false;
}

bool CIoContext::AsyncTransmitFile(HANDLE hFile, DWORD dwSize, bool bKeepAlive)
{
	CIoBuffer * pIoBuffer = new CIoBuffer(IOFileTransmitted);
	pIoBuffer->AddData((UINT)hFile);

	pIoBuffer->m_bKeepAlive = bKeepAlive;

	BOOL b = TransmitFile(m_sIo, hFile, dwSize, 0, &pIoBuffer->m_ol, NULL, TF_WRITE_BEHIND);

	if(b || GetLastError()==ERROR_IO_PENDING)
	{
		return true;
	}
	CloseHandle(hFile);
	delete pIoBuffer;
	return false;
}

void CIoContext::OnFileTransmitted(CIoBuffer * pIoBuffer)
{
	HANDLE hFile = *((HANDLE*)(pIoBuffer->GetBuffer()));
	CloseHandle(hFile);

	if(!pIoBuffer->m_bKeepAlive)
		Close();

	delete pIoBuffer;
}

void CIoContext::OnSent(CIoBuffer* pIoBuffer)
{
	if(!pIoBuffer->m_bKeepAlive)
	{
		Close();
	}

	delete pIoBuffer;
}

void CIoContext::OnIoFailed(CIoBuffer * pIoBuffer)
{
	switch(pIoBuffer->GetType())
	{
	case IOAccepted:
		AsyncAccept();
		delete pIoBuffer;
		return;

	case IOFileTransmitted:
		{
			HANDLE hFile = *((HANDLE*)(pIoBuffer->GetBuffer()));
			CloseHandle(hFile);
		}
		break;

	case IOSent:
		break;

	case IORecved:
		OnClosed();
		break;
	}

	delete pIoBuffer;
}

void CIoContext::OnClosed()
{
	m_pIocp->AddJob(CJob(this, NULL));
}

void CIoContext::Close()
{	
	shutdown(m_sIo, SD_BOTH);
}


string CIoContext::GetIp()
{
	return inet_ntoa(m_addrRemote.sin_addr);
}

void CIoContext::ProcessJob(CIoBuffer * pIoBuffer)
{

}

bool CIoContext::StartSvr(int iPort)
{
	sockaddr_in SvrAddr;
	SvrAddr.sin_family = AF_INET;
	SvrAddr.sin_addr.s_addr = ADDR_ANY;
	SvrAddr.sin_port = htons(iPort);
	
	if(bind(m_sIo, (struct sockaddr FAR *)&SvrAddr, sizeof(SvrAddr)))
	{
		//SetErr(ERR_SOCKET);
		return false;
	}

	if(listen(m_sIo, SOMAXCONN))
	{
		//SetErr(ERR_SOCKET);
		return false;
	}
	
	if(!AssociateWithIocp())
	{
		//SetErr(ERR_SOCKET);
		return false;
	}

	for(int i=0; i<100; i++)
		AsyncAccept();

	return true;
}

bool CIoContext::RestartSvr(int nPort)
{
	CloseSocket();
	CreateSocket();
    Init();

	return StartSvr(nPort);
}

void CIoContext::StopConThread()
{
	if(m_hConThreadId!=0)
	{
		TerminateThread(m_hConThreadId, 0);
		m_hConThreadId = 0;
	}
}