#include <tchar.h>
#include <Windows.h>
#include <Lmcons.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define BUFSIZE 1024

BOOL GetOSDisplayString( LPTSTR pszOS)
{
	GRS_USEPRINTF();

	OSVERSIONINFOEX osvi = {sizeof(OSVERSIONINFOEX)};
	SYSTEM_INFO si = {};
	BOOL bOsVersionInfoEx;
	DWORD dwType;

	if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
	{
		return FALSE;
	}

	GetSystemInfo(&si);

	if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && osvi.dwMajorVersion > 4 )
	{
		StringCchCopy(pszOS, BUFSIZE, _T("Microsoft "));

		if ( osvi.dwMajorVersion == 6 )
		{
			if( 0 == osvi.dwMinorVersion )
			{
				if( osvi.wProductType == VER_NT_WORKSTATION )
				{
					StringCchCat(pszOS, BUFSIZE, _T("Windows Vista "));
				}
				else
				{
					StringCchCat(pszOS, BUFSIZE, _T("Windows Server 2008 " ));
				}

			}
			else if( 1 == osvi.dwMinorVersion )
			{
				if( osvi.wProductType == VER_NT_WORKSTATION )
				{
					StringCchCat(pszOS, BUFSIZE, _T("Windows 7 "));
				}
				else
				{
					StringCchCat(pszOS, BUFSIZE, _T("Windows Unknown "));
				}
			}
			else
			{
				StringCchCat(pszOS, BUFSIZE, _T("Windows Unknown "));
			}

			GetProductInfo( 6, 0, 0, 0, &dwType);
			switch( dwType )
			{
			case PRODUCT_ULTIMATE:
				StringCchCat(pszOS, BUFSIZE, _T("Ultimate Edition" ));
				break;
			case PRODUCT_HOME_PREMIUM:
				StringCchCat(pszOS, BUFSIZE, _T("Home Premium Edition" ));
				break;
			case PRODUCT_HOME_BASIC:
				StringCchCat(pszOS, BUFSIZE, _T("Home Basic Edition" ));
				break;
			case PRODUCT_ENTERPRISE:
				StringCchCat(pszOS, BUFSIZE, _T("Enterprise Edition" ));
				break;
			case PRODUCT_BUSINESS:
				StringCchCat(pszOS, BUFSIZE, _T("Business Edition" ));
				break;
			case PRODUCT_STARTER:
				StringCchCat(pszOS, BUFSIZE, _T("Starter Edition" ));
				break;
			case PRODUCT_CLUSTER_SERVER:
				StringCchCat(pszOS, BUFSIZE, _T("Cluster Server Edition" ));
				break;
			case PRODUCT_DATACENTER_SERVER:
				StringCchCat(pszOS, BUFSIZE, _T("Datacenter Edition" ));
				break;
			case PRODUCT_DATACENTER_SERVER_CORE:
				StringCchCat(pszOS, BUFSIZE, _T("Datacenter Edition (core installation)" ));
				break;
			case PRODUCT_ENTERPRISE_SERVER:
				StringCchCat(pszOS, BUFSIZE, _T("Enterprise Edition" ));
				break;
			case PRODUCT_ENTERPRISE_SERVER_CORE:
				StringCchCat(pszOS, BUFSIZE, _T("Enterprise Edition (core installation)" ));
				break;
			case PRODUCT_ENTERPRISE_SERVER_IA64:
				StringCchCat(pszOS, BUFSIZE, _T("Enterprise Edition for Itanium-based Systems" ));
				break;
			case PRODUCT_SMALLBUSINESS_SERVER:
				StringCchCat(pszOS, BUFSIZE, _T("Small Business Server" ));
				break;
			case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
				StringCchCat(pszOS, BUFSIZE, _T("Small Business Server Premium Edition" ));
				break;
			case PRODUCT_STANDARD_SERVER:
				StringCchCat(pszOS, BUFSIZE, _T("Standard Edition" ));
				break;
			case PRODUCT_STANDARD_SERVER_CORE:
				StringCchCat(pszOS, BUFSIZE, _T("Standard Edition (core installation)" ));
				break;
			case PRODUCT_WEB_SERVER:
				StringCchCat(pszOS, BUFSIZE, _T("Web Server Edition" ));
				break;
			}

			if ( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 )
			{
				StringCchCat(pszOS, BUFSIZE, _T( ", 64-bit" ));
			}
			else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL )
			{
				StringCchCat(pszOS, BUFSIZE, _T(", 32-bit"));
			}
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
		{
			if( GetSystemMetrics(SM_SERVERR2) )
			{
				StringCchCat(pszOS, BUFSIZE, _T( "Windows Server 2003 R2, "));
			}
			else if ( osvi.wSuiteMask==VER_SUITE_STORAGE_SERVER )
			{
				StringCchCat(pszOS, BUFSIZE, _T( "Windows Storage Server 2003"));
			}
			else if( osvi.wProductType == VER_NT_WORKSTATION &&
				si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
			{
				StringCchCat(pszOS, BUFSIZE, _T( "Windows XP Professional x64 Edition"));
			}
			else 
			{
				StringCchCat(pszOS, BUFSIZE, _T("Windows Server 2003, "));
			}
			if ( osvi.wProductType != VER_NT_WORKSTATION )
			{
				if ( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 )
				{
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					{
						StringCchCat(pszOS, BUFSIZE, _T( "Datacenter Edition for Itanium-based Systems" ));
					}
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					{
						StringCchCat(pszOS, BUFSIZE, _T( "Enterprise Edition for Itanium-based Systems" ));
					}
				}

				else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
				{
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					{
						StringCchCat(pszOS, BUFSIZE, _T( "Datacenter x64 Edition" ));
					}
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					{					
						StringCchCat(pszOS, BUFSIZE, _T( "Enterprise x64 Edition" ));
					}
					else
					{
						StringCchCat(pszOS, BUFSIZE, _T( "Standard x64 Edition" ));
					}
				}

				else
				{
					if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
					{
						StringCchCat(pszOS, BUFSIZE, _T( "Compute Cluster Edition" ));
					}
					else if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					{
						StringCchCat(pszOS, BUFSIZE, _T( "Datacenter Edition" ));
					}
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					{
						StringCchCat(pszOS, BUFSIZE, _T( "Enterprise Edition" ));
					}
					else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
					{
						StringCchCat(pszOS, BUFSIZE, _T( "Web Edition" ));
					}
					else
					{
						StringCchCat(pszOS, BUFSIZE, _T( "Standard Edition" ));
					}
				}
			}
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
		{
			StringCchCat(pszOS, BUFSIZE, _T("Windows XP "));
			if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
			{
				StringCchCat(pszOS, BUFSIZE, _T( "Home Edition" ));
			}
			else
			{
				StringCchCat(pszOS, BUFSIZE, _T( "Professional" ));
			}
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
		{
			StringCchCat(pszOS, BUFSIZE, _T("Windows 2000 "));

			if ( osvi.wProductType == VER_NT_WORKSTATION )
			{
				StringCchCat(pszOS, BUFSIZE, _T( "Professional" ));
			}
			else 
			{
				if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
				{
					StringCchCat(pszOS, BUFSIZE, _T( "Datacenter Server" ));
				}
				else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
				{
					StringCchCat(pszOS, BUFSIZE, _T( "Advanced Server" ));
				}
				else
				{
					StringCchCat(pszOS, BUFSIZE, _T( "Server" ));
				}
			}
		}

		// Include service pack (if any) and build number.

		if( _tcslen(osvi.szCSDVersion) > 0 )
		{
			StringCchCat(pszOS, BUFSIZE, _T(" ") );
			StringCchCat(pszOS, BUFSIZE, osvi.szCSDVersion);
		}

		TCHAR buf[80];

		StringCchPrintf( buf, 80, _T(" (build %d)"), osvi.dwBuildNumber);
		StringCchCat(pszOS, BUFSIZE, buf);

		return TRUE; 
	}
	else
	{  
		GRS_PRINTF(_T( "This sample does not support this version of Windows.\n") );
		return FALSE;
	}
}


int _tmain()
{
	GRS_USEPRINTF();
	//计算机名
	DWORD dwBufLen = MAX_COMPUTERNAME_LENGTH + 1;
	TCHAR pCName[MAX_COMPUTERNAME_LENGTH + 1];
	GetComputerName(pCName,&dwBufLen);
	GRS_PRINTF(_T("Computer Name:%s"),pCName);
	//用户名
	dwBufLen = UNLEN + 1;
	TCHAR pUName[UNLEN + 1];
	GetUserName(pUName,&dwBufLen);
	GRS_PRINTF(_T("\tUser Name:%s\n"),pUName);
	//系统信息
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);
	GRS_PRINTF(_T("System Info: \n"));  
	GRS_PRINTF(_T("  OEM ID: %u\n"), si.dwOemId);
	GRS_PRINTF(_T("  Number of processors: %u\n"),si.dwNumberOfProcessors); 
	GRS_PRINTF(_T("  Page size: %u\n"), si.dwPageSize); 
	GRS_PRINTF(_T("  Processor type: %u\n"), si.dwProcessorType); 
	GRS_PRINTF(_T("  Minimum application address: %lx\n"),si.lpMinimumApplicationAddress); 
	GRS_PRINTF(_T("  Maximum application address: %lx\n"),si.lpMaximumApplicationAddress); 
	GRS_PRINTF(_T("  Active processor mask: "));
	for( int i = sizeof(si.dwActiveProcessorMask)*8 - 1; i >= 0; --i ) 
	{ 
		GRS_PRINTF(_T("%0d"),(si.dwActiveProcessorMask>>i)&1); 
	} 
	GRS_PRINTF(_T("\n"));

	//鼠标信息
	BOOL fResult;
	int aMouseInfo[3] = {};
	fResult = GetSystemMetrics(SM_MOUSEPRESENT); 
	if ( 0 == fResult ) 
	{
		GRS_PRINTF(_T("No mouse installed.\n")); 
	}
	else 
	{ 
		GRS_PRINTF(_T("Mouse installed.\n"));
		fResult = GetSystemMetrics(SM_SWAPBUTTON); 
		if (fResult == 0) 
		{
			GRS_PRINTF(_T("Buttons not swapped.\n")); 
		}
		else
		{
			GRS_PRINTF(_T("Buttons swapped.\n"));
		}

		fResult = SystemParametersInfo(SPI_GETMOUSE,0,&aMouseInfo,0);

		if( fResult )
		{ 
			GRS_PRINTF(_T("Speed: %d\n"), aMouseInfo[2]); 
			GRS_PRINTF(_T("Threshold (x,y): %d,%d\n"),aMouseInfo[0], aMouseInfo[1]); 
		}
	} 

	TCHAR pStrBuf[BUFSIZE] = {};

	if( GetOSDisplayString(pStrBuf) )
	{
		GRS_PRINTF(_T("%s\n"),pStrBuf);
	}
	TCHAR pDir[MAX_PATH + 1] = {};
	if( GetWindowsDirectory(pDir,MAX_PATH) )
	{
		GRS_PRINTF(_T("Windows Directory:%s\n"),pDir);
	}
	if( GetSystemDirectory(pDir,MAX_PATH) )
	{
		GRS_PRINTF(_T("System Directory:%s\n"),pDir);
	}

	GRS_PRINTF(_T("System Environment Variable:\n"));
	LPTCH lpvEnv = GetEnvironmentStrings();
	if ( lpvEnv == NULL )
	{
		GRS_PRINTF(_T("GetEnvironmentStrings failed (%d)\n"),GetLastError()); 
	}
	else
	{
		LPTSTR lpszVariable = (LPTSTR) lpvEnv;
		size_t szLen = 0;
		size_t szMax = HeapSize(GetProcessHeap(),0,lpvEnv);
		while (*lpszVariable)
		{
			GRS_PRINTF(_T("\n%s\n"), lpszVariable);
			StringCchLength(lpszVariable,szMax,&szLen);
			lpszVariable += szLen + 1;
		}
		FreeEnvironmentStrings(lpvEnv);
	}
	GRS_PRINTF(_T("\n"));
	
	_tsystem(_T("PAUSE"));
	return 0;
}