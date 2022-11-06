#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {};DWORD dwRead = 0;
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_SCANF() ReadConsole(GetStdHandle(STD_INPUT_HANDLE),pBuf,1,&dwRead,NULL);


int mymain()
{
	GRS_USEPRINTF();
	GRS_PRINTF(_T("按任意键继续......"));
	GRS_SCANF();
	return 0;
}