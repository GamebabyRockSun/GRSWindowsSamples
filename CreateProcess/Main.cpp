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
	
	//��������Ӧ��
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

	//��������̨Ӧ��
	//1�����ӵ����������������Ĭ������
	si.dwFlags = 0;
	StringCchPrintf(pszExe,MAX_PATH,_T("%s%s"),pszAppPath,_T("SubConsole.exe"));
	CreateProcess(pszExe,pszExe,NULL,NULL,TRUE,
		0,NULL,pszAppPath,&si,&pi);
	WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	//2���������������
	si.dwFlags = 0;
	StringCchPrintf(pszExe,MAX_PATH,_T("%s%s"),pszAppPath,_T("SubConsole.exe"));
	CreateProcess(pszExe,pszExe,NULL,NULL,TRUE,
		CREATE_NEW_CONSOLE,NULL,pszAppPath,&si,&pi);
	WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	//3������Ľ��̣�ע���ڲ���Ҫ����һ��AllocConsole��SubConsole�ڲ�û����ô�������Բ�������
	si.dwFlags = 0;
	StringCchPrintf(pszExe,MAX_PATH,_T("%s%s"),pszAppPath,_T("SubConsole.exe"));
	CreateProcess(pszExe,pszExe,NULL,NULL,TRUE,
		DETACHED_PROCESS,NULL,pszAppPath,&si,&pi);
	WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	//4���ض������롢������ļ� Input.TxtҪ��ǰ����
	sa.bInheritHandle = TRUE;	//�趨�ļ�����ɼ̳�
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

	//5���ض��򵽹ܵ�������Ping��������
	GRS_USEPRINTFA();
	StringCchPrintf(pszExe,MAX_PATH,_T("PING 127.0.0.1"));
	BYTE pBuffer[1024] = {}; // ����
	DWORD dwLen = 0;
	HANDLE hRead1, hWrite1; // �ܵ���д���
	BOOL bRet;
	sa.bInheritHandle = TRUE;
	// ���������ܵ����ܵ�����ǿɱ��̳е�
	bRet = CreatePipe(&hRead1, &hWrite1, &sa, 1024);
	if (!bRet)
	{
		GRS_PRINTF(_T("�ܵ�����ʧ��,Error Code:0x%08x\n"),GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdOutput = hWrite1;
	// �����ӽ��̣�����ping����ӽ����ǿɼ̳е�
	CreateProcess(NULL, pszExe, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, pszAppPath, &si, &pi);
	// д�˾���ѱ��̳У�������ɹرգ���Ȼ���ܵ�ʱ��������
	CloseHandle(hWrite1);
	// ���ܵ����ݣ�������Ϣ����ʾ
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