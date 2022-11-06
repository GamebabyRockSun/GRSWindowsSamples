#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <locale.h>

int _tmain()
{
	_tsetlocale(LC_ALL,_T("chs"));

	const size_t szBuflen = 1024;
	TCHAR pAnyStr[] = _T("123ABCabc’‘«ÆÀÔ¿Ó");
	TCHAR pBuff[szBuflen];
	size_t szLen = 0;
	
	StringCchPrintf(pBuff,szBuflen,_T("WriteConsole OutPut:%s\n"),pAnyStr);
	StringCchLength(pBuff,szBuflen,&szLen);
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuff,szLen,NULL,NULL);
	
	ZeroMemory(pBuff,szBuflen*sizeof(TCHAR));
	_tcscpy_s(pBuff,szBuflen,pAnyStr);
	_tprintf_s(_T("_tprintf_s OutPut:%s\n"),pBuff);

	_tsystem(_T("PAUSE"));
	return 0;
}