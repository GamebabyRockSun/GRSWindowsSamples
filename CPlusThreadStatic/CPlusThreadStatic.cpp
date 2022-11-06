#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#include <process.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

class ThreadClass
{
protected:
	HANDLE m_hThread;
	DWORD m_dwThreadID;
public:
	ThreadClass(BOOL bCreate = TRUE)
		:m_hThread(NULL)
		,m_dwThreadID(0)
	{
		GRS_USEPRINTF();
		GRS_PRINTF(_T("ThreadClass对象[A:0x%08x]初始化在线程[ID:0x%x]中\n")
			,this,GetCurrentThreadId());
		if(bCreate)
		{
			CreateThread();
		}
	}
	virtual ~ThreadClass()
	{

	}
protected:
	static UINT WINAPI ThreadProc(LPVOID lpParameter)
	{
		ThreadClass* pThis = (ThreadClass*)lpParameter;
		
		if(NULL != pThis)
		{
			if( pThis->InitThread() )
			{
				pThis->Run();
			}
			return pThis->ExitThread();
		}
		return 0;
	}

public:
	BOOL CreateThread(DWORD dwStackSize = 0)
	{
		GRS_USEPRINTF();
		m_hThread = (HANDLE)_beginthreadex(NULL,dwStackSize,ThreadProc,this,CREATE_SUSPENDED,(UINT*)&m_dwThreadID);
		if( -1L == (LONG)m_hThread )
		{
			GRS_PRINTF(_T("_beginthreadex发生错误"));
			return FALSE;
		}
		ResumeThread(m_hThread);

		return -1L != (LONG)m_hThread;
	}

public:
	virtual BOOL InitThread()
	{
		GRS_USEPRINTF();
		GRS_PRINTF(_T("ThreadClass对象[A:0x%08x]线程[H:0x%08x ID:0x%x]初始化\n")
			,this,m_hThread,m_dwThreadID);
		return TRUE;
	}
	virtual BOOL Run()
	{
		GRS_USEPRINTF();
		GRS_PRINTF(_T("ThreadClass对象[A:0x%08x]线程[H:0x%08x ID:0x%x]运行\n")
			,this,m_hThread,m_dwThreadID);
		return TRUE;
	}
	virtual BOOL ExitThread()
	{
		GRS_USEPRINTF();
		GRS_PRINTF(_T("ThreadClass对象[A:0x%08x]线程[H:0x%08x ID:0x%x]退出\n")
			,this,m_hThread,m_dwThreadID);
		return TRUE;
	}
};


int _tmain()
{
	ThreadClass thd;
	_tsystem(_T("PAUSE"));
	return 0;
}