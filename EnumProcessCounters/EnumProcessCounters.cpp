#define WIN32_LEAN_AND_MEAN 1

#include <windows.h>
#include <winperf.h>
#include <malloc.h>
#include <stdio.h>
#include <tchar.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <tchar.h>

#pragma comment(lib, "pdh.lib")

int _tmain ()
{
	PDH_STATUS  pdhStatus               = ERROR_SUCCESS;
	LPTSTR      szCounterListBuffer     = NULL;
	DWORD       dwCounterListSize       = 0;
	LPTSTR      szInstanceListBuffer    = NULL;
	DWORD       dwInstanceListSize      = 0;
	LPTSTR      szThisInstance          = NULL;
	LPTSTR		szThisCounter			= NULL;


	// Determine the required buffer size for the data. 
	pdhStatus = PdhEnumObjectItems (
		NULL,                   // real time source
		NULL,                   // local machine
		TEXT("Process"),        // object to enumerate
		szCounterListBuffer,    // pass NULL and 0
		&dwCounterListSize,     // to get length required
		szInstanceListBuffer,   // buffer size 
		&dwInstanceListSize,    // 
		PERF_DETAIL_WIZARD,     // counter detail level
		0); 

	if (pdhStatus == PDH_MORE_DATA) 
	{
		// Allocate the buffers and try the call again.
		szCounterListBuffer = (LPTSTR)malloc (
			(dwCounterListSize * sizeof (TCHAR)));
		szInstanceListBuffer = (LPTSTR)malloc (
			(dwInstanceListSize * sizeof (TCHAR)));

		if ((szCounterListBuffer != NULL) &&
			(szInstanceListBuffer != NULL)) 
		{
			pdhStatus = PdhEnumObjectItems (
				NULL,                 // real time source
				NULL,                 // local machine
				TEXT("Process"),      // object to enumerate
				szCounterListBuffer,  // buffer to receive counter list
				&dwCounterListSize, 
				szInstanceListBuffer, // buffer to receive instance list 
				&dwInstanceListSize,    
				PERF_DETAIL_WIZARD,   // counter detail level
				0);

			if (pdhStatus == ERROR_SUCCESS) 
			{
				_tprintf (TEXT("\nEnumerating Processes:"));

				// Walk the instance list. The list can contain one
				// or more null-terminated strings. The last string 
				// is followed by a second null-terminator.
				for (szThisInstance = szInstanceListBuffer;
					*szThisInstance != 0;
					szThisInstance += lstrlen(szThisInstance) + 1) 
				{
					_tprintf (TEXT("\n  %s"), szThisInstance);
				}

				_tprintf (TEXT("\n"));
				_tprintf (TEXT("Enumerating Counters:"));

				// Walk the instance list. The list can contain one
				// or more null-terminated strings. The last string 
				// is followed by a second null-terminator.
				for (szThisCounter = szCounterListBuffer;
					*szThisCounter != 0;
					szThisCounter += lstrlen(szThisCounter) + 1) 
				{
					_tprintf (TEXT("\n  %s"), szThisCounter);
				}

			}
			else 
			{
				_tprintf(TEXT("\nPdhEnumObjectItems failed with %ld."), pdhStatus);
			}
			_tprintf (TEXT("\n"));
		} 
		else 
		{
			_tprintf (TEXT("\nUnable to allocate buffers\n"));
			pdhStatus = ERROR_OUTOFMEMORY;
		}

		if (szCounterListBuffer != NULL) 
			free (szCounterListBuffer);

		if (szInstanceListBuffer != NULL) 
			free (szInstanceListBuffer);
	} 
	else 
	{
		_tprintf(TEXT("\nPdhEnumObjectItems failed with %ld.\n"), pdhStatus);
	}
	_tsystem(_T("PAUSE"));
	return pdhStatus;
}