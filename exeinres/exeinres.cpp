#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include "resource.h"
#pragma comment(lib,"Winmm.lib")

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

int _tmain()
{
	GRS_USEPRINTF();
	GRS_PRINTF(_T("����һ����Ϊ��ԴǶ�뵽����Exe�е���Exe\n"));
	PlaySound(MAKEINTRESOURCE(IDR_WAVE1),GetModuleHandle(NULL),SND_RESOURCE|SND_ASYNC);
	_tsystem(_T("PAUSE"));
	return 0;
}