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
{//线程函数
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
	//模拟一个释放CPU的操作，这个时间会被错误统计在GetTickCount计算的结果中
	Sleep(1000);

	DWORD dwEnd = GetTickCount();
	GRS_USEPRINTF();
	GRS_PRINTF(_T("线程[H:0x%08X]运行时间(GetTickCount计算):%ums\n"),GetCurrentThread(),dwEnd - dwStart);	
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
		dwCPUIndex = i;//dwCPUIndex = 0;
		SetThreadAffinityMask(hThread[i],CPUIndextoMask(dwCPUIndex));
		GRS_PRINTF(_T("线程[ID:0x%x]运行在CPU(%d)上 !\n"),dwThreadID,dwCPUIndex);
		ResumeThread(hThread[i]);
	}
	
	//如果每个CPU都安排了一个线程的话，此时应该看到所有CPU占用率几乎都是100%
	GRS_PRINTF(_T("线程全部创建完毕，请查看任务管理器中CPU使用率!\n"));
	_tsystem(_T("PAUSE"));
	
	//通知所有线程停止
	SetEvent(hStopEvent);
	
	//等待所有线程退出
	WaitForMultipleObjects(dwCPUCnt,hThread,TRUE,INFINITE);
	
	DWORD dwExitCode = 0;
	FILETIME tmCreation = {};
	FILETIME tmExit = {};
	FILETIME tmKernel = {};
	FILETIME tmUser = {};
	SYSTEMTIME tmSys = {};
	ULARGE_INTEGER bigTmp1 = {};
	ULARGE_INTEGER bigTmp2 = {};

	//取得线程退出代码，此例中就是循环次数，统计线程运行时间，并关闭所有线程句柄
	for(DWORD i = 0; i < dwCPUCnt; i ++)
	{
		GetExitCodeThread(hThread[i],&dwExitCode);
		GetThreadTimes(hThread[i],&tmCreation,&tmExit,&tmKernel,&tmUser);
		GRS_PRINTF(_T("线程[H:0x%08X]退出，退出码:%u，以下为时间统计信息：\n"),hThread[i],dwExitCode);
		
		FileTimeToLocalFileTime(&tmCreation,&tmCreation);
		FileTimeToSystemTime(&tmCreation,&tmSys);
		GRS_PRINTF(_T("\t创建时间:%2u:%02u:%02u.%04u\n")
			,tmSys.wHour,tmSys.wMinute,tmSys.wSecond,tmSys.wMilliseconds);
		
		FileTimeToLocalFileTime(&tmExit,&tmExit);
		FileTimeToSystemTime(&tmExit,&tmSys);
		GRS_PRINTF(_T("\t退出时间:%2u:%02u:%02u.%04u\n")
			,tmSys.wHour,tmSys.wMinute,tmSys.wSecond,tmSys.wMilliseconds);

		//注意FILETIME中的时间值是精确到100纳秒级的，除以10000才是毫秒值
		bigTmp1.HighPart	= tmCreation.dwHighDateTime;
		bigTmp1.LowPart		= tmCreation.dwLowDateTime;
		bigTmp2.HighPart	= tmExit.dwHighDateTime;
		bigTmp2.LowPart		= tmExit.dwLowDateTime;
		GRS_PRINTF(_T("\t间隔时间(线程存活周期):%I64dms\n")
			,(bigTmp2.QuadPart - bigTmp1.QuadPart)/ 10000);
		
		bigTmp1.HighPart = tmKernel.dwHighDateTime;
		bigTmp1.LowPart = tmKernel.dwLowDateTime;
		GRS_PRINTF(_T("\t内核模式(RING0)耗时:%I64dms!\n")
			,bigTmp1.QuadPart/10000);
		bigTmp2.HighPart = tmUser.dwHighDateTime;
		bigTmp2.LowPart = tmUser.dwLowDateTime;
		GRS_PRINTF(_T("\t用户模式(RING3)耗时:%I64dms!\n")
			,bigTmp2.QuadPart/10000);

		GRS_PRINTF(_T("\t实际占用CPU总时间:%I64dms!\n\n")
			,(bigTmp2.QuadPart + bigTmp1.QuadPart)/10000);

		CloseHandle(hThread[i]);
	}

	//释放线程句柄数组
	GRS_SAFEFREE(hThread);

	//关闭停止事件句柄
	CloseHandle(hStopEvent);
	_tsystem(_T("PAUSE"));

	return 0;
}
