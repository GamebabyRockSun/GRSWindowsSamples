#include <tchar.h>
#include <windows.h>

int _tmain()
{
	OutputDebugStringW(L"ProcessSample��ʼ���У�\n");
	DebugBreak();//�ϵ�
	OutputDebugStringA("ProcessSample ���ANSI String\n");
	_tsystem(_T("PAUSE"));
	OutputDebugString(_T("ProcessSample�������У�\n"));
	return 0;
}