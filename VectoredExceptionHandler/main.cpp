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

int g_iVal = 0;

LONG CALLBACK VEH1(struct _EXCEPTION_POINTERS* pEI)
{
	GRS_OUTPUT(_T("VEH1\n"));
	return EXCEPTION_CONTINUE_SEARCH;
}

LONG CALLBACK VEH2(struct _EXCEPTION_POINTERS* pEI)
{
	GRS_OUTPUT(_T("VEH2\n"));
	//if(EXCEPTION_INT_DIVIDE_BY_ZERO == pEI->ExceptionRecord->ExceptionCode)
	//{
	//	g_iVal = 25;
	//	return EXCEPTION_CONTINUE_EXECUTION;
	//}
	return EXCEPTION_CONTINUE_SEARCH;
}

LONG CALLBACK VEH3(struct _EXCEPTION_POINTERS* pEI)
{

	GRS_OUTPUT(_T("VEH3\n"));
	return EXCEPTION_CONTINUE_SEARCH;
}

LONG EHFilter(PEXCEPTION_POINTERS pEI)
{
	if(EXCEPTION_INT_DIVIDE_BY_ZERO == pEI->ExceptionRecord->ExceptionCode)
	{
		g_iVal = 34;
		return EXCEPTION_EXECUTE_HANDLER;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

void Fun1(int iVal)
{
	__try
	{
		GRS_PRINTF(_T("Fun1 g_iVal = %d iVal = %d\n"),g_iVal,iVal);
		iVal /= g_iVal;
		GRS_PRINTF(_T("Fun1 g_iVal = %d iVal = %d\n"),g_iVal,iVal);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		GRS_OUTPUT(_T("Fun1 __except块执行，程序将退出\n"));
		_tsystem(_T("PAUSE"));
		ExitProcess(1);
	}
}

int _tmain()
{
	PVOID pVEH1 = AddVectoredExceptionHandler(0,VEH1);
	PVOID pVEH2 = AddVectoredExceptionHandler(0,VEH2);
	PVOID pVEH3 = AddVectoredExceptionHandler(0,VEH3);

	int i = 0;	
	__try
	{
		Fun1(g_iVal);
	}
	__except(EHFilter(GetExceptionInformation()))
	{
		GRS_OUTPUT(_T("main __except executed!\n"));
	}

	RemoveVectoredExceptionHandler(pVEH1);
	RemoveVectoredExceptionHandler(pVEH2);
	RemoveVectoredExceptionHandler(pVEH3);

	_tsystem(_T("PAUSE"));
	return 0;
}