#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <locale.h>
#include <mbstring.h>

int _tmain()
{
	_tsetlocale(LC_ALL,_T("chs"));
	char pStringA[] = "123ABC汉字абвгде╔╗╚╝╠╣╦";
	printf("转换前:%s\n",pStringA);

	wchar_t pStringW[256] = {};
	MultiByteToWideChar( CP_ACP, 0, pStringA,
		strlen(pStringA)+1, pStringW,   
		sizeof(pStringW)/sizeof(pStringW[0]) );
	wprintf(L"转换后:%s\n",pStringW);

	ZeroMemory(pStringA,sizeof(pStringA)/sizeof(pStringA[0]));
	WideCharToMultiByte(CP_ACP,0,pStringW,wcslen(pStringW)
		,pStringA,sizeof(pStringA)/sizeof(pStringA[0])
		,"",NULL);
	printf("转换回来后:%s\n",pStringA);

	printf("printf输出UNICODE:%S\n",pStringW);
	wprintf(L"wprintf输出MBCS:%S\n",pStringA);

	ZeroMemory(pStringW,sizeof(pStringW)/sizeof(pStringW[0]));
	mbstowcs(pStringW,pStringA,_mbslen((const unsigned char*)pStringA));
	wprintf(L"mbstowcs转换后:%s\n",pStringW);

	ZeroMemory(pStringA,sizeof(pStringA)/sizeof(pStringA[0]));
	wcstombs(pStringA,pStringW,sizeof(pStringW)/sizeof(pStringW[0]));
	printf("wcstombs转换后:%s\n",pStringA);
	
	_tsystem(_T("PAUSE"));
	return 0;
}