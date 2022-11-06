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
{//线程函数
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
	//创建停止事件
	HANDLE hStopEvent = CreateEvent(NULL,TRUE,FALSE,NULL);

	//获取CPU个数
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);
	const DWORD dwCPUCnt = si.dwNumberOfProcessors;
	
	//创建线程句柄数组
	HANDLE*hThread = (HANDLE*)GRS_CALLOC(dwCPUCnt * sizeof(HANDLE));
	DWORD dwThreadID = 0;
	DWORD dwCPUIndex = 0;

	//循环创建线程
	for(DWORD i = 0; i < dwCPUCnt; i ++)
	{
		hThread[i] = CreateThread(NULL,0,ThreadFunction,hStopEvent,CREATE_SUSPENDED,&dwThreadID);
		//dwCPUIndex = 0;//dwCPUIndex = 0;
		//SetThreadAffinityMask(hThread[i],GRS_CPUINDEXTOMASK(dwCPUIndex));
		//GRS_PRINTF(_T("线程[ID:0x%x]运行在CPU(%d)上 !\n"),dwThreadID,dwCPUIndex);
		ResumeThread(hThread[i]);
		GRS_PRINTF(_T("线程[ID:0x%x]创建完毕，请查看任务管理器中对应CPU索引使用率!\n"),dwThreadID);
		_tsystem(_T("PAUSE"));
	}
	
	//如果每个CPU都安排了一个线程的话，此时应该看到所有CPU占用率几乎都是100%
	GRS_PRINTF(_T("线程全部创建完毕，请查看任务管理器中CPU使用率!\n"));
	_tsystem(_T("PAUSE"));
	
	//通知所有线程停止
	SetEvent(hStopEvent);
	
	//等待所有线程退出
	WaitForMultipleObjects(dwCPUCnt,hThread,TRUE,INFINITE);
	
	DWORD dwExitCode = 0;
	//取得线程退出代码，此例中就是循环次数，并关闭所有线程句柄
	for(DWORD i = 0; i < dwCPUCnt; i ++)
	{
		GetExitCodeThread(hThread[i],&dwExitCode);
		GRS_PRINTF(_T("线程[H:0x%08X]退出，退出码:%u!\n"),hThread[i],dwExitCode);
		CloseHandle(hThread[i]);
	}

	//释放线程句柄数组
	GRS_SAFEFREE(hThread);

	//关闭停止事件句柄
	CloseHandle(hStopEvent);
	_tsystem(_T("PAUSE"));

	return 0;
}
