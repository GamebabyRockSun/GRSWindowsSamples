#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

BOOL CALLBACK EnumCodePagesProc(LPTSTR lpCodePageString)
{
	const int iBufLen = 1024;
	TCHAR pBuff[iBufLen];
	size_t szLen = 0;
	CPINFOEX ci = {};

	GetCPInfoEx((UINT)_ttoi(lpCodePageString),0,&ci);
	StringCchPrintf(pBuff,iBufLen,_T("CP:%5s\tCP Name:%s\n\
Max Char Size:%uBytes Default Char:0x%02X%02X Lead Byte:0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X Unicode Default Char:%c CP:%u\n\
-------------------------------------------------------------------------------------------------------------------------------------------\n")
		,lpCodePageString
		,ci.CodePageName
		,ci.MaxCharSize
		,ci.DefaultChar[0],ci.DefaultChar[1]
		,ci.LeadByte[0],ci.LeadByte[1],ci.LeadByte[2],ci.LeadByte[3],ci.LeadByte[4],ci.LeadByte[5]
		,ci.LeadByte[6],ci.LeadByte[7],ci.LeadByte[8],ci.LeadByte[9],ci.LeadByte[10],ci.LeadByte[11]
		,ci.UnicodeDefaultChar
		,ci.CodePage);
	StringCchLength(pBuff,iBufLen,&szLen);
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuff,szLen,NULL,NULL);
	return TRUE;
}

int _tmain()
{
	COORD cd = {156,1024};
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),cd);
	TCHAR psInstalled[] = _T("Installed Code Pages:\n\
-------------------------------------------------------------------------------------------------------------------------------------------\n"); 
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),psInstalled
		,sizeof(psInstalled)/sizeof(psInstalled[0]) - 1,NULL,NULL);
	EnumSystemCodePages(EnumCodePagesProc,CP_INSTALLED);

	_tsystem(_T("PAUSE"));

	TCHAR psAll[] = _T("All Supported Code Pages:\n\
-------------------------------------------------------------------------------------------------------------------------------------------\n"); 
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),psAll
		,sizeof(psAll)/sizeof(psAll[0]) - 1,NULL,NULL);
	EnumSystemCodePages(EnumCodePagesProc,CP_SUPPORTED);

	_tsystem(_T("PAUSE"));
	return 0;
}