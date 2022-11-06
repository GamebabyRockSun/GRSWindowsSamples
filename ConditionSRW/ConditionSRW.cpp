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
	//��д��ֻ��Ҫ��ʼ������ ����Ҫ�ͷ� ϵͳ�����д���
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
//{//�������߳�,�Թ���ʽȡ�ö���Դ���ʵ�Ȩ��
//	GRS_USEPRINTF();
//	GRS_PRINTF(_T("Timestamp[%u]:Thread[ID:0x%x] ��ȡȫ�ֱ���ֵΪ%d\n")
//		,GetTickCount(),GetCurrentThreadId(),g_iGlobalVal);
//	return 0;
//}
//
//DWORD WINAPI WriteThread( LPVOID lpParameter )
//{//д�����߳��Զ�ռ��ʽȡ��ȫ�ֱ����ķ���Ȩ
//	for(int i = 0; i <= 4321; i ++)
//	{
//		g_iGlobalVal = i;
//		//ģ��һ��ʱ���Գ��Ĵ������
//		for(int j = 0; j < 1000; j ++);
//	}
//
//	GRS_USEPRINTF();
//	GRS_PRINTF(_T("Timestamp[%u]:Thread[ID:0x%x] д������ֵΪ%d\n")
//		,GetTickCount(),GetCurrentThreadId(),g_iGlobalVal);
//	return 0;
//}

DWORD WINAPI ReadThread( LPVOID lpParameter )
{//�������߳�,�Թ���ʽȡ�ö���Դ���ʵ�Ȩ��
	__try
	{
		AcquireSRWLockShared(&g_sl);
		if( SleepConditionVariableSRW(&g_cv,&g_sl,20,CONDITION_VARIABLE_LOCKMODE_SHARED) )
		{
			GRS_USEPRINTF();
			GRS_PRINTF(_T("Timestamp[%u]:Thread[ID:0x%x] ��ȡȫ�ֱ���ֵΪ%d\n")
				,GetTickCount(),GetCurrentThreadId(),g_iGlobalVal);
		}
		else
		{
			GRS_USEPRINTF();
			GRS_PRINTF(_T("Timestamp[%u]:Thread[ID:0x%x] ��ȡȫ�ֱ���ֵ(δ�����ı�)Ϊ%d\n")
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
{//д�����߳��Զ�ռ��ʽȡ��ȫ�ֱ����ķ���Ȩ
	__try
	{
		AcquireSRWLockExclusive(&g_sl);

		GRS_USEPRINTF();
		for(int i = 0; i <= 4321; i ++)
		{
			g_iGlobalVal = i;
			SwitchToThread();
		}

		GRS_PRINTF(_T("Timestamp[%u]:Thread[ID:0x%x] д������ֵΪ%d\n")
			,GetTickCount(),GetCurrentThreadId(),g_iGlobalVal);
	}
	__finally
	{
		ReleaseSRWLockExclusive(&g_sl);
		WakeAllConditionVariable(&g_cv);
	}
	return 0;
}


