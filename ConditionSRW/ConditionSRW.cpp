#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <time.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

const int g_iThreadCnt = 20;
int g_iGlobalVal = 0;

SRWLOCK			   g_sl = {};
CONDITION_VARIABLE g_cv = {};

DWORD WINAPI ReadThread( LPVOID lpParameter );
DWORD WINAPI WriteThread( LPVOID lpParameter );

int _tmain()
{
	srand((unsigned int)time(NULL));
	//读写锁只需要初始化即可 不需要释放 系统会自行处理
	InitializeSRWLock(&g_sl);
	InitializeConditionVariable(&g_cv);

	GRS_USEPRINTF();
	HANDLE aThread[g_iThreadCnt];
	DWORD ThreadID = 0;
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);

	for( int i=0; i < g_iThreadCnt; i++ )
	{
		if( 0 == rand() % 2 )
		{
			aThread[i] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)WriteThread
				,NULL,CREATE_SUSPENDED,&ThreadID);
		}
		else
		{
			aThread[i] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ReadThread
				,NULL,CREATE_SUSPENDED,&ThreadID);
		}
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

//DWORD WINAPI ReadThread( LPVOID lpParameter )
//{//读数据线程,以共享方式取得对资源访问的权限
//	GRS_USEPRINTF();
//	GRS_PRINTF(_T("Timestamp[%u]:Thread[ID:0x%x] 读取全局变量值为%d\n")
//		,GetTickCount(),GetCurrentThreadId(),g_iGlobalVal);
//	return 0;
//}
//
//DWORD WINAPI WriteThread( LPVOID lpParameter )
//{//写数据线程以独占方式取得全局变量的访问权
//	for(int i = 0; i <= 4321; i ++)
//	{
//		g_iGlobalVal = i;
//		//模拟一个时间稍长的处理过程
//		for(int j = 0; j < 1000; j ++);
//	}
//
//	GRS_USEPRINTF();
//	GRS_PRINTF(_T("Timestamp[%u]:Thread[ID:0x%x] 写入数据值为%d\n")
//		,GetTickCount(),GetCurrentThreadId(),g_iGlobalVal);
//	return 0;
//}

DWORD WINAPI ReadThread( LPVOID lpParameter )
{//读数据线程,以共享方式取得对资源访问的权限
	__try
	{
		AcquireSRWLockShared(&g_sl);
		if( SleepConditionVariableSRW(&g_cv,&g_sl,20,CONDITION_VARIABLE_LOCKMODE_SHARED) )
		{
			GRS_USEPRINTF();
			GRS_PRINTF(_T("Timestamp[%u]:Thread[ID:0x%x] 读取全局变量值为%d\n")
				,GetTickCount(),GetCurrentThreadId(),g_iGlobalVal);
		}
		else
		{
			GRS_USEPRINTF();
			GRS_PRINTF(_T("Timestamp[%u]:Thread[ID:0x%x] 读取全局变量值(未发生改变)为%d\n")
				,GetTickCount(),GetCurrentThreadId(),g_iGlobalVal);
		}
	}
	__finally
	{
		ReleaseSRWLockShared(&g_sl);
	}
	return 0;
}

DWORD WINAPI WriteThread( LPVOID lpParameter )
{//写数据线程以独占方式取得全局变量的访问权
	__try
	{
		AcquireSRWLockExclusive(&g_sl);

		GRS_USEPRINTF();
		for(int i = 0; i <= 4321; i ++)
		{
			g_iGlobalVal = i;
			SwitchToThread();
		}

		GRS_PRINTF(_T("Timestamp[%u]:Thread[ID:0x%x] 写入数据值为%d\n")
			,GetTickCount(),GetCurrentThreadId(),g_iGlobalVal);
	}
	__finally
	{
		ReleaseSRWLockExclusive(&g_sl);
		WakeAllConditionVariable(&g_cv);
	}
	return 0;
}


