#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

__declspec(dllimport) int Func_A(int iVal);
__declspec(dllimport) int Func_B(int iVal1,int iVal2);
__declspec(dllimport) int Func_C(int iVal1,int iVal2,int iVal3);

int _tmain()
{
	GRS_USEPRINTF();

	int iVal1 = 10;
	int iVal2 = 20;
	int iVal3 = 30;

	GRS_PRINTF(_T("iVal1 = %d,Func_A(iVal1) = %d\n"),iVal1,Func_A(iVal1));
	GRS_PRINTF(_T("iVal1 = %d,iVal2 = %d,Func_B(iVal1,iVal2) = %d\n")
		,iVal1,iVal2,Func_B(iVal1,iVal2));
	GRS_PRINTF(_T("iVal1 = %d,iVal2 = %d,iVal3 = %d,Func_C(iVal1,iVal2,iVal3) = %d\n")
		,iVal1,iVal2,iVal3,Func_C(iVal1,iVal2,iVal3));

	_tsystem(_T("PAUSE"));
	return 0;
}