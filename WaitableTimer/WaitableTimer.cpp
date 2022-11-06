#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

int _tmain()
{
	HANDLE hTimer = NULL;
	LARGE_INTEGER liDueTime;
	GRS_USEPRINTF();

	liDueTime.QuadPart=-10000000LL; //100ƒ…√Îµ•Œª

	// Create a waitable timer.
	hTimer = CreateWaitableTimer(NULL, TRUE, _T("WaitableTimer"));
	if (NULL == hTimer)
	{
		GRS_PRINTF(_T("CreateWaitableTimer failed (%d)\n"), GetLastError());
		return 1;
	}

	GRS_PRINTF(_T("Waiting for 1 seconds...\n"));

	// Set a timer to wait for 1 seconds.
	if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0))
	{
		GRS_PRINTF(_T("SetWaitableTimer failed (%d)\n"), GetLastError());
		return 2;
	}

	// Wait for the timer.
	if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0)
	{
		GRS_PRINTF(_T("WaitForSingleObject failed (%d)\n"), GetLastError());
	}
	else
	{
		GRS_PRINTF(_T("Timer was signaled.\n"));
	}

	_tsystem(_T("PAUSE"));

	return 0;
}
