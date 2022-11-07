#define _WIN32_WINNT 0x0502
#include <tchar.h>
#include <windows.h>
#include <stdio.h>

#define MEMORY_REQUESTED 1024 * 1024 * 1024 //1GB

BOOL LoggedSetLockPagesPrivilege ( HANDLE hProcess, BOOL bEnable);

void _cdecl main()
{
	BOOL bResult;                   // generic Boolean value
	ULONG_PTR NumberOfPages;        // number of pages to request
	ULONG_PTR NumberOfPagesInitial; // initial number of pages requested
	ULONG_PTR *aPFNs1;               // page info; holds opaque data
	ULONG_PTR *aPFNs2;               // page info; holds opaque data
	PVOID lpMemReserved;            // AWE window
	SYSTEM_INFO sSysInfo;           // useful system information
	size_t PFNArraySize;               // memory to request for PFN array
	TCHAR* pszData;
	TCHAR  pszReadData[100];

	MEMORYSTATUSEX ms = {sizeof(MEMORYSTATUSEX)};
	GlobalMemoryStatusEx(&ms);
	
//	LoggedSetLockPagesPrivilege(::GetCurrentProcess(),TRUE);

	//1 "����"
	lpMemReserved = VirtualAlloc( NULL,MEMORY_REQUESTED, MEM_RESERVE | MEM_PHYSICAL, PAGE_READWRITE );
	//������Ҫ������ҳ���С �Լ�����ҳ����Ҫ��ҳ�������С
	GetSystemInfo(&sSysInfo);  // fill the system information structure
	NumberOfPages = (MEMORY_REQUESTED + sSysInfo.dwPageSize - 1)/sSysInfo.dwPageSize;
	PFNArraySize = NumberOfPages * sizeof (ULONG_PTR);
	//2 ׼������ҳ���ҳ����������
	aPFNs1 = (ULONG_PTR *) HeapAlloc(GetProcessHeap(), 0, PFNArraySize);
	aPFNs2 = (ULONG_PTR *) HeapAlloc(GetProcessHeap(), 0, PFNArraySize);
	NumberOfPagesInitial = NumberOfPages;
	//3 ��������ҳ��
	bResult = AllocateUserPhysicalPages( GetCurrentProcess(),&NumberOfPages,aPFNs1 );
	bResult = AllocateUserPhysicalPages( GetCurrentProcess(),&NumberOfPages,aPFNs2 );
	//4 ӳ���һ��1GB�������Ŀռ���
	bResult = MapUserPhysicalPages( lpMemReserved,NumberOfPages,aPFNs1 );
	pszData = (TCHAR*)lpMemReserved;
	_tcscpy(pszData,_T("���ǵ�һ�������ڴ�"));
	//5 ӳ��ڶ���1GB�������Ŀռ���
	bResult = MapUserPhysicalPages( lpMemReserved,NumberOfPages,aPFNs2 );
	_tcscpy(pszData,_T("���ǵڶ��������ڴ�"));
	//6 ��ӳ��ص�һ���ڴ�,����ȡ��ʼ����
	bResult = MapUserPhysicalPages( lpMemReserved,NumberOfPages,aPFNs1 );
	_tcscpy(pszReadData,pszData);
	//7 ȡ��ӳ��
	bResult = MapUserPhysicalPages( lpMemReserved,NumberOfPages,NULL );
	//8 �ͷ�����ҳ��
	bResult = FreeUserPhysicalPages( GetCurrentProcess(),&NumberOfPages,aPFNs1 );
	bResult = FreeUserPhysicalPages( GetCurrentProcess(),&NumberOfPages,aPFNs2 );
	//9 �ͷű�����"����"�ռ�
	bResult = VirtualFree( lpMemReserved,0,	MEM_RELEASE );
	//10 �ͷ�ҳ������
	bResult = HeapFree(GetProcessHeap(), 0, aPFNs1);
	bResult = HeapFree(GetProcessHeap(), 0, aPFNs2);

	_tsystem(_T("PAUSE"));

}

/*****************************************************************
LoggedSetLockPagesPrivilege: a function to obtain or
release the privilege of locking physical pages.

Inputs:

HANDLE hProcess: Handle for the process for which the
privilege is needed

BOOL bEnable: Enable (TRUE) or disable?

Return value: TRUE indicates success, FALSE failure.

*****************************************************************/
BOOL
LoggedSetLockPagesPrivilege ( HANDLE hProcess,
							 BOOL bEnable)
{
	struct {
		DWORD Count;
		LUID_AND_ATTRIBUTES Privilege [1];
	} Info;

	HANDLE Token;
	BOOL Result;

	// Open the token.

	Result = OpenProcessToken ( hProcess,
		TOKEN_ADJUST_PRIVILEGES,
		& Token);

	if( Result != TRUE ) 
	{
		printf( "Cannot open process token.\n" );
		return FALSE;
	}

	// Enable or disable?

	Info.Count = 1;
	if( bEnable ) 
	{
		Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
	} 
	else 
	{
		Info.Privilege[0].Attributes = 0;
	}

	// Get the LUID.

	Result = LookupPrivilegeValue ( NULL,
		SE_LOCK_MEMORY_NAME,
		&(Info.Privilege[0].Luid));

	if( Result != TRUE ) 
	{
		printf( "Cannot get privilege for %s.\n", SE_LOCK_MEMORY_NAME );
		return FALSE;
	}

	// Adjust the privilege.

	Result = AdjustTokenPrivileges ( Token, FALSE,
		(PTOKEN_PRIVILEGES) &Info,
		0, NULL, NULL);

	// Check the result.

	if( Result != TRUE ) 
	{
		printf ("Cannot adjust token privileges (%u)\n", GetLastError() );
		return FALSE;
	} 
	else 
	{
		if( GetLastError() != ERROR_SUCCESS ) 
		{
			printf ("Cannot enable the SE_LOCK_MEMORY_NAME privilege; ");
			printf ("please check the local policy.\n");
			return FALSE;
		}
	}

	CloseHandle( Token );

	return TRUE;
}
