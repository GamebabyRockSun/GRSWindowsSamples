#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#pragma hdrstop

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_REALLOC(p,sz)	HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#define GRS_FREE(p)			HeapFree(GetProcessHeap(),0,p)
#define GRS_MSIZE(p)		HeapSize(GetProcessHeap(),0,p)
#define GRS_MVALID(p)		HeapValidate(GetProcessHeap(),0,p)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

int _tmain()
{
	HANDLE hToken;
	UCHAR InfoBuffer[1000] = {};
	TCHAR privilegeName[500] = {};
	TCHAR displayName[500] = {};
	PTOKEN_PRIVILEGES ptgPrivileges = (PTOKEN_PRIVILEGES) InfoBuffer;
	DWORD infoBufferSize = sizeof(InfoBuffer)/sizeof(InfoBuffer[0]);
	DWORD privilegeNameSize;
	DWORD displayNameSize;
	DWORD langId = GetUserDefaultLangID();

	GRS_USEPRINTF();
	if ( ! OpenProcessToken( GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
	{
		GRS_PRINTF( _T("OpenProcessToken Error 0x%08X\n"),GetLastError());
		return 1;
	}

	if(!GetTokenInformation( hToken, TokenPrivileges, InfoBuffer,sizeof(InfoBuffer), &infoBufferSize))
	{
		GRS_PRINTF( _T("GetTokenInformation Error 0x%08X\n"),GetLastError());
		CloseHandle(hToken);
		return 1;
	}

	GRS_PRINTF(_T("当前帐号特权: \n\n") );
	for( UINT i = 0; i < ptgPrivileges->PrivilegeCount; i ++ )
	{
		privilegeNameSize = sizeof(privilegeName) / sizeof(privilegeName[0]);
		displayNameSize = sizeof(displayName) / sizeof(displayName[0]);
		
		LookupPrivilegeName( NULL, &ptgPrivileges->Privileges[i].Luid,
			privilegeName, &privilegeNameSize );
		LookupPrivilegeDisplayName( NULL, privilegeName,
			displayName, &displayNameSize, &langId );
		GRS_PRINTF( _T("%-30s\t(%-20s)\t"), displayName, privilegeName );

		if(ptgPrivileges->Privileges[i].Attributes & (SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT))
		{
			GRS_PRINTF( _T("优先权开放"));
		}
		else
		{
			GRS_PRINTF( _T("优先权不可用"));
		}
		GRS_PRINTF( _T("\n"));
	}
	_tsystem(_T("PAUSE"));
	return 0;
}

