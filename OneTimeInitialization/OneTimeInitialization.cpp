#define _WIN32_WINNT 0x0600
#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <time.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

INIT_ONCE g_InitOnce = {};
const int g_iThreadCnt = 20;
int g_iGlobalVal = 0;

BOOL CALLBACK InitHandleFunction(PINIT_ONCE InitOnce,PVOID Parameter,PVOID *lpContext);
HANDLE OpenEventHandleSync();

DWORD WINAPI ThreadFunc( LPVOID lpParameter );

int _tmain()
{
	srand((unsigned int)time(NULL));
	GRS_USEPRINTF();
	HANDLE aThread[g_iThreadCnt];
	DWORD ThreadID = 0;
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);

	//初始化一次性初始化对象,不用释放,系统会自动释放
	InitOnceInitialize(&g_InitOnce);

	for( int i=0; i < g_iThreadCnt; i++ )
	{
		aThread[i] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadFunc
				,NULL,CREATE_SUSPENDED,&ThreadID);
		SetThreadAffinityMask(aThread[i],1 << (i % si.dwNumberOfProcessors));
		ResumeThread(aThread[i]);
	}

	WaitForMultipleObjects(g_iThreadCnt, aThread, TRUE, INFINITE);

	for( int i=0; i < g_iThreadCnt; i++ )
	{
		CloseHandle(aThread[i]);
	}

	_tsystem(_T("PAUSE"));

	return 0;
}

BOOL CALLBACK InitHandleFunction(PINIT_ONCE InitOnce,PVOID Parameter,PVOID *lpContext)
{	
	//一次性初始化创建一个Event对象,比使用OpenEvent方法要安全
	//因为多线程时,若一个线程判定句柄为空,并准备开始创建,被切换
	//而另一个线程也判定为空开始创建,此时会创建多个Event对象
	//而多创建的Event句柄有可能丢失
	HANDLE hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	if ( NULL == hEvent )
	{
		return FALSE;
	}
	else
	{
		GRS_USEPRINTF();
		GRS_PRINTF(_T("线程[ID:0x%x]创建了Event句柄[0x%08X]\n"),GetCurrentThreadId(),hEvent);
		*lpContext = hEvent;
		return TRUE;
	}
	return FALSE;
}

HANDLE OpenEventHandleSync()
{
	PVOID lpContext = NULL;
	BOOL  bStatus = InitOnceExecuteOnce(&g_InitOnce,(PINIT_ONCE_FN)InitHandleFunction,NULL,&lpContext);
	if( bStatus )
	{
		return (HANDLE)lpContext;
	}
	else
	{
		return (INVALID_HANDLE_VALUE);
	}
}

DWORD WINAPI ThreadFunc( LPVOID lpParameter )
{
	//TCHAR pEventName[] = _T("MyEvent");
	////此时不好决断是用CreateEvent还是OpenEvent?
	//HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS,FALSE,pEventName);
	//if(NULL == hEvent)
	//{
	//	GRS_USEPRINTF();
	//	GRS_PRINTF(_T("线程[ID:0x%x]判定Event对象\"%s\"不存在,执行创建\n")
	//		,GetCurrentThreadId(),pEventName);

	//	SwitchToThread();//切换下线程或模拟一个耗时操作

	//	hEvent = CreateEvent(NULL,TRUE,TRUE,pEventName);
	//	if(NULL != hEvent)
	//	{
	//		GRS_USEPRINTF();
	//		GRS_PRINTF(_T("线程[ID:0x%x]创建Event\"%s\"句柄[0x%08X]\n")
	//			,GetCurrentThreadId(),pEventName,hEvent);
	//	}
	//	else
	//	{
	//		GRS_USEPRINTF();
	//		GRS_PRINTF(_T("线程[ID:0x%x]创建Event\"%s\"句柄失败Err:[0x%08X]\n")
	//			,GetCurrentThreadId(),pEventName,GetLastError());

	//	}
	//}
	//else
	//{
	//	GRS_USEPRINTF();
	//	GRS_PRINTF(_T("线程[ID:0x%x]获得Event句柄[0x%08X]\n"),GetCurrentThreadId(),hEvent);
	//}

	//return (DWORD)hEvent;

	HANDLE hEvent = OpenEventHandleSync();
	if(NULL != hEvent)
	{
		GRS_USEPRINTF();
		GRS_PRINTF(_T("线程[ID:0x%x]获得Event句柄[0x%08X]\n"),GetCurrentThreadId(),hEvent);
	}
	return (DWORD)hEvent;	
}