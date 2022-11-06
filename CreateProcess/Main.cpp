#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_USEPRINTFA() CHAR pBufA[1024] = {}
#define GRS_PRINTFA(...) \
	StringCchPrintfA(pBufA,1024,__VA_ARGS__);\
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE),pBufA,lstrlenA(pBufA),NULL,NULL);

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

VOID GetAppPath(LPTSTR pszBuffer)
{
	DWORD dwLen = 0;
	if(0 == (dwLen = ::GetModuleFileName(NULL,pszBuffer,MAX_PATH)))
	{
		return;			
	}
	DWORD i = dwLen;
	for(; i > 0; i -- )
	{
		if( '\\' == pszBuffer[i] )
		{
			pszBuffer[i + 1] = '\0';
			break;
		}
	}
}

int _tmain()
{
	GRS_USEPRINTF();
	TCHAR pszAppPath[MAX_PATH + 1] = {};
	TCHAR pszExe[MAX_PATH + 1] = {};
	GetAppPath(pszAppPath);
		
	STARTUPINFO si = {sizeof(STARTUPINFO)};
	PROCESS_INFORMATION pi = {};
	SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES)};
	sa.bInheritHandle = FALSE;
	
	//启动窗口应用
	si.dwFlags = STARTF_USESIZE | STARTF_USESHOWWINDOW | STARTF_USEPOSITION;
	si.wShowWindow = SW_SHOWNORMAL;
	si.dwXSize = 400;
	si.dwYSize = 300;
	si.dwX = 312;
	si.dwY = 234;
	StringCchPrintf(pszExe,MAX_PATH,_T("%s%s"),pszAppPath,_T("SubGUI.exe"));
	CreateProcess(pszExe,pszExe,&sa,&sa,FALSE,
		0,NULL,pszAppPath,&si,&pi);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	_tsystem(_T("PAUSE"));

	//启动控制台应用
	//1、附加到本进程输入输出，默认特性
	si.dwFlags = 0;
	StringCchPrintf(pszExe,MAX_PATH,_T("%s%s"),pszAppPath,_T("SubConsole.exe"));
	CreateProcess(pszExe,pszExe,NULL,NULL,TRUE,
		0,NULL,pszAppPath,&si,&pi);
	WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	//2、独立的输入输出
	si.dwFlags = 0;
	StringCchPrintf(pszExe,MAX_PATH,_T("%s%s"),pszAppPath,_T("SubConsole.exe"));
	CreateProcess(pszExe,pszExe,NULL,NULL,TRUE,
		CREATE_NEW_CONSOLE,NULL,pszAppPath,&si,&pi);
	WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	//3、分离的进程（注意内部需要调用一下AllocConsole，SubConsole内部没有这么做，所以不正常）
	si.dwFlags = 0;
	StringCchPrintf(pszExe,MAX_PATH,_T("%s%s"),pszAppPath,_T("SubConsole.exe"));
	CreateProcess(pszExe,pszExe,NULL,NULL,TRUE,
		DETACHED_PROCESS,NULL,pszAppPath,&si,&pi);
	WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	//4、重定向输入、输出到文件 Input.Txt要提前创建
	sa.bInheritHandle = TRUE;	//设定文件句柄可继承
	si.dwFlags = STARTF_USESTDHANDLES;
	StringCchPrintf(pszExe,MAX_PATH,_T("%s%s"),pszAppPath,_T("Output.txt"));
	HANDLE hOutput = CreateFile(pszExe,GENERIC_WRITE,0,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	StringCchPrintf(pszExe,MAX_PATH,_T("%s%s"),pszAppPath,_T("Input.txt"));
	HANDLE hInput = CreateFile(pszExe,GENERIC_READ,0,&sa,OPEN_EXISTING,0,NULL);
	si.hStdOutput = hOutput;
	si.hStdInput = hInput;
	StringCchPrintf(pszExe,MAX_PATH,_T("%s%s"),pszAppPath,_T("SubConsole.exe"));
	CreateProcess(pszExe,pszExe,NULL,NULL,TRUE,
		DETACHED_PROCESS,NULL,pszAppPath,&si,&pi);
	WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(hOutput);
	CloseHandle(hInput);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	//5、重定向到管道，接收Ping命令的输出
	GRS_USEPRINTFA();
	StringCchPrintf(pszExe,MAX_PATH,_T("PING 127.0.0.1"));
	BYTE pBuffer[1024] = {}; // 缓存
	DWORD dwLen = 0;
	HANDLE hRead1, hWrite1; // 管道读写句柄
	BOOL bRet;
	sa.bInheritHandle = TRUE;
	// 创建匿名管道，管道句柄是可被继承的
	bRet = CreatePipe(&hRead1, &hWrite1, &sa, 1024);
	if (!bRet)
	{
		GRS_PRINTF(_T("管道创建失败,Error Code:0x%08x\n"),GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdOutput = hWrite1;
	// 创建子进程，运行ping命令，子进程是可继承的
	CreateProcess(NULL, pszExe, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, pszAppPath, &si, &pi);
	// 写端句柄已被继承，本地则可关闭，不然读管道时将被阻塞
	CloseHandle(hWrite1);
	// 读管道内容，并用消息框显示
	dwLen = 1000;
	DWORD dwRead = 0;
	while (ReadFile(hRead1, pBuffer, dwLen, &dwRead, NULL))
	{
		if ( 0 == dwRead )
		{
			break;
		}
		GRS_PRINTFA("%s\n",pBuffer);
		ZeroMemory(pBuffer,1024);
	}
	GRS_PRINTF(_T("Read Completion!\n"));
	CloseHandle(hRead1);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	
	_tsystem(_T("PAUSE"));
	return 0;
}