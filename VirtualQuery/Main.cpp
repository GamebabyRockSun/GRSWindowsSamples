#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <atlstr.h>

CString FormatMemInfo(MEMORY_BASIC_INFORMATION &mi)
{
	CString strAllocProtect;
	if(mi.AllocationProtect & PAGE_NOACCESS)                // 0x0001
	{
		strAllocProtect = _T("N  ");
	}
	if(mi.AllocationProtect & PAGE_READONLY)                // 0x0002
	{
		strAllocProtect = _T("R  ");
	}
	else if(mi.AllocationProtect & PAGE_READWRITE)          // 0x0004
	{
		strAllocProtect = _T("RW ");
	}
	else if(mi.AllocationProtect & PAGE_WRITECOPY)          // 0x0008
	{
		strAllocProtect = _T("WC ");
	}
	else if(mi.AllocationProtect & PAGE_EXECUTE)            // 0x0010
	{
		strAllocProtect = _T("E  ");
	}
	else if(mi.AllocationProtect & PAGE_EXECUTE_READ)       // 0x0020
	{
		strAllocProtect = _T("ER ");
	}
	else if(mi.AllocationProtect & PAGE_EXECUTE_READWRITE)  // 0x0040
	{
		strAllocProtect = _T("ERW");
	}
	else if(mi.AllocationProtect & PAGE_EXECUTE_WRITECOPY)  // 0x0080
	{
		strAllocProtect = _T("EWC");
	}
	if(mi.AllocationProtect & PAGE_GUARD)                   // 0x0100
	{
		strAllocProtect += _T("+Guard");
	}
	if(mi.AllocationProtect & PAGE_NOCACHE)                 // 0x0200
	{
		strAllocProtect += _T("+NoCache");
	}
	CString strState;
	if(mi.State == MEM_COMMIT)
	{
		strState = _T("Commit ");
	}
	else if(mi.State == MEM_FREE)
	{
		strState = _T("Free   ");
	}
	else if(mi.State == MEM_RESERVE)
	{
		strState =_T( "Reserve");
	}
	else
	{
		strState =_T( "Damned ");
	}
	CString strProtect;
	if(mi.Protect & PAGE_NOACCESS)
	{
		strProtect = _T("N  ");
	}
	else if(mi.Protect & PAGE_READONLY)
	{
		strProtect = _T("R  ");
	}
	else if(mi.Protect & PAGE_READWRITE)
	{
		strProtect = _T("RW ");
	}
	else if(mi.Protect & PAGE_WRITECOPY)
	{
		strProtect = _T("WC ");
	}
	else if(mi.Protect & PAGE_EXECUTE)
	{
		strProtect = _T("E  ");
	}
	else if(mi.Protect & PAGE_EXECUTE_READ)
	{
		strProtect = _T("ER ");
	}
	else if(mi.Protect & PAGE_EXECUTE_READWRITE)
	{
		strProtect = _T("ERW");
	}
	else if(mi.Protect & PAGE_EXECUTE_WRITECOPY)
	{
		strProtect = _T("EWC");
	}
	else if(mi.Protect & PAGE_GUARD) 
	{
		strProtect += _T("+Guard");
	}
	else if( mi.Protect & PAGE_NOCACHE )
	{
		strProtect += _T("+NoCache");
	}
	
	CString strType;
	if(mi.Type == MEM_IMAGE)
	{
		strType =_T( "Image  ");
	}
	else if(mi.Type == MEM_MAPPED)
	{
		strType = _T("Mapped ");
	}
	else if(mi.Type == MEM_PRIVATE)
	{
		strType = _T("Private");
	}
	else
	{
		strType = _T("-      ");
	}

	CString strSize;
	strSize.Format(_T("%8u%s")
		, ( mi.RegionSize > 1099511627776u ? mi.RegionSize / 1099511627776u 
			: ( mi.RegionSize > 1073741824u ? mi.RegionSize / 1073741824u 
			: ( mi.RegionSize > 1048576u ? mi.RegionSize / 1048576u
			: ( mi.RegionSize > 1024u ? mi.RegionSize / 1024u : mi.RegionSize) 
			  ) 
			) 
		   )
		, ( mi.RegionSize > 1099511627776u ? _T("TB")
			: ( mi.RegionSize > 1073741824u ? _T("GB")
			: ( mi.RegionSize > 1048576u ? _T("MB")
				: ( mi.RegionSize > 1024u ? _T("KB") : _T("B ") 
					) 
				) 
			) 
		)
	);

	CString strRet;
	strRet.Format(_T("0x%p  0x%p  0x%p  %12s  %12s  %8s  %10s  %8s")
		, mi.BaseAddress
		, mi.AllocationBase
		, (BYTE*)mi.AllocationBase + mi.RegionSize
		, strSize
		, strAllocProtect
		, strState
		, strProtect
		, strType);

	return strRet;
}

int _tmain()
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	
	VOID* pLowerBound = (VOID*)info.lpMinimumApplicationAddress; 
	VOID* pUpperBound = (VOID*)info.lpMaximumApplicationAddress; 

	MEMORY_BASIC_INFORMATION mi;
	VOID* pPtr = pLowerBound;
	VOID* pOldPtr = pPtr;

	_putts(_T("  BaseAddress         AllocBase           EndAddress     SIZE            AllocProtect   State     CurProtect  TYPE"));
	while( pPtr <= pUpperBound)
	{
		if( VirtualQuery((void*)pPtr, &mi, sizeof(mi)) == 0 )
		{
			break;
		}

		_putts(FormatMemInfo(mi)); 
		
		pOldPtr = pPtr;
		pPtr = (BYTE*)mi.BaseAddress + mi.RegionSize;

		if( pPtr <= pOldPtr )   
		{
			break;
		}
	}

	_tsystem(_T("PAUSE"));
	return 0;
}


