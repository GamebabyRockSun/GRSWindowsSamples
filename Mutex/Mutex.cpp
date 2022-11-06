#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define THREADCOUNT 2
HANDLE g_hMutex = NULL; 
//ģ��д�����ݿ����������Դ�ķ���
DWORD WINAPI WriteToDatabase( LPVOID );

void _tmain()
{
	GRS_USEPRINTF();
	HANDLE aThread[THREADCOUNT];
	DWORD ThreadID;
	int i;

	g_hMutex = CreateMutex(	NULL,FALSE,NULL);
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);

	for( i=0; i < THREADCOUNT; i++ )
	{
		aThread[i] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)WriteToDatabase
			,NULL,CREATE_SUSPENDED,&ThreadID);
		SetThreadAffinityMask(aThread[i],1 << (i % si.dwNumberOfProcessors));
		ResumeThread(aThread[i]);
	}

	WaitForMultipleObjects(THREADCOUNT, aThread, TRUE, INFINITE);
	
	for( i=0; i < THREADCOUNT; i++ )
	{
		CloseHandle(aThread[i]);
	}

	CloseHandle(g_hMutex);

	_tsystem(_T("PAUSE"));
}

DWORD WINAPI WriteToDatabase( LPVOID lpParam )
{ 
	GRS_USEPRINTF();
	DWORD dwCount=0, dwWaitResult; 
	while( dwCount < 20 )
	{ 
		dwWaitResult = WaitForSingleObject(g_hMutex,INFINITE);
		switch (dwWaitResult) 
		{
		case WAIT_OBJECT_0: 
			__try 
			{ 
				GRS_PRINTF(_T("Timestamp(%u) Thread 0x%x writing to database...\n")
					,GetTickCount(),GetCurrentThreadId());
				Sleep(1);
				dwCount++;
			} 
			__finally
			{ 
				if (! ReleaseMutex(g_hMutex)) 
				{ 
					// Deal with error.
				} 
			} 
			break; 
		case WAIT_ABANDONED: 
			return FALSE; 
		}
	}
	return TRUE; 

}



