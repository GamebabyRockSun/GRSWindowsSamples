// SubConsole.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <locale.h>
int _tmain(int argc, _TCHAR* argv[])
{
	_tsetlocale(LC_ALL,_T("chs"));

	_tprintf(_T("控制台子进程SubConsole输出信息......\n"));
	_tprintf(_T("请输入一个整数："));
	int iInput = 0;
	_tscanf(_T("%d"),&iInput);
	_tprintf(_T("您输入的是：%d\n"),iInput);

	_tsystem(_T("PAUSE"));

	return 0;
}

