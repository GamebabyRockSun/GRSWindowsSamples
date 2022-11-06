#include <tchar.h>
#include <windows.h>
#include <atlconv.h>
#include <stdio.h>
#include <locale.h>

int _tmain()
{
	_tsetlocale(LC_ALL,_T("chs"));

	USES_CONVERSION;
	WCHAR wStr[] = L"’‘«ÆÀÔ¿Ó123ABCxyz";
	wprintf(L"UNICODE Str = %s\n",wStr);
	printf("W2A Convert = %s\n",W2A(wStr));
	_tprintf(_T("W2T Convert = %s\n"),W2T(wStr));

	CHAR aStr[] = "123ABCxyz’‘«ÆÀÔ¿Ó";
	printf("MBCS Str = %s\n",aStr);
	wprintf(L"A2W Convert = %s\n",A2W(aStr));
	_tprintf(_T("A2T Convert = %s\n"),A2T(aStr));	

	_tsystem(_T("PAUSE"));

}