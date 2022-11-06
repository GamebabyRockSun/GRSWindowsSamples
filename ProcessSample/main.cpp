#include <tchar.h>
#include <windows.h>

int _tmain()
{
	OutputDebugStringW(L"ProcessSample开始运行！\n");
	DebugBreak();//断点
	OutputDebugStringA("ProcessSample 输出ANSI String\n");
	_tsystem(_T("PAUSE"));
	OutputDebugString(_T("ProcessSample结束运行！\n"));
	return 0;
}