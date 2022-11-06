#include <tchar.h>
#include <windows.h>
#include <Ntsecapi.h>
#include <strsafe.h>

#define NT_HANDLE_LIST 0x10

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_REALLOC(p,sz)	HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#define GRS_FREE(p)			HeapFree(GetProcessHeap(),0,p)
#define GRS_MSIZE(p)		HeapSize(GetProcessHeap(),0,p)
#define GRS_MVALID(p)		HeapValidate(GetProcessHeap(),0,p)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

typedef enum _OBJECT_INFORMATION_CLASS 
{
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllInformation,
    ObjectDataInformation
} OBJECT_INFORMATION_CLASS, *POBJECT_INFORMATION_CLASS;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
	ULONG	ProcessId;
	UCHAR	ObjectTypeNumber;
	UCHAR	Flags;
	USHORT	Handle;
	PVOID	Object;
	ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef struct _OBJECT_BASIC_INFORMATION 
{
	ULONG  Attributes;
	ACCESS_MASK  GrantedAccess;
	ULONG  HandleCount;
	ULONG  PointerCount;
	ULONG  PagedPoolUsage;
	ULONG  NonPagedPoolUsage;
	ULONG  Reserved[3];
	ULONG  NameInformationLength;
	ULONG  TypeInformationLength;
	ULONG  SecurityDescriptorLength;
	LARGE_INTEGER  CreateTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

typedef struct _OBJECT_NAME_INFORMATION
{
	UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

typedef enum _POOL_TYPE
{
	NonPagedPool,
	PagedPool,
	NonPagedPoolMustSucceed,
	DontUseThisType,
	NonPagedPoolCacheAligned,
	PagedPoolCacheAligned,
	NonPagedPoolCacheAlignedMustS
} POOL_TYPE;

typedef struct _OBJECT_TYPE_INFORMATION 
{
	UNICODE_STRING    TypeName;
	ULONG       TotalNumberOfHandles;
	ULONG       TotalNumberOfObjects;
	WCHAR       Unused1[8];
	ULONG       HighWaterNumberOfHandles;
	ULONG       HighWaterNumberOfObjects;
	WCHAR       Unused2[8];
	ACCESS_MASK     InvalidAttributes;
	GENERIC_MAPPING   GenericMapping;
	ACCESS_MASK     ValidAttributes;
	BOOLEAN       SecurityRequired;
	BOOLEAN       MaintainHandleCount;
	USHORT      MaintainTypeList;
	POOL_TYPE     PoolType;
	ULONG       DefaultPagedPoolCharge;
	ULONG       DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

#define DUPLICATE_SAME_ATTRIBUTES 0x00000004

// 声明NtQuerySystemInformation()函数指针和变量
typedef LONG  (WINAPI *PNTQUERYSYSTEMINFORMATION)(DWORD SystemInformationClass,
	PVOID SystemInformation,ULONG SystemInformationLength,PULONG ReturnLength);
typedef LONG  (WINAPI *PNTQUERYOBJECT)(HANDLE Handle,DWORD ObjectInformationClass,PVOID ObjectInformation,
	ULONG ObjectInformationLength,PULONG ReturnLength);
typedef LONG  (WINAPI *PNTDUPLICATEOBJECT)(HANDLE SourceProcessHandle,HANDLE SourceHandle,HANDLE TargetProcessHandle,
	PHANDLE TargetHandle,ACCESS_MASK DesiredAccess,BOOL bInheritHandle,ULONG Options );

PNTQUERYSYSTEMINFORMATION NtQuerySystemInformation = NULL;
PNTQUERYOBJECT			  NtQueryObject			   = NULL; 
PNTDUPLICATEOBJECT		  NtDuplicateObject		   = NULL; 

//提权函数
BOOL RaisePrivleges( HANDLE hToken, LPTSTR pPriv )
{
	TOKEN_PRIVILEGES tkp = {};
	if ( !LookupPrivilegeValue( NULL, pPriv, &tkp.Privileges[0].Luid ) )
	{
		return FALSE;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes |= SE_PRIVILEGE_ENABLED;
	int iRet = AdjustTokenPrivileges( hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0 );
	if ( iRet == NULL )
	{
		return FALSE;
	}
	else
	{
		iRet = GetLastError();
		switch ( iRet )
		{
		case ERROR_NOT_ALL_ASSIGNED: //未指派所有的特权
			{
				return FALSE;
			}
		case ERROR_SUCCESS: //成功地指派了所有的特权
			{
				return TRUE;
			}
		default: //不知名的错误
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

// 获得系统所有的句柄列表
DWORD* GetHandleList(PSYSTEM_HANDLE_INFORMATION &pHandleList,DWORD& dwHandleCnt)
{
	GRS_USEPRINTF();
	pHandleList = NULL;
	dwHandleCnt = 0;

	DWORD dwBufLen =  0x200000;
	PDWORD pdwHandleList = (PDWORD)GRS_ALLOC(dwBufLen);
	LONG lRet = (*NtQuerySystemInformation)( NT_HANDLE_LIST,pdwHandleList, dwBufLen, &dwBufLen);
	if ( !lRet )
	{
		dwHandleCnt = pdwHandleList[0];
		pHandleList = (PSYSTEM_HANDLE_INFORMATION)( pdwHandleList + 1 );
	}
	else
	{
		GRS_SAFEFREE(pdwHandleList);
	}

	return pdwHandleList;
}

int _tmain()
{
	GRS_USEPRINTF();

	HMODULE hNtdll = LoadLibrary( _T("ntdll.dll") );
	NtQuerySystemInformation = (PNTQUERYSYSTEMINFORMATION)GetProcAddress( hNtdll, "NtQuerySystemInformation");
	NtQueryObject = (PNTQUERYOBJECT)GetProcAddress(hNtdll, "NtQueryObject");
	NtDuplicateObject = (PNTDUPLICATEOBJECT)GetProcAddress(hNtdll, "NtDuplicateObject"); 

	HANDLE hCurrentProc = GetCurrentProcess();
	HANDLE hToken = NULL;
	OpenProcessToken( hCurrentProc, TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken );
	if ( hToken )
	{
		RaisePrivleges( hToken, SE_DEBUG_NAME );
		CloseHandle( hToken );
	}

	PSYSTEM_HANDLE_INFORMATION pHandleList = NULL;
	DWORD dwCnt = 0;
	DWORD*dwBuf = GetHandleList(pHandleList,dwCnt);
	if( NULL == dwBuf )
	{
		return 0;
	}

	LONG  lRet; 
	DWORD dwRead; 
	DWORD dwNameBuffer; 
	OBJECT_BASIC_INFORMATION BasicInfo = {}; 
	OBJECT_NAME_INFORMATION *pNameInfo = NULL; 
	OBJECT_TYPE_INFORMATION *pTypeInfo = NULL; 
	HANDLE hDupObject = NULL; 
	BOOL bDup = FALSE;
	DWORD dwCurProcID = GetCurrentProcessId();
	DWORD dwShowProcID = 0;

	for(DWORD i = 0; i < dwCnt; i++) 
	{ 
		if( dwShowProcID != pHandleList[i].ProcessId)
		{
			dwShowProcID = pHandleList[i].ProcessId;
			_tsystem(_T("PAUSE"));
		}

		hDupObject = NULL;
		if( pHandleList[i].ProcessId == dwCurProcID ) 
		{ 
			hDupObject = (HANDLE)pHandleList[i].Handle; 
		}
		else
		{
			HANDLE hProcess = GetCurrentProcess();//OpenProcess(PROCESS_ALL_ACCESS,FALSE,pHandleList[i].ProcessId);
			lRet = NtDuplicateObject(hProcess,(HANDLE)pHandleList[i].Handle,GetCurrentProcess(),&hDupObject,0,0,DUPLICATE_SAME_ATTRIBUTES); 
			if( lRet != 0 ) 
			{ 
				GRS_PRINTF(_T("无法复制句柄：0x%08X\t错误码：0x%08x\n"),pHandleList[i].Handle,lRet);
				hDupObject = (HANDLE)pHandleList[i].Handle; 
			} 
		}

		//基本信息 
		lRet = NtQueryObject(hDupObject,ObjectBasicInformation,&BasicInfo,sizeof(OBJECT_BASIC_INFORMATION),&dwRead); 
		if(lRet != 0) 
		{ 
			GRS_PRINTF(_T("无法得到句柄 0x%08X 的基本信息，错误码:0x%08x\n"),pHandleList[i].Handle,lRet); 		
			continue; 
		} 

		//类型信息 
		pTypeInfo = (OBJECT_TYPE_INFORMATION *)GRS_CALLOC(BasicInfo.TypeInformationLength + 2); 
		lRet = NtQueryObject(hDupObject,ObjectTypeInformation,pTypeInfo,BasicInfo.TypeInformationLength + 2,&dwRead); 
		if(lRet != 0) 
		{ 
			GRS_PRINTF(_T("无法得到句柄 0x%08X 的类型信息，错误码:0x%08x\n"),pHandleList[i].Handle,lRet); 
			continue; 
		} 

		//名字信息 
		dwNameBuffer = (BasicInfo.NameInformationLength == 0) ? 
			(MAX_PATH * sizeof (WCHAR)) : BasicInfo.NameInformationLength; 
		pNameInfo = (OBJECT_NAME_INFORMATION *)GRS_CALLOC(dwNameBuffer * sizeof(TCHAR)); 
		lRet = NtQueryObject(hDupObject,ObjectNameInformation,pNameInfo,dwNameBuffer,&dwRead); 
		if(lRet != 0) 
		{ 
			GRS_PRINTF(_T("无法得到句柄 0x%08X 的名称信息，错误码:0x%08x\n"),pHandleList[i].Handle,lRet); 
			continue; 
		} 

		GRS_PRINTF(_T("进程ID:%-8u\t句柄 0x%08X(Dup:0x%08X)\t类型 %-20s\t名称 %s\n")
			,pHandleList[i].ProcessId,pHandleList[i].Handle,hDupObject,pTypeInfo->TypeName.Buffer,pNameInfo->Name.Buffer);

		GRS_SAFEFREE(pTypeInfo); 
		GRS_SAFEFREE(pNameInfo); 
	} 
	
	_tsystem(_T("PAUSE"));
	return 0;
}
