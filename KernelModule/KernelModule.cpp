#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);
#define GRS_PRINTFA(...) \
	StringCchPrintfA(pOutBufA,1024,__VA_ARGS__);\
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE),pOutBufA,lstrlenA(pOutBufA),NULL,NULL);

#define GRS_SUCCESS(status)      ((NTSTATUS)(status)>=0)

//几乎所有ZwQuerySystemInformation可以枚举的系统信息的枚举值
//列举在这里供大家参考
typedef enum _SYSTEM_INFORMATION_CLASS     //    Q S
{
	SystemBasicInformation,                // 00 Y N
	SystemProcessorInformation,            // 01 Y N
	SystemPerformanceInformation,          // 02 Y N
	SystemTimeOfDayInformation,            // 03 Y N
	SystemNotImplemented1,                 // 04 Y N
	SystemProcessesAndThreadsInformation,  // 05 Y N
	SystemCallCounts,                      // 06 Y N
	SystemConfigurationInformation,        // 07 Y N
	SystemProcessorTimes,                  // 08 Y N
	SystemGlobalFlag,                      // 09 Y Y
	SystemNotImplemented2,                 // 10 Y N
	SystemModuleInformation,               // 11 Y N
	SystemLockInformation,                 // 12 Y N
	SystemNotImplemented3,                 // 13 Y N
	SystemNotImplemented4,                 // 14 Y N
	SystemNotImplemented5,                 // 15 Y N
	SystemHandleInformation,               // 16 Y N
	SystemObjectInformation,               // 17 Y N
	SystemPagefileInformation,             // 18 Y N
	SystemInstructionEmulationCounts,      // 19 Y N
	SystemInvalidInfoClass1,               // 20
	SystemCacheInformation,                // 21 Y Y
	SystemPoolTagInformation,              // 22 Y N
	SystemProcessorStatistics,             // 23 Y N
	SystemDpcInformation,                  // 24 Y Y
	SystemNotImplemented6,                 // 25 Y N
	SystemLoadImage,                       // 26 N Y
	SystemUnloadImage,                     // 27 N Y
	SystemTimeAdjustment,                  // 28 Y Y
	SystemNotImplemented7,                 // 29 Y N
	SystemNotImplemented8,                 // 30 Y N
	SystemNotImplemented9,                 // 31 Y N
	SystemCrashDumpInformation,            // 32 Y N
	SystemExceptionInformation,            // 33 Y N
	SystemCrashDumpStateInformation,       // 34 Y Y/N
	SystemKernelDebuggerInformation,       // 35 Y N
	SystemContextSwitchInformation,        // 36 Y N
	SystemRegistryQuotaInformation,        // 37 Y Y
	SystemLoadAndCallImage,                // 38 N Y
	SystemPrioritySeparation,              // 39 N Y
	SystemNotImplemented10,                // 40 Y N
	SystemNotImplemented11,                // 41 Y N
	SystemInvalidInfoClass2,               // 42
	SystemInvalidInfoClass3,               // 43
	SystemTimeZoneInformation,             // 44 Y N
	SystemLookasideInformation,            // 45 Y N
	SystemSetTimeSlipEvent,                // 46 N Y
	SystemCreateSession,                   // 47 N Y
	SystemDeleteSession,                   // 48 N Y
	SystemInvalidInfoClass4,               // 49
	SystemRangeStartInformation,           // 50 Y N
	SystemVerifierInformation,             // 51 Y Y
	SystemAddVerifier,                     // 52 N Y
	SystemSessionProcessesInformation      // 53 Y N
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_MODULE_INFORMATION  // Information Class 11
{
	ULONG  Reserved[2];
	PVOID  pBase;
	ULONG  Size;
	ULONG  Flags;
	USHORT Index;
	USHORT Unknown;
	USHORT LoadCount;
	USHORT ModuleNameOffset;
	CHAR   ImageName[256];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

typedef NTSTATUS ( __stdcall *ZWQUERYSYSTEMINFORMATION ) (
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass
	, IN OUT PVOID SystemInformation
	, IN ULONG SystemInformationLength
	, OUT PULONG ReturnLength OPTIONAL );
typedef ULONG    ( __stdcall *RTLNTSTATUSTODOSERROR    ) ( IN NTSTATUS Status );

ZWQUERYSYSTEMINFORMATION ZwQuerySystemInformation = NULL;
RTLNTSTATUSTODOSERROR    RtlNtStatusToDosError    = NULL;

VOID ShowLastError (LPCTSTR pszMsg, DWORD dwMessageId )
{
	GRS_USEPRINTF();
	LPTSTR pErrMsg = NULL;

	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		dwMessageId,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
		( LPTSTR )&pErrMsg, 0, NULL );
	
	GRS_PRINTF(_T("%s: %s"), pszMsg, pErrMsg );

	LocalFree( pErrMsg );
	return;
}  

VOID ShowKernelError (LPCTSTR pszMsg, NTSTATUS status )
{
	GRS_USEPRINTF();
	LPTSTR pErrMsg = NULL;

	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		RtlNtStatusToDosError( status ),//内核错误码转换成用户方式的错误码
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
		( LPTSTR )&pErrMsg, 0, NULL );

	GRS_PRINTF(_T("%s: %s"), pszMsg, pErrMsg );
	
	LocalFree( pErrMsg );
	return;
} 

BOOL LoadFunction ( VOID )
{
	BOOL	bRet = FALSE;
	TCHAR   pszNtDllName[] = _T("ntdll.dll");
	HMODULE hNtDll   = NULL;

	if ( ( hNtDll = GetModuleHandle( pszNtDllName ) ) == NULL )
	{
		ShowLastError( _T("GetModuleHandle() failed"), GetLastError() );
		return( FALSE );
	}

	if ( !( ZwQuerySystemInformation = ( ZWQUERYSYSTEMINFORMATION )GetProcAddress( hNtDll,
		"ZwQuerySystemInformation" ) ) )
	{
		goto LoadFunction_Clear;
	}

	if ( !( RtlNtStatusToDosError = ( RTLNTSTATUSTODOSERROR )GetProcAddress( hNtDll,
		"RtlNtStatusToDosError" ) ) )
	{
		goto LoadFunction_Clear;
	}
	
	bRet = TRUE;

LoadFunction_Clear:

	if ( FALSE == bRet )
	{
		ShowLastError( _T("GetProcAddress() failed"), GetLastError() );
	}
	hNtDll = NULL;
	return( bRet );
}  /* end of LoadFunction */


VOID EnumKernelModules ()
{
	GRS_USEPRINTF();
	NTSTATUS                    status;
	PSYSTEM_MODULE_INFORMATION  pModuleInfo = NULL;
	PVOID                       pBase   = NULL;
	ULONG                       n      = 0;
	ULONG                       i      = 0;
	VOID                       *pInfoBuf    = NULL;

	ZwQuerySystemInformation( SystemModuleInformation, &n, 0, &n );
	if ( NULL == ( pInfoBuf = GRS_CALLOC(n) ) )
	{
		GRS_PRINTF(_T("GRS_CALLOC() failed\n"));
		goto EnumKernelModuls_Clear;
	}

	status = ZwQuerySystemInformation( SystemModuleInformation, pInfoBuf, n, NULL );
	if ( !GRS_SUCCESS( status ) )
	{
		ShowKernelError( _T("ZwQuerySystemInformation() failed"), status );
		goto EnumKernelModuls_Clear;
	}

	pModuleInfo = ( PSYSTEM_MODULE_INFORMATION )( ( PULONG )pInfoBuf + 1 );
	n      = *( (PULONG)pInfoBuf );
	for ( i = 0; i < n; i++ )
	{
		GRS_PRINTF(_T("LoadCount:%-4d\t"),pModuleInfo[i].LoadCount);
		GRS_PRINTF(_T("INDEX:%-4d\t"),pModuleInfo[i].Index);
		GRS_PRINTF(_T("BaseAddr:0x%08x\t"),pModuleInfo[i].pBase);
		GRS_PRINTFA("%s\n",pModuleInfo[i].ImageName);
	}

EnumKernelModuls_Clear:
	GRS_SAFEFREE(pInfoBuf);
}

int _tmain()
{
	LoadFunction();
	EnumKernelModules();
	_tsystem(_T("PAUSE"));
	return 0;
}