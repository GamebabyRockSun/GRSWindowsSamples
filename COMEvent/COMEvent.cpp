#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

void _tmain( )
{
	GRS_USEPRINTF();

	HANDLE hCom;
	OVERLAPPED o;
	BOOL fSuccess;
	DWORD dwEvtMask;

	hCom = CreateFile( _T("COM1"),
		GENERIC_READ | GENERIC_WRITE,
		0,    // exclusive access 
		NULL, // default security attributes 
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL 
		);

	if (hCom == INVALID_HANDLE_VALUE) 
	{
		// Handle the error. 
		GRS_PRINTF(_T("CreateFile failed with error %d.\n"), GetLastError());
		return;
	}

	// Set the event mask. 

	fSuccess = SetCommMask(hCom, EV_CTS | EV_DSR);

	if (!fSuccess) 
	{
		// Handle the error. 
		GRS_PRINTF(_T("SetCommMask failed with error %d.\n"), GetLastError());
		return;
	}

	// Create an event object for use by WaitCommEvent. 

	o.hEvent = CreateEvent(NULL,TRUE,FALSE,	NULL);

	// Initialize the rest of the OVERLAPPED structure to zero.
	o.Internal = 0;
	o.InternalHigh = 0;
	o.Offset = 0;
	o.OffsetHigh = 0;

	if (WaitCommEvent(hCom, &dwEvtMask, &o)) 
	{
		if (dwEvtMask & EV_DSR) 
		{
			// To do.
		}

		if (dwEvtMask & EV_CTS) 
		{
			// To do. 
		}
	}
	else
	{
		DWORD dwRet = GetLastError();
		if( ERROR_IO_PENDING == dwRet)
		{
			GRS_PRINTF(_T("I/O is pending...\n"));

			// To do.
		}
		else 
			GRS_PRINTF(_T("Wait failed with error %d.\n"), GetLastError());
	}
}
