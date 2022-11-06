#include <tchar.h>
#include <windows.h>

#define GRS_PRINTF(...) \
	{\
	TCHAR pBuf[1024];\
	wsprintf(pBuf,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);\
	}

int _tmain()
{
	const int iCount = 128;
	PVOID pFrames[iCount] = {};     
	CaptureStackBackTrace(0, iCount, pFrames, NULL);

	for(int i = 0; i < iCount && pFrames[i]; i++)
	{
		GRS_PRINTF(_T("Õ»¶¥Ë÷Òý:%3d\tº¯ÊýµØÖ·:0x%08X\n"), i, pFrames[i]);
	}
	_tsystem(_T("PAUSE"));
	return 0;
}

