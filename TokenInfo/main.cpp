#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <Sddl.h>

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

BOOL GetLogonSID (HANDLE hToken, PSID *ppsid) 
{
	GRS_USEPRINTF();
	BOOL bSuccess = FALSE;
	DWORD dwIndex;
	DWORD dwLength = 0;
	PTOKEN_GROUPS ptg = NULL;

	if (NULL == ppsid)
	{
		goto Cleanup;
	}

	if (!GetTokenInformation(hToken,TokenGroups,(LPVOID) ptg,0,&dwLength)) 
	{
		if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
		{
			goto Cleanup;
		}

		ptg = (PTOKEN_GROUPS)GRS_CALLOC(dwLength);

		if (ptg == NULL)
		{
			goto Cleanup;
		}
	}

	if (!GetTokenInformation(hToken,TokenGroups,(LPVOID) ptg,dwLength,&dwLength) ) 
	{
		goto Cleanup;
	}

	GRS_PRINTF(_T("共找到token 0x%08x的%d个Sid：\n"),hToken,ptg->GroupCount);
	LPTSTR pSidString = NULL; 
	for (dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++) 
	{
		ConvertSidToStringSid(ptg->Groups[dwIndex].Sid,&pSidString);
		GRS_PRINTF(_T("%d SID:%s\n"),dwIndex,pSidString);
		LocalFree(pSidString);

		if ((ptg->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID) ==  SE_GROUP_LOGON_ID) 
		{
			dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);
			*ppsid = (PSID)GRS_CALLOC(dwLength);
			if (*ppsid == NULL)
			{
				goto Cleanup;
			}

			if (!CopySid(dwLength, *ppsid, ptg->Groups[dwIndex].Sid)) 
			{
				GRS_FREE((LPVOID)*ppsid);
				goto Cleanup;
			}
			ConvertSidToStringSid(*ppsid,&pSidString);
			GRS_PRINTF(_T("%d SID:%s is Logon ID!\n"),dwIndex,pSidString);
			//break;
		}
	}
	bSuccess = TRUE;
Cleanup: 
	GRS_SAFEFREE(ptg);
	return bSuccess;
}

VOID FreeLogonSID (PSID *ppsid) 
{
	HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
}

int _tmain()
{
	HANDLE hProcTocken = NULL;
	OpenProcessToken(GetCurrentProcess(),TOKEN_ALL_ACCESS,&hProcTocken);
	PSID pSid = NULL;
	GetLogonSID(hProcTocken,&pSid);
	FreeLogonSID(&pSid);
	_tsystem(_T("PAUSE"));
	return 0;
}

