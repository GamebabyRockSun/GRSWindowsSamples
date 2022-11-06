#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_REALLOC(p,sz)	HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#define GRS_FREE(p)			HeapFree(GetProcessHeap(),0,p)
#define GRS_MSIZE(p)		HeapSize(GetProcessHeap(),0,p)
#define GRS_MVALID(p)		HeapValidate(GetProcessHeap(),0,p)

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_USEPRINTFA() CHAR pABuf[1024] = {}
#define GRS_PRINTFA(...) \
	StringCchPrintfA(pABuf,1024,__VA_ARGS__);\
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE),pABuf,lstrlenA(pABuf),NULL,NULL);

#define GRS_USEPRINTFW() WCHAR pWBuf[1024] = {}
#define GRS_PRINTFW(...) \
	StringCchPrintfW(pWBuf,1024,__VA_ARGS__);\
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),pWBuf,lstrlenW(pWBuf),NULL,NULL);


LPTSTR GetAppPath(LPTSTR pszPathBuf);
DWORD OnCreateThreadDebugEvent(const LPDEBUG_EVENT,HANDLE,HANDLE);
DWORD OnCreateProcessDebugEvent(const LPDEBUG_EVENT,HANDLE,HANDLE);
DWORD OnExitThreadDebugEvent(const LPDEBUG_EVENT,HANDLE,HANDLE);
DWORD OnExitProcessDebugEvent(const LPDEBUG_EVENT,HANDLE,HANDLE);
DWORD OnLoadDllDebugEvent(const LPDEBUG_EVENT,HANDLE,HANDLE);
DWORD OnUnloadDllDebugEvent(const LPDEBUG_EVENT,HANDLE,HANDLE);
DWORD OnOutputDebugStringEvent(const LPDEBUG_EVENT,HANDLE,HANDLE);
DWORD OnRipEvent(const LPDEBUG_EVENT,HANDLE,HANDLE);
void EnterDebugLoop(const LPDEBUG_EVENT pDbgEvent,HANDLE,HANDLE);

int _tmain()
{
	GRS_USEPRINTF();

	TCHAR psAppPath[MAX_PATH + 1] = {};
	TCHAR psDebugApp[MAX_PATH + 1] = {};
	GetAppPath(psAppPath);
	StringCchCopy(psDebugApp,MAX_PATH,psAppPath);
	StringCbCat(psDebugApp,MAX_PATH,_T("ProcessSample.exe"));

	STARTUPINFO si = {};
	PROCESS_INFORMATION pi = {};

	BOOL bRet = CreateProcess(NULL,psDebugApp,NULL,NULL,FALSE,
		CREATE_NEW_CONSOLE | DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS,
		NULL,NULL,&si,&pi);
	if( !bRet )
	{
		GRS_PRINTF(_T("被调试的应用程序%s无法启动\n"),psDebugApp);
		return 1;
	}
	
	DEBUG_EVENT dbgEvent = {};
	EnterDebugLoop(&dbgEvent,pi.hProcess,pi.hThread);
	
	_tsystem(_T("PAUSE"));
	return 0;
}

LPTSTR GetAppPath(LPTSTR pszPathBuf)
{
	DWORD dwLen = 0;
	if(dwLen = ::GetModuleFileName(NULL,pszPathBuf,MAX_PATH))
	{	
		DWORD i = dwLen;
		for(; i > 0; i -- )
		{
			if( '\\' == pszPathBuf[i] )
			{
				pszPathBuf[i + 1] = '\0';
				break;
			}
		}
		return pszPathBuf;
	}
	return NULL;
}

void EnterDebugLoop(const LPDEBUG_EVENT pDbgEvent,HANDLE hProcess,HANDLE hMainThread)
{
	DWORD dwContinueStatus = DBG_CONTINUE; // exception continuation 
	GRS_USEPRINTF();
	while(WaitForDebugEvent(pDbgEvent, INFINITE))
	{ 
		switch (pDbgEvent->dwDebugEventCode) 
		{ 
		case EXCEPTION_DEBUG_EVENT: //第一次捕获的异常
			switch(pDbgEvent->u.Exception.ExceptionRecord.ExceptionCode)
			{ 
			case EXCEPTION_ACCESS_VIOLATION: 
				break;
			case EXCEPTION_BREAKPOINT: 
				{
					GRS_PRINTF(_T("被调试进程(Process ID:%u)中线程(Thread ID:%u)产生断点\n")
						,pDbgEvent->dwProcessId,pDbgEvent->dwThreadId);
					//后面可以暂停断点线程，读取对应的Context等
					HANDLE hThread = OpenThread(THREAD_ALL_ACCESS,FALSE,pDbgEvent->dwThreadId);
					if( NULL != hThread )
					{
						SuspendThread(hThread);
						GRS_PRINTF(_T("线程被暂停执行\n"));
						_tsystem(_T("PAUSE"));
						GRS_PRINTF(_T("线程继续执行\n"));
						ResumeThread(hThread);
					}
				}
				break;
			case EXCEPTION_DATATYPE_MISALIGNMENT: 
				break;
			case EXCEPTION_SINGLE_STEP: 
				break;
			case DBG_CONTROL_C: 
				break;
			default:
				break;
			} 

		case CREATE_THREAD_DEBUG_EVENT: 
			dwContinueStatus = OnCreateThreadDebugEvent(pDbgEvent,hProcess,hMainThread);
			break;
		case CREATE_PROCESS_DEBUG_EVENT: 
			dwContinueStatus = OnCreateProcessDebugEvent(pDbgEvent,hProcess,hMainThread);
			break;
		case EXIT_THREAD_DEBUG_EVENT: 
			dwContinueStatus = OnExitThreadDebugEvent(pDbgEvent,hProcess,hMainThread);
			break;
		case EXIT_PROCESS_DEBUG_EVENT: 
			dwContinueStatus = OnExitProcessDebugEvent(pDbgEvent,hProcess,hMainThread);
			break;
		case LOAD_DLL_DEBUG_EVENT: 
			dwContinueStatus = OnLoadDllDebugEvent(pDbgEvent,hProcess,hMainThread);
			break;
		case UNLOAD_DLL_DEBUG_EVENT: 
			dwContinueStatus = OnUnloadDllDebugEvent(pDbgEvent,hProcess,hMainThread);
			break;
		case OUTPUT_DEBUG_STRING_EVENT: 
			dwContinueStatus = OnOutputDebugStringEvent(pDbgEvent,hProcess,hMainThread);
			break;
		case RIP_EVENT:
			dwContinueStatus = OnRipEvent(pDbgEvent,hProcess,hMainThread);
			break;
		}
		if( DBG_EXCEPTION_NOT_HANDLED != dwContinueStatus && DBG_CONTINUE != dwContinueStatus )
		{
			break;//跳出调试循环
		}
		ContinueDebugEvent(pDbgEvent->dwProcessId,pDbgEvent->dwThreadId,dwContinueStatus);
		ZeroMemory(pDbgEvent,sizeof(DEBUG_EVENT));
	}
}

DWORD OnCreateThreadDebugEvent(const LPDEBUG_EVENT pDE,HANDLE hProcess,HANDLE hMainThread)
{
	GRS_USEPRINTF();
	GRS_PRINTF(_T("创建线程(Thread ID:%u)\n"),pDE->dwThreadId);
	return DBG_CONTINUE;
}
DWORD OnCreateProcessDebugEvent(const LPDEBUG_EVENT pDE,HANDLE hProcess,HANDLE hMainThread)
{
	GRS_USEPRINTF();
	GRS_PRINTF(_T("创建进程(Process ID:%u)\n"),pDE->dwProcessId);
	return DBG_CONTINUE;
}
DWORD OnExitThreadDebugEvent(const LPDEBUG_EVENT pDE,HANDLE hProcess,HANDLE hMainThread)
{
	GRS_USEPRINTF();
	GRS_PRINTF(_T("线程(Thread ID:%u)退出(ExitCode:%u)\n")
		,pDE->dwThreadId,pDE->u.ExitThread.dwExitCode);
	return DBG_CONTINUE;
}
DWORD OnExitProcessDebugEvent(const LPDEBUG_EVENT pDE,HANDLE hProcess,HANDLE hMainThread)
{
	GRS_USEPRINTF();
	GRS_PRINTF(_T("进程(Process ID:%u)退出(ExitCode:%u)\n")
		,pDE->dwProcessId,pDE->u.ExitProcess.dwExitCode);
	return 0;
}
DWORD OnLoadDllDebugEvent(const LPDEBUG_EVENT pDE,HANDLE hProcess,HANDLE hMainThread)
{
	if( pDE->u.LoadDll.lpImageName )
	{//UNICODE
		WCHAR**ppName = (WCHAR**)pDE->u.LoadDll.lpImageName;
		GRS_USEPRINTFW();
		WCHAR* pOutputStr = (WCHAR*)GRS_CALLOC(MAX_PATH * sizeof(WCHAR));
		if(ReadProcessMemory(hProcess
			,*ppName
			,pOutputStr,MAX_PATH * sizeof(WCHAR),NULL))
		{
			GRS_PRINTFW(L"加载DLL(%s)\n",pOutputStr);
		}
		GRS_FREE(pOutputStr);
	}
	else
	{//ANSI
		GRS_USEPRINTFA();
		CHAR* pOutputStr = (CHAR*)GRS_CALLOC(MAX_PATH * sizeof(CHAR));
		if(ReadProcessMemory(hProcess,pDE->u.LoadDll.lpImageName
			,pOutputStr,MAX_PATH,NULL))
		{
			GRS_PRINTFA("加载DLL(%s)\n",pOutputStr);
		}
		GRS_FREE(pOutputStr);
	}
	return DBG_CONTINUE;
}
DWORD OnUnloadDllDebugEvent(const LPDEBUG_EVENT pDE,HANDLE hProcess,HANDLE hMainThread)
{
	//GRS_USEPRINTF();
	//if(NULL != pDE->LoadDll.lpImageName)
	//{
	//	GRS_PRINTF(_T("卸载DLL(%s)\n"),(TCHAR*)pDE->LoadDll.lpImageName);
	//}
	return DBG_CONTINUE;
}
DWORD OnOutputDebugStringEvent(const LPDEBUG_EVENT pDE,HANDLE hProcess,HANDLE hMainThread)
{
	if( pDE->u.DebugString.fUnicode )
	{//UNICODE
		GRS_USEPRINTFW();
		WCHAR* pOutputStr = (WCHAR*)GRS_CALLOC(pDE->u.DebugString.nDebugStringLength * sizeof(WCHAR));
		if(ReadProcessMemory(hProcess
			,pDE->u.DebugString.lpDebugStringData
			,pOutputStr,pDE->u.DebugString.nDebugStringLength,NULL))
		{
			GRS_PRINTFW(L"被调试进程(ID:%u)输出UNICODE:%s\n"
				,pDE->dwProcessId,pOutputStr);
		}
		GRS_FREE(pOutputStr);
	}
	else
	{//ANSI
		GRS_USEPRINTFA();
		CHAR* pOutputStr = (CHAR*)GRS_CALLOC(pDE->u.DebugString.nDebugStringLength * sizeof(CHAR));
		if(ReadProcessMemory(hProcess
			,pDE->u.DebugString.lpDebugStringData
			,pOutputStr,pDE->u.DebugString.nDebugStringLength,NULL))
		{
			GRS_PRINTFA("被调试进程(ID:%u)输出ANSI串:%s\n"
				,pDE->dwProcessId,pOutputStr);
		}
		GRS_FREE(pOutputStr);
	}
	return DBG_CONTINUE;
}
DWORD OnRipEvent(const LPDEBUG_EVENT pDE,HANDLE hProcess,HANDLE hMainThread)
{
	return DBG_CONTINUE;
}