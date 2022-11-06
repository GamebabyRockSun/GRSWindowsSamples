#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

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

BOOL SetPrivilege(HANDLE hToken,LPCTSTR lpszPrivilege,BOOL bEnablePrivilege) 
{
	GRS_USEPRINTF();
	TOKEN_PRIVILEGES tp;
	LUID luid;
	//查找到特权的LUID值
	if ( !LookupPrivilegeValue(NULL,lpszPrivilege,&luid ) )
	{
		GRS_PRINTF(_T("LookupPrivilegeValue error: %u\n"), GetLastError() ); 
		return FALSE; 
	}
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
	{
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	}
	else
	{
		tp.Privileges[0].Attributes = 0;
	}
	//为访问标记（访问字串）分配特权
	if ( !AdjustTokenPrivileges(hToken,FALSE,&tp,sizeof(TOKEN_PRIVILEGES),(PTOKEN_PRIVILEGES) NULL,(PDWORD) NULL) )
	{ 
		GRS_PRINTF(_T("AdjustTokenPrivileges error: %u\n"), GetLastError() ); 
		return FALSE; 
	} 
	if ( GetLastError() == ERROR_NOT_ALL_ASSIGNED )
	{
		GRS_PRINTF(_T("分配指定的特权(%s)失败. \n"),lpszPrivilege);
		return FALSE;
	} 
	return TRUE;
}

LPTSTR g_pPrivileges[]=
{ 
	SE_ASSIGNPRIMARYTOKEN_NAME,
	SE_BACKUP_NAME,
	SE_DEBUG_NAME, 
	SE_INCREASE_QUOTA_NAME, 
	SE_TCB_NAME
};

int _tmain()
{
	HANDLE hProcTocken = NULL;
	OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&hProcTocken);
	int iCnt = sizeof(g_pPrivileges)/sizeof(g_pPrivileges[0]);
	GRS_USEPRINTF();
	for(int i = 0; i < iCnt ; i ++)
	{
		if(SetPrivilege(hProcTocken,g_pPrivileges[i],TRUE) )
		{
			GRS_PRINTF(_T("特权 %s 打开成功\n"),g_pPrivileges[i]);
		}
	}

	_tsystem(_T("PAUSE"));

	return 0;
}