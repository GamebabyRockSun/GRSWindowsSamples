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

#define GRS_CPUINDEXTOMASK(dwCPUIndex) (1 << (dwCPUIndex))

DWORD WINAPI ThreadFunction( LPVOID lpParam ) 
{//�̺߳���
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
	GRS_USEPRINTF();
	GRS_PRINTF(_T("Rand Avg is %f\n"),fRandAvg);	
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
		//dwCPUIndex = 0;//dwCPUIndex = 0;
		//SetThreadAffinityMask(hThread[i],GRS_CPUINDEXTOMASK(dwCPUIndex));
		//GRS_PRINTF(_T("�߳�[ID:0x%x]������CPU(%d)�� !\n"),dwThreadID,dwCPUIndex);
		ResumeThread(hThread[i]);
		GRS_PRINTF(_T("�߳�[ID:0x%x]������ϣ���鿴����������ж�ӦCPU����ʹ����!\n"),dwThreadID);
		_tsystem(_T("PAUSE"));
	}
	
	//���ÿ��CPU��������һ���̵߳Ļ�����ʱӦ�ÿ�������CPUռ���ʼ�������100%
	GRS_PRINTF(_T("�߳�ȫ��������ϣ���鿴�����������CPUʹ����!\n"));
	_tsystem(_T("PAUSE"));
	
	//֪ͨ�����߳�ֹͣ
	SetEvent(hStopEvent);
	
	//�ȴ������߳��˳�
	WaitForMultipleObjects(dwCPUCnt,hThread,TRUE,INFINITE);
	
	DWORD dwExitCode = 0;
	//ȡ���߳��˳����룬�����о���ѭ�����������ر������߳̾��
	for(DWORD i = 0; i < dwCPUCnt; i ++)
	{
		GetExitCodeThread(hThread[i],&dwExitCode);
		GRS_PRINTF(_T("�߳�[H:0x%08X]�˳����˳���:%u!\n"),hThread[i],dwExitCode);
		CloseHandle(hThread[i]);
	}

	//�ͷ��߳̾������
	GRS_SAFEFREE(hThread);

	//�ر�ֹͣ�¼����
	CloseHandle(hStopEvent);
	_tsystem(_T("PAUSE"));

	return 0;
}
