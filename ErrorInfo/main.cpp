#include <tchar.h>
#include <windows.h>

int _tmain()
{
	MessageBeep(MB_ICONINFORMATION);
	FatalAppExit(0,_T("出错退出演示!"));

	TCHAR pInfo[] = _T("这个字符串不能输出!");
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pInfo,sizeof(pInfo)/sizeof(pInfo[0]),NULL,NULL);

	_tsystem(_T("PAUSE"));
	return 0;
}