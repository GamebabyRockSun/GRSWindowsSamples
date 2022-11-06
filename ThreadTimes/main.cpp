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

#define CPUIndextoMask(dwCPUIndex) (1 << (dwCPUIndex))

DWORD WINAPI ThreadFunction( LPVOID lpParam ) 
{//�̺߳���
	DWORD dwStart = GetTickCount();

	HANDLE hEvent = (HANDLE)lpParam;
	srand((unsigned int)time(NULL));
	float fRandAvg = 0.0f;
	float fCnt = 0.0f;
	while(TRUE)
	{
		fCnt += 1.0f;
		fRandAvg += (float)rand();
		fRandAvg /= fCnt;
		if(WAIT_OBJECT_0 == WaitForSingleObject(hEvent,0))
		{
			break;
		}
	}	
	//ģ��һ���ͷ�CPU�Ĳ��������ʱ��ᱻ����ͳ����GetTickCount����Ľ����
	Sleep(1000);

	DWORD dwEnd = GetTickCount();
	GRS_USEPRINTF();
	GRS_PRINTF(_T("�߳�[H:0x%08X]����ʱ��(GetTickCount����):%ums\n"),GetCurrentThread(),dwEnd - dwStart);	
	return (DWORD)fCnt; 
} 

int _tmain()
{
	GRS_USEPRINTF();
	//����ֹͣ�¼�
	HANDLE hStopEvent = CreateEvent(NULL,TRUE,FALSE,NULL);

	//��ȡCPU����
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);
	const DWORD dwCPUCnt = si.dwNumberOfProcessors;
	
	//�����߳̾������
	HANDLE*hThread = (HANDLE*)GRS_CALLOC(dwCPUCnt * sizeof(HANDLE));
	DWORD dwThreadID = 0;
	DWORD dwCPUIndex = 0;

	//ѭ�������߳�
	for(DWORD i = 0; i < dwCPUCnt; i ++)
	{
		hThread[i] = CreateThread(NULL,0,ThreadFunction,hStopEvent,CREATE_SUSPENDED,&dwThreadID);
		dwCPUIndex = i;//dwCPUIndex = 0;
		SetThreadAffinityMask(hThread[i],CPUIndextoMask(dwCPUIndex));
		GRS_PRINTF(_T("�߳�[ID:0x%x]������CPU(%d)�� !\n"),dwThreadID,dwCPUIndex);
		ResumeThread(hThread[i]);
	}
	
	//���ÿ��CPU��������һ���̵߳Ļ�����ʱӦ�ÿ�������CPUռ���ʼ�������100%
	GRS_PRINTF(_T("�߳�ȫ��������ϣ���鿴�����������CPUʹ����!\n"));
	_tsystem(_T("PAUSE"));
	
	//֪ͨ�����߳�ֹͣ
	SetEvent(hStopEvent);
	
	//�ȴ������߳��˳�
	WaitForMultipleObjects(dwCPUCnt,hThread,TRUE,INFINITE);
	
	DWORD dwExitCode = 0;
	FILETIME tmCreation = {};
	FILETIME tmExit = {};
	FILETIME tmKernel = {};
	FILETIME tmUser = {};
	SYSTEMTIME tmSys = {};
	ULARGE_INTEGER bigTmp1 = {};
	ULARGE_INTEGER bigTmp2 = {};

	//ȡ���߳��˳����룬�����о���ѭ��������ͳ���߳�����ʱ�䣬���ر������߳̾��
	for(DWORD i = 0; i < dwCPUCnt; i ++)
	{
		GetExitCodeThread(hThread[i],&dwExitCode);
		GetThreadTimes(hThread[i],&tmCreation,&tmExit,&tmKernel,&tmUser);
		GRS_PRINTF(_T("�߳�[H:0x%08X]�˳����˳���:%u������Ϊʱ��ͳ����Ϣ��\n"),hThread[i],dwExitCode);
		
		FileTimeToLocalFileTime(&tmCreation,&tmCreation);
		FileTimeToSystemTime(&tmCreation,&tmSys);
		GRS_PRINTF(_T("\t����ʱ��:%2u:%02u:%02u.%04u\n")
			,tmSys.wHour,tmSys.wMinute,tmSys.wSecond,tmSys.wMilliseconds);
		
		FileTimeToLocalFileTime(&tmExit,&tmExit);
		FileTimeToSystemTime(&tmExit,&tmSys);
		GRS_PRINTF(_T("\t�˳�ʱ��:%2u:%02u:%02u.%04u\n")
			,tmSys.wHour,tmSys.wMinute,tmSys.wSecond,tmSys.wMilliseconds);

		//ע��FILETIME�е�ʱ��ֵ�Ǿ�ȷ��100���뼶�ģ�����10000���Ǻ���ֵ
		bigTmp1.HighPart	= tmCreation.dwHighDateTime;
		bigTmp1.LowPart		= tmCreation.dwLowDateTime;
		bigTmp2.HighPart	= tmExit.dwHighDateTime;
		bigTmp2.LowPart		= tmExit.dwLowDateTime;
		GRS_PRINTF(_T("\t���ʱ��(�̴߳������):%I64dms\n")
			,(bigTmp2.QuadPart - bigTmp1.QuadPart)/ 10000);
		
		bigTmp1.HighPart = tmKernel.dwHighDateTime;
		bigTmp1.LowPart = tmKernel.dwLowDateTime;
		GRS_PRINTF(_T("\t�ں�ģʽ(RING0)��ʱ:%I64dms!\n")
			,bigTmp1.QuadPart/10000);
		bigTmp2.HighPart = tmUser.dwHighDateTime;
		bigTmp2.LowPart = tmUser.dwLowDateTime;
		GRS_PRINTF(_T("\t�û�ģʽ(RING3)��ʱ:%I64dms!\n")
			,bigTmp2.QuadPart/10000);

		GRS_PRINTF(_T("\tʵ��ռ��CPU��ʱ��:%I64dms!\n\n")
			,(bigTmp2.QuadPart + bigTmp1.QuadPart)/10000);

		CloseHandle(hThread[i]);
	}

	//�ͷ��߳̾������
	GRS_SAFEFREE(hThread);

	//�ر�ֹͣ�¼����
	CloseHandle(hStopEvent);
	_tsystem(_T("PAUSE"));

	return 0;
}
