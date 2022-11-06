
#pragma once

#include "IoBuffer.h"

//-------------------------------
// 每个客户端分配一个上下文内容
//-------------------------------

class CIoContext
{
public:

	SOCKET m_sIo;
	SOCKADDR_IN m_addrRemote;
	HANDLE m_hConThreadId; 

	CIoContext();
	CIoContext(SOCKET sock);

	virtual ~CIoContext();

	void Init();

	bool AssociateWithIocp(); 
	virtual void ProcessIoMsg(CIoBuffer * pIoBuffer); 
	virtual void ProcessJob(CIoBuffer * pIoBuffer);

	bool CreateSocket();

	void CloseSocket();
	void Close();
	bool AsyncRecv();  
	bool AsyncAccept();   
	bool AsyncSend(CIoBuffer * IoBuffer); 
	bool AsyncTransmitFile(const char * strFilePath, bool bKeepAlive=true); 
	bool AsyncTransmitFile(HANDLE hFile, DWORD dwSize=0, bool bKeepAlive=true); 
	bool AsyncConnect(const char * szIp, int nPort);

	virtual void OnAccepted(CIoBuffer * pIoBuffer); 
	void OnRecved(CIoBuffer * pIoBuffer);
	void OnSent(CIoBuffer * pIoBuffer); 
	virtual void OnClosed();
	void OnFileTransmitted(CIoBuffer * pIoBuffer);
	virtual void OnConnected(CIoBuffer * pIoBuffer);
	virtual void OnIoFailed(CIoBuffer * pIoBuffer);

	string GetIp();  
	void StopConThread();

	static DWORD WINAPI ThreadConnect(LPVOID pParam); 

	bool StartSvr(int iPort);
	bool RestartSvr(int nPort);
};

//-----------------------------
// 提交给后台线程池的工作内容
//-----------------------------
class CJob
{
public:
	CIoContext * pIoContext;
	CIoBuffer * pIoBuffer;

	CJob() {};

	CJob(CIoContext * IoContext, CIoBuffer * IoBuffer)
	{
		pIoContext = IoContext;
		pIoBuffer = IoBuffer;
	};

	inline bool IsNull() {return pIoContext==NULL;};
	inline bool IsNullIoBuffer() {return pIoBuffer==NULL;};
};
