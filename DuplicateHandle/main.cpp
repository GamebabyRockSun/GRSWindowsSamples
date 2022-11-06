#include <tchar.h>
#include <windows.h>

DWORD CALLBACK ThreadProc(PVOID pvParam);

int _tmain()
{
	HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);
	HANDLE hMutexDup, hThread;
	DWORD dwThreadId;

	DuplicateHandle(GetCurrentProcess(),hMutex,GetCurrentProcess(),&hMutexDup, 
		0,FALSE,DUPLICATE_SAME_ACCESS);

	hThread = CreateThread(NULL, 0, ThreadProc,(LPVOID) hMutexDup, 0, &dwThreadId);
	CloseHandle(hMutex);

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	return 0;
}

DWORD CALLBACK ThreadProc(PVOID pvParam)
{
	HANDLE hMutex = (HANDLE)pvParam;
	CloseHandle(hMutex);
	return 0;
}