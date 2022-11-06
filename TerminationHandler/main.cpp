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
	

int test()
{
	int iNum = 0;
	GRS_OUTPUT(_T("Begin Test\n"));
	__try
	{
		GRS_OUTPUT(_T("进入__try\n"));
		__leave;
		GRS_OUTPUT(_T("这句不会输出\n"));
		iNum = 25;
	}
	__finally
	{
		GRS_OUTPUT(_T("进入__finally\n"));
		iNum = 322;
	}
	GRS_OUTPUT(_T("End Test\n"));
	return iNum;
}

int _tmain()
{
	int iNum = 0;
	__try
	{
		iNum = test();
		GRS_PRINTF(_T("1、iNum = %d\n"),iNum);
		iNum = 0;

		iNum /= iNum;
		GRS_PRINTF(_T("2、iNum = %d\n"),iNum);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		GRS_OUTPUT(_T("进入__except\n"));
		iNum = ~0;
	}
	iNum /= iNum;
	GRS_PRINTF(_T("3、iNum = %d\n"),iNum);
	_tsystem(_T("PAUSE"));
	return 0;
}
