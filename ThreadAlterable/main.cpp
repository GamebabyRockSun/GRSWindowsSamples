#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

VOID CALLBACK APCFunction(ULONG_PTR dwParam)
{
	int i = dwParam;
	GRS_USEPRINTF();
	GRS_PRINTF(_T("%d APC Function Run!\n"),i);
	Sleep(20);
}

int _tmain()
{
	//为主线程添加APC函数
	for(int i = 0;i < 100;i++)
	{
		QueueUserAPC(APCFunction,GetCurrentThread(),i);
	}
	//通过SleepEx 进入“可警告状态”,注释后，APC函数不会被调用
	SleepEx(10000,TRUE);

	_tsystem(_T("PAUSE"));
	return 0;
}