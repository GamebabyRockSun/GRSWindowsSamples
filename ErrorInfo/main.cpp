#include <tchar.h>
#include <windows.h>

int _tmain()
{
	MessageBeep(MB_ICONINFORMATION);
	FatalAppExit(0,_T("�����˳���ʾ!"));

	TCHAR pInfo[] = _T("����ַ����������!");
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pInfo,sizeof(pInfo)/sizeof(pInfo[0]),NULL,NULL);

	_tsystem(_T("PAUSE"));
	return 0;
}