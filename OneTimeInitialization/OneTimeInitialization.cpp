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

	//��ʼ��һ���Գ�ʼ������,�����ͷ�,ϵͳ���Զ��ͷ�
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
	//һ���Գ�ʼ������һ��Event����,��ʹ��OpenEvent����Ҫ��ȫ
	//��Ϊ���߳�ʱ,��һ���߳��ж����Ϊ��,��׼����ʼ����,���л�
	//����һ���߳�Ҳ�ж�Ϊ�տ�ʼ����,��ʱ�ᴴ�����Event����
	//���ഴ����Event����п��ܶ�ʧ
	HANDLE hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	if ( NULL == hEvent )
	{
		return FALSE;
	}
	else
	{
		GRS_USEPRINTF();
		GRS_PRINTF(_T("�߳�[ID:0x%x]������Event���[0x%08X]\n"),GetCurrentThreadId(),hEvent);
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
	////��ʱ���þ�������CreateEvent����OpenEvent?
	//HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS,FALSE,pEventName);
	//if(NULL == hEvent)
	//{
	//	GRS_USEPRINTF();
	//	GRS_PRINTF(_T("�߳�[ID:0x%x]�ж�Event����\"%s\"������,ִ�д���\n")
	//		,GetCurrentThreadId(),pEventName);

	//	SwitchToThread();//�л����̻߳�ģ��һ����ʱ����

	//	hEvent = CreateEvent(NULL,TRUE,TRUE,pEventName);
	//	if(NULL != hEvent)
	//	{
	//		GRS_USEPRINTF();
	//		GRS_PRINTF(_T("�߳�[ID:0x%x]����Event\"%s\"���[0x%08X]\n")
	//			,GetCurrentThreadId(),pEventName,hEvent);
	//	}
	//	else
	//	{
	//		GRS_USEPRINTF();
	//		GRS_PRINTF(_T("�߳�[ID:0x%x]����Event\"%s\"���ʧ��Err:[0x%08X]\n")
	//			,GetCurrentThreadId(),pEventName,GetLastError());

	//	}
	//}
	//else
	//{
	//	GRS_USEPRINTF();
	//	GRS_PRINTF(_T("�߳�[ID:0x%x]���Event���[0x%08X]\n"),GetCurrentThreadId(),hEvent);
	//}

	//return (DWORD)hEvent;

	HANDLE hEvent = OpenEventHandleSync();
	if(NULL != hEvent)
	{
		GRS_USEPRINTF();
		GRS_PRINTF(_T("�߳�[ID:0x%x]���Event���[0x%08X]\n"),GetCurrentThreadId(),hEvent);
	}
	return (DWORD)hEvent;	
}