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

int _tmain(int argc, TCHAR *argv[])
{
	GRS_USEPRINTF();

	DCB dcb;
	HANDLE hCom;
	BOOL fSuccess;
	TCHAR *pcCommPort = _T("COM2");

	hCom = CreateFile( pcCommPort,GENERIC_READ | GENERIC_WRITE,
		0,    // must be opened with exclusive-access
		NULL, // default security attributes
		OPEN_EXISTING, // must use OPEN_EXISTING
		0,    // not overlapped I/O
		NULL  // hTemplate must be NULL for comm devices
		);
	if (hCom == INVALID_HANDLE_VALUE) 
	{
		// Handle the error.
		GRS_PRINTF (_T("CreateFile failed with error %d.\n"), GetLastError());
		return (1);
	}

	// Build on the current configuration, and skip setting the size
	// of the input and output buffers with SetupComm.

	SecureZeroMemory(&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);
	fSuccess = GetCommState(hCom, &dcb);

	if (!fSuccess) 
	{
		// Handle the error.
		GRS_PRINTF (_T("GetCommState failed with error %d.\n"), GetLastError());
		return (2);
	}

	// Fill in DCB: 57,600 bps, 8 data bits, no parity, and 1 stop bit.

	dcb.BaudRate = CBR_57600;     // set the baud rate
	dcb.ByteSize = 8;             // data size, xmit, and rcv
	dcb.Parity = NOPARITY;        // no parity bit
	dcb.StopBits = ONESTOPBIT;    // one stop bit

	fSuccess = SetCommState(hCom, &dcb);

	if (!fSuccess) 
	{
		// Handle the error.
		GRS_PRINTF (_T("SetCommState failed with error %d.\n"), GetLastError());
		return (3);
	}
	_tprintf (_T("Serial port %s successfully reconfigured.\n"), pcCommPort);
	_tsystem(_T("PAUSE"));
	return (0);
}
