// SubConsole.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <locale.h>
int _tmain(int argc, _TCHAR* argv[])
{
	_tsetlocale(LC_ALL,_T("chs"));

	_tprintf(_T("����̨�ӽ���SubConsole�����Ϣ......\n"));
	_tprintf(_T("������һ��������"));
	int iInput = 0;
	_tscanf(_T("%d"),&iInput);
	_tprintf(_T("��������ǣ�%d\n"),iInput);

	_tsystem(_T("PAUSE"));

	return 0;
}

