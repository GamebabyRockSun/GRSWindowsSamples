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

int _tmain()
{
	int i = 0;
	__try
	{
		__try
		{
			GRS_PRINTF(_T("1、i = %d\n"),i);
			i /= i ;
		}
		__finally
		{
			++ i;
			GRS_PRINTF(_T("2、i = %d\n"),i);
			i = 0;
			i /= i;
			GRS_PRINTF(_T("这句不会输出\n"));
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		i = 3;
		GRS_PRINTF(_T("3、i = %d\n"),i);
		__try
		{
			i /= i;
			GRS_PRINTF(_T("4、i = %d\n"),i);
		}
		__finally
		{
			i = 0;
			GRS_PRINTF(_T("5、i = %d\n"),i);
		}
	}
	_tsystem(_T("PAUSE"));
	return 0;
}