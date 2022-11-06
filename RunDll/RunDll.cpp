#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

extern "C"
{
	VOID WriteData(int iVal,DWORD dwVal);
	VOID ReadData(int* piVal,DWORD* pdwVal);
};

int _tmain()
{
	int iVal = 0;
	DWORD dwVal = 0;

	ReadData(&iVal,&dwVal);
	iVal = 18;
	dwVal = 24;
	WriteData(iVal,dwVal);
	ReadData(&iVal,&dwVal);

	return 0;
}


