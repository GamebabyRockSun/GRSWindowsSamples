#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <locale.h>

int _tmain()
{
	_tsetlocale(LC_ALL,_T("chs"));
	
	//取得UTF-8的编码信息
	CPINFOEX ci = {};
	GetCPInfoEx(CP_UTF8,0,&ci);
	_tprintf(_T("Code Page Name: UTF-8\tMax Char Size:%uBytes\tDefault Char:0x%X%X\tLead Byte:0x%X%X%X%X%X%X%X%X%X%X%X%X\n")
		,ci.MaxCharSize
		,ci.DefaultChar[0],ci.DefaultChar[1]
		,ci.LeadByte[0],ci.LeadByte[1],ci.LeadByte[2],ci.LeadByte[3],ci.LeadByte[4],ci.LeadByte[5]
		,ci.LeadByte[6],ci.LeadByte[7],ci.LeadByte[8],ci.LeadByte[9],ci.LeadByte[10],ci.LeadByte[11]);
	_tprintf(_T("Unicode Default Char:%c\tCode Page:%u\tCode Page Name:%s\n")
		,ci.UnicodeDefaultChar
		,ci.CodePage
		,ci.CodePageName);
	//UTF-8编码的串"1234ABCxyz赵钱孙李"
	UCHAR pUTF8[] = "\x31\x32\x33\x34\x41\x42\x43\x78\x79\x7A\xE8\xB5\xB5\xE9\x92\xB1\xE5\xAD\x99\xE6\x9D\x8E";
	//UTF-8转UNICODE
	INT iLen = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pUTF8, -1, NULL,0);
	WCHAR * pwUtf8 = (WCHAR*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(iLen+1)*sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pUTF8, -1, pwUtf8, iLen);
	wprintf(L"UTF8->Unicode = %s\n",pwUtf8);

	//UNICODE转ANSI，经过两次转换 UTF-8 已经变成了 GBK 编码
	iLen = WideCharToMultiByte(CP_ACP, 0, pwUtf8, -1, NULL, 0, NULL, NULL); 
	CHAR* psGBK = (CHAR*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,iLen * sizeof(CHAR));
	WideCharToMultiByte (CP_ACP, 0, pwUtf8, -1, psGBK, iLen, NULL,NULL);
	printf("Unicode->GBK = %s\n",psGBK);

	HeapFree(GetProcessHeap(),0,pwUtf8);
	HeapFree(GetProcessHeap(),0,psGBK);

	_tsystem(_T("PAUSE"));
	return 0;
}
