#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <locale.h>
#include <mbstring.h>

int _tmain()
{
	_tsetlocale(LC_ALL,_T("chs"));
	char pStringA[] = "123ABC���֧ѧҧӧԧէ֨X�[�^�a�d�g�j";
	printf("ת��ǰ:%s\n",pStringA);

	wchar_t pStringW[256] = {};
	MultiByteToWideChar( CP_ACP, 0, pStringA,
		strlen(pStringA)+1, pStringW,   
		sizeof(pStringW)/sizeof(pStringW[0]) );
	wprintf(L"ת����:%s\n",pStringW);

	ZeroMemory(pStringA,sizeof(pStringA)/sizeof(pStringA[0]));
	WideCharToMultiByte(CP_ACP,0,pStringW,wcslen(pStringW)
		,pStringA,sizeof(pStringA)/sizeof(pStringA[0])
		,"",NULL);
	printf("ת��������:%s\n",pStringA);

	printf("printf���UNICODE:%S\n",pStringW);
	wprintf(L"wprintf���MBCS:%S\n",pStringA);

	ZeroMemory(pStringW,sizeof(pStringW)/sizeof(pStringW[0]));
	mbstowcs(pStringW,pStringA,_mbslen((const unsigned char*)pStringA));
	wprintf(L"mbstowcsת����:%s\n",pStringW);

	ZeroMemory(pStringA,sizeof(pStringA)/sizeof(pStringA[0]));
	wcstombs(pStringA,pStringW,sizeof(pStringW)/sizeof(pStringW[0]));
	printf("wcstombsת����:%s\n",pStringA);
	
	_tsystem(_T("PAUSE"));
	return 0;
}