#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <locale.h>
#include <mbstring.h>

int _tmain()
{
	_tsetlocale(LC_ALL,_T("chs"));

	TCHAR pszTest[] = _T("123Abcºº×Ö€ørþ`þcþ{þ‹©L©J©I©N©P©R©T©S");
	_tprintf(_T("Len=%d; %s\n"),_tcslen(pszTest),pszTest);
#ifdef _UNICODE
	wprintf(L"Len=%d; wprintf->:%s\n",wcslen(pszTest),pszTest);
#else
	printf("Len=%d; printf->:%s\n",strlen(pszTest),pszTest);
#endif
	char pszAString[] = "123Abcºº×Ö€ørþ`þcþ{þ‹©L©J©I©N©P©R©T©S";
	printf("MBCS(ANSI)->:Len=%d; %s\n",strlen(pszAString),pszAString);

	wchar_t pszWString[] = L"123Abcºº×Ö€ørþ`þcþ{þ‹©L©J©I©N©P©R©T©S";
	wprintf(L"UNICODE->:Len=%d; %s\n",wcslen(pszWString),pszWString);

	printf("MBCS Len = %d And Unicode Len = %d : %s\n"
		,_mbslen((const unsigned char*)pszAString),wcslen(pszWString),pszAString);

	_tsystem(_T("PAUSE"));
	return 0;
}