#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_OUTPUT(s) WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),s,lstrlen(s),NULL,NULL)

int _tmain()
{
	TCHAR pszFileName[] = _T("C:\\Test.txt");
	if(INVALID_HANDLE_VALUE == CreateFile(pszFileName,GENERIC_READ,0,NULL,OPEN_EXISTING,NULL,NULL) )
	{
		DWORD dwLastError = GetLastError();
		LPTSTR lpMsgBuf = NULL;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 	FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,dwLastError,GetUserDefaultLangID(),(LPTSTR)&lpMsgBuf,0, NULL );
		if(NULL != lpMsgBuf )
		{
			TCHAR pMsg[1024];
			StringCchPrintf(pMsg,1024,_T("打开文件\"%s\"失败,原因:%s\n"),pszFileName,lpMsgBuf);
			GRS_OUTPUT(pMsg);
			HeapFree(GetProcessHeap(),0,lpMsgBuf);
		}
	}

	_tsystem(_T("PAUSE"));
	return 0;
}
