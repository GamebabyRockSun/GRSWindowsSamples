#include <tchar.h>
#include <windows.h>
#include <locale.h>
#include <malloc.h>

int _tmain()
{
	_tsetlocale(LC_ALL,_T("chs"));

	//大小写转换
	TCHAR chLower[] = _T("abc αβγδεζνξοπρσηθικλμτυφχψω");
	TCHAR *chUpper = NULL;
	_tprintf(_T("Lower Char = %s\n"),chLower);
	chUpper = CharUpper(chLower);
	_tprintf(_T("Upper Char = %s\n"),chUpper);
	_tprintf(_T("Upper Char Point = 0x%08X,Lower Char Point = 0x%08X\n"),chUpper,chLower);
	CharLower(chUpper);
	_tprintf(_T("Convert Lower Char = %s\n"),chLower);

	TCHAR pString[] = _T("一二三赵钱孙李ABCabc123１２３４");
	int iLen = lstrlen(pString);
	TCHAR* pNext = pString;
	TCHAR* pPrev = pString + sizeof(pString)/sizeof(pString[0]) - 1;	
	for(int i = 0;i < iLen; i ++)
	{
		pPrev = CharPrev(pString,pPrev);
		_tprintf(_T("Next Char='%c'\tPrev Char='%c'"),*pNext,*pPrev);
		
		if(IsCharAlpha(*pNext))
		{
			_tprintf(_T("\tNext Char is Alpha"));
		}
		else if(IsCharAlphaNumeric(*pNext))
		{
			_tprintf(_T("\tNext Char is Alpha Numeric"));
		}
		else
		{
			_tprintf(_T("\tNext Char is Unknown Type"),*pNext);
		}

		if(IsCharLower(*pNext))
		{
			_tprintf(_T("\tNext Char is Lower\n"));
		}
		else if(IsCharUpper(*pNext))
		{
			_tprintf(_T("\tNext Char is Upper\n"));
		}
		else
		{
			_tprintf(_T("\tNext Char is no Lower or Upper\n"),*pNext);
		}

		pNext = CharNext(pNext);
	}

	
	
	_tsystem(_T("PAUSE"));

	return 0;
}