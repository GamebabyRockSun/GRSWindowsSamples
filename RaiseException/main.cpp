#include <tchar.h>
#include <windows.h>


#define GRS_OUTPUT(s) WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),s,lstrlen(s),NULL,NULL)
#define GRS_PRINTF(...) \
	{\
	const UINT nBufLen = 1024;\
	TCHAR pBuf[nBufLen];\
	wsprintf(pBuf,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);\
	}


DWORD FilterFunction() 
{ 
	GRS_OUTPUT(_T("1"));                 
	return EXCEPTION_EXECUTE_HANDLER; 
} 

VOID main(VOID) 
{ 
	__try 
	{ 
		__try 
		{ 
			RaiseException(1,0,0, NULL);           
		} 
		__finally 
		{ 
			GRS_OUTPUT(_T("2"));
		} 
	} 
	__except ( FilterFunction() ) 
	{ 
		GRS_OUTPUT(_T("3\n"));
	} 

	_tsystem(_T("PAUSE"));
}

