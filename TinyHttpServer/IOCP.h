#pragma once

#include "IoContext.h"

//---------------------
// 完成端口服务器
//---------------------
class CIOCP  
{
public:

	HANDLE m_hCompletionPort; 
	HANDLE m_hJobCompletionPort; 
	HANDLE m_hFreeClientCompletionPort;

	deque<CJob> m_dqJobs; 
	HANDLE m_hEventThreadStopped;
	long m_nThread;  
	int m_nThreadJob;  
	int m_nThreadIo; 

	CRITICAL_SECTION m_csJob; 

	deque<CIoContext *> m_dqClosedClient;
	CRITICAL_SECTION m_csClosedClient;

	CIOCP();
	virtual ~CIOCP();

	BOOL Start(); 
	void Stop();  
	void CloseHandles(); 
	
	void StartThreads(); 
	void StopThreads();

	BOOL CreateCompletionPorts();

	static void ThreadFreeClient(LPVOID pParam);

	static void ThreadIoWorker(LPVOID pParam); 
	static void ThreadJob(LPVOID pParam); 

	CJob GetJob();           
	void ProcessJob(); 
	void AddJob(CJob Job);
	void AddClosedClient(CIoContext * pIoClient);    
	void WakenOneJobThread();
};
