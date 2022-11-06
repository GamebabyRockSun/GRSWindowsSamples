#include "StdAfx.h"
#include "IOCP.h"
#include <process.h>

CIOCP * m_pIocp;

CIOCP::CIOCP()
{
	InitializeCriticalSection(&m_csJob); 
	InitializeCriticalSection(&m_csClosedClient); 
	m_pIocp = this;
	m_hCompletionPort = m_hJobCompletionPort = NULL;
	m_hEventThreadStopped = CreateEvent(NULL, TRUE, TRUE, NULL);
	
	m_nThread = 0;
	m_nThreadJob = 10;
	
	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	m_nThreadIo = SysInfo.dwNumberOfProcessors * 2 + 2;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);
}

CIOCP::~CIOCP()
{
	Stop();
	DeleteCriticalSection(&m_csJob);
	DeleteCriticalSection(&m_csClosedClient);
	CloseHandle(m_hEventThreadStopped);
	WSACleanup();
}

void CIOCP::CloseHandles()
{
	if(m_hJobCompletionPort)
	{
		CloseHandle(m_hJobCompletionPort);
		m_hJobCompletionPort = NULL;
	}

	if(m_hCompletionPort)
	{
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}
}

void CIOCP::Stop()
{
	StopThreads();
	CloseHandles();
}

BOOL CIOCP::Start()
{
	if(!CreateCompletionPorts())
	{
		return FALSE;
	}

	StartThreads();

	return TRUE;
}

void CIOCP::StartThreads()
{
	int i;

	for(i=0; i<m_nThreadJob; i++)
	{	
		_beginthread(ThreadJob, 0, (LPVOID)this);
	}

	for(i=0; i<m_nThreadIo; i++)
	{	
		_beginthread(ThreadIoWorker, 0, (LPVOID)this);
	}

	_beginthread(ThreadFreeClient, 0, (LPVOID)this);
}

void CIOCP::StopThreads()
{
	ResetEvent(m_hEventThreadStopped);

	int i;

	PostQueuedCompletionStatus(m_hFreeClientCompletionPort, 0, 0, NULL);

	for(i=0; i<m_nThreadJob; i++)
	{
		PostQueuedCompletionStatus(m_hJobCompletionPort, 0, 0, NULL);
	}

	for(i=0; i<m_nThreadIo; i++)
	{
		PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, NULL);
	}

	if(m_nThread>0)
	{
		WaitForSingleObject(m_hEventThreadStopped, 3000);
	}
}

void CIOCP::ThreadFreeClient(LPVOID pParam)
{
	CIOCP * pThis = (CIOCP*)pParam;

	DWORD dwIoSize;
	DWORD dwKey;
	LPOVERLAPPED lpOverlapped;
	BOOL bIORet;
	HANDLE hCompletionPort = pThis->m_hFreeClientCompletionPort;
	InterlockedIncrement(&pThis->m_nThread);

	deque<CIoContext *> & dqClosedClient = pThis->m_dqClosedClient;
    CRITICAL_SECTION & csClosedClient = pThis->m_csClosedClient;
	
	int i;
	int Num = 0;

	while(1)
	{
		bIORet = GetQueuedCompletionStatus(
			hCompletionPort,
			&dwIoSize,
			&dwKey, 
			&lpOverlapped, 300);

		if(!bIORet && dwKey==0 && GetLastError()!=ERROR_TIMEOUT)
		{
			break;
		}

		EnterCriticalSection(&csClosedClient);
		if(Num>0)
		{
			for(i=0; i<Num; i++)
			{
				delete dqClosedClient[i];
			}
			dqClosedClient.erase(dqClosedClient.begin(), dqClosedClient.begin()+Num);
		}
		Num = dqClosedClient.size();
		LeaveCriticalSection(&csClosedClient);
	}

	EnterCriticalSection(&csClosedClient);

	for(i=0; i<dqClosedClient.size(); i++)
	{
		delete dqClosedClient[i];
	}
    dqClosedClient.erase(dqClosedClient.begin(), dqClosedClient.begin()+Num);

	LeaveCriticalSection(&csClosedClient);

	InterlockedDecrement(&pThis->m_nThread);
	if(pThis->m_nThread<=0)
	{
		SetEvent(pThis->m_hEventThreadStopped);
	}
}

void CIOCP::ThreadIoWorker(LPVOID pParam)
{
	CIOCP * pThis = (CIOCP*)pParam;
	DWORD dwIoSize;
	CIoContext* lpIoContext;
	CIoBuffer *pOverlapBuff;
	LPOVERLAPPED lpOverlapped;
	BOOL bIORet;
	HANDLE hCompletionPort = pThis->m_hCompletionPort;

	InterlockedIncrement(&pThis->m_nThread);
	
	while(1)
	{
		bIORet = GetQueuedCompletionStatus(
			hCompletionPort,
			&dwIoSize,
			(LPDWORD) (&lpIoContext), 
			&lpOverlapped, INFINITE);

		if(bIORet && lpIoContext!=NULL)
		{
			pOverlapBuff=CONTAINING_RECORD(lpOverlapped, CIoBuffer, m_ol);
			pOverlapBuff->SetIoSize(dwIoSize);
			lpIoContext->ProcessIoMsg(pOverlapBuff);			
		}
		else if(!bIORet)
		{
			pOverlapBuff=CONTAINING_RECORD(lpOverlapped, CIoBuffer, m_ol);
			lpIoContext->OnIoFailed(pOverlapBuff);
		}
		else
		{
			break;
		}
	}

	InterlockedDecrement(&pThis->m_nThread);
	if(pThis->m_nThread<=0)
	{
		SetEvent(pThis->m_hEventThreadStopped);
	}
}

void CIOCP::ThreadJob(LPVOID pParam)
{
	CIOCP * pThis = (CIOCP*)pParam;

	DWORD dwIoSize;
	DWORD dwKey;
	LPOVERLAPPED lpOverlapped;
	BOOL bIORet;
	HANDLE hCompletionPort = pThis->m_hJobCompletionPort;
	InterlockedIncrement(&pThis->m_nThread);
	
	while(1)
	{
		bIORet = GetQueuedCompletionStatus(
			hCompletionPort,
			&dwIoSize,
			&dwKey, 
			&lpOverlapped, INFINITE);

		if(!bIORet || dwKey==0)
			break;

		pThis->ProcessJob();
	}

	InterlockedDecrement(&pThis->m_nThread);

	if(pThis->m_nThread<=0)
	{
		SetEvent(pThis->m_hEventThreadStopped);
	}
}

void CIOCP::ProcessJob()
{
	CJob Job = GetJob();

	if(Job.IsNullIoBuffer())
	{
		AddClosedClient(Job.pIoContext);
	}
	else
	{
		Job.pIoContext->ProcessJob(Job.pIoBuffer);
		delete Job.pIoBuffer;
	}
}

CJob CIOCP::GetJob()
{
	CJob Job;
	EnterCriticalSection(&m_csJob); 
	Job = m_dqJobs[0];
	m_dqJobs.pop_front();

	LeaveCriticalSection(&m_csJob);	

	return Job;
}

void CIOCP::AddJob(CJob Job)
{
	EnterCriticalSection(&m_csJob);
	m_dqJobs.push_back(Job);
	LeaveCriticalSection(&m_csJob);

	WakenOneJobThread();
}

void CIOCP::AddClosedClient(CIoContext * pIoClient)
{
	EnterCriticalSection(&m_csClosedClient);
	m_dqClosedClient.push_back(pIoClient);
	LeaveCriticalSection(&m_csClosedClient);
}

void CIOCP::WakenOneJobThread()
{
	PostQueuedCompletionStatus(m_hJobCompletionPort, 0, 1, NULL);
}

BOOL CIOCP::CreateCompletionPorts()
{
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if(m_hCompletionPort==NULL)
		return FALSE;

	m_hJobCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if(m_hJobCompletionPort==NULL)
		return FALSE;

	m_hFreeClientCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if(m_hFreeClientCompletionPort==NULL)
		return FALSE;

	return TRUE;
}