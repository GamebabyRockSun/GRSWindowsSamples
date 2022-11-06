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

void EHFun(int & i)
{
	try
	{
		i /= i;
		throw i;
	}
	catch (int& e)
	{
		e = 1;
		GRS_PRINTF(_T("���ﲻ�Ჶ���쳣\n"));
	}
	catch(...)
	{
		GRS_PRINTF(_T("���ﲶ���쳣\n"));
	}
}
int _tmain()
{
	int i = 0;
	__try
	{
		EHFun(i);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		i = 25;
		GRS_PRINTF(_T("__except�鲶���쳣\n"));
	}
	_tsystem(_T("PAUSE"));
	return 0;
}