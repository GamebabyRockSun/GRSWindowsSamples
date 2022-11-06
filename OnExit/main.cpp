#include <tchar.h>
#include <Windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

int fn1(void);
int fn2(void);
int fn3(void);
int fn4 (void);

int WINAPI _tWinMain(HINSTANCE h,HINSTANCE hPrev,LPTSTR psCmdLine,int nShowCmd)
{
	AllocConsole();
	GRS_USEPRINTF();
	_onexit( fn1 );
	_onexit( fn2 );
	_onexit( fn3 );
	_onexit( fn4 );
	GRS_PRINTF( _T("This is executed first.\n") );
	_tsystem(_T("PAUSE"));

	return 0;
}

int fn1()
{
	GRS_USEPRINTF();
	GRS_PRINTF( _T("next.\n") );
	_tsystem(_T("PAUSE"));
	FreeConsole();
	return 0;
}

int fn2()
{
	GRS_USEPRINTF();
	GRS_PRINTF( _T("executed " ));
	return 0;
}

int fn3()
{
	GRS_USEPRINTF();
	GRS_PRINTF( _T("is " ));
	return 0;
}

int fn4()
{
	GRS_USEPRINTF();
	GRS_PRINTF( _T("This " ) );
	return 0;
}
