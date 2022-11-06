#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <time.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

DWORD* g_pdwAnyData = NULL;
CRITICAL_SECTION g_csAnyData = {};
//模拟对这个全局的数据指针进行分配写入释放等操作
void AllocData();
void WriteData();
void ReadData();
void FreeData();

DWORD WINAPI ThreadProc( LPVOID lpParameter );

#define THREADCOUNT 20
#define THREADACTCNT 20		//每个线程执行20次动作

void _tmain()
{
	srand((unsigned int)time(NULL));
	if (!InitializeCriticalSectionAndSpinCount(&g_csAnyData,0x80000400) ) 
	{
		return;
	}

	HANDLE aThread[THREADCOUNT];
	DWORD ThreadID = 0;
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);

	for( int i=0; i < THREADCOUNT; i++ )
	{
		aThread[i] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadProc
			,NULL,CREATE_SUSPENDED,&ThreadID);
		SetThreadAffinityMask(aThread[i],1 << (i % si.dwNumberOfProcessors));
		ResumeThread(aThread[i]);
	}

	WaitForMultipleObjects(THREADCOUNT, aThread, TRUE, INFINITE);

	for( int i=0; i < THREADCOUNT; i++ )
	{
		CloseHandle(aThread[i]);
	}
	
	DeleteCriticalSection(&g_csAnyData);

	GRS_SAFEFREE(g_pdwAnyData);
	_tsystem(_T("PAUSE"));
}

DWORD WINAPI ThreadProc( LPVOID lpParameter )
{
	GRS_USEPRINTF();
	int iAct = rand()%4;
	int iActCnt = THREADACTCNT;
	while(iActCnt --)
	{
		switch(iAct)
		{
		case 0:
			{
				AllocData();
			}
			break;
		case 1:
			{
				FreeData();
			}
			break;
		case 2:
			{
				WriteData();
			}
			break;
		case 3:
			{
				ReadData();
			}
			break;
		}
		iAct = rand()%4;
	}

	return iActCnt;
}

void AllocData()
{
	EnterCriticalSection(&g_csAnyData);
	GRS_USEPRINTF();
	__try
	{
		GRS_PRINTF(_T("Thread(ID:0x%x) Alloc Data\n\t"),GetCurrentThreadId());

		if(NULL == g_pdwAnyData)
		{
			g_pdwAnyData = (DWORD*)GRS_CALLOC(sizeof(DWORD));
			GRS_PRINTF(_T("g_pdwAnyData is NULL Alloc it Address(0x%08x)\n"),g_pdwAnyData);
		}
		else
		{
			GRS_PRINTF(_T("g_pdwAnyData isn't NULL Can't Alloc it Address(0x%08x)\n"),g_pdwAnyData);
		}
	}
	__finally
	{
		LeaveCriticalSection(&g_csAnyData);
	}
}

void WriteData()
{
	EnterCriticalSection(&g_csAnyData);
	GRS_USEPRINTF();
	__try
	{
		GRS_PRINTF(_T("Thread(ID:0x%x) Write Data\n\t"),GetCurrentThreadId());

		if(NULL != g_pdwAnyData)
		{
			*g_pdwAnyData = rand();
			GRS_PRINTF(_T("g_pdwAnyData isn't NULL Write Val(%u)\n"),*g_pdwAnyData);
		}
		else
		{
			GRS_PRINTF(_T("g_pdwAnyData is NULL Can't Write\n"));
		}
	}
	__finally
	{
		LeaveCriticalSection(&g_csAnyData);
	}
}

void ReadData()
{
	EnterCriticalSection(&g_csAnyData);
	GRS_USEPRINTF();
	__try
	{
		GRS_PRINTF(_T("Thread(ID:0x%x) Read Data\n\t"),GetCurrentThreadId());

		if(NULL != g_pdwAnyData)
		{
			GRS_PRINTF(_T("g_pdwAnyData isn't NULL Read Val(%u)\n"),*g_pdwAnyData);
		}
		else
		{
			GRS_PRINTF(_T("g_pdwAnyData is NULL Can't Read\n"));
		}
	}
	__finally
	{
		LeaveCriticalSection(&g_csAnyData);
	}
}

void FreeData()
{
	EnterCriticalSection(&g_csAnyData);
	GRS_USEPRINTF();
	__try
	{
//		GRS_PRINTF(_T("Thread(ID:0x%x) Free Data\n\t"),GetCurrentThreadId());

		if(NULL != g_pdwAnyData)
		{
			GRS_PRINTF(_T("g_pdwAnyData isn't NULL Free it\n"));
			GRS_SAFEFREE(g_pdwAnyData);
		}
		else
		{
			GRS_PRINTF(_T("g_pdwAnyData is NULL Can't Free\n"));
		}
	}
	__finally
	{
		LeaveCriticalSection(&g_csAnyData);
	}
}


