#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

typedef BOOL (WINAPI *LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,PDWORD);

DWORD CountBits(ULONG_PTR bitMask)
{
	DWORD LSHIFT = sizeof(ULONG_PTR)*8 - 1;
	DWORD bitSetCount = 0;
	DWORD bitTest = 1 << LSHIFT;
	DWORD i;

	for( i = 0; i <= LSHIFT; ++i)
	{
		bitSetCount += ((bitMask & bitTest)?1:0);
		bitTest/=2;
	}
	return bitSetCount;
}

int _cdecl _tmain ()
{
	GRS_USEPRINTF();

	LPFN_GLPI Glpi;

	Glpi = (LPFN_GLPI) GetProcAddress(GetModuleHandle(_T("kernel32")),
		"GetLogicalProcessorInformation");
	if (NULL == Glpi) 
	{
		GRS_PRINTF(_T("\nGetLogicalProcessorInformation is not supported.\n"));
		return (1);
	}

	BOOL done = FALSE;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	DWORD returnLength = 0;

	while (!done) 
	{
		DWORD rc = Glpi(buffer, &returnLength);

		if (FALSE == rc) 
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
			{
				GRS_SAFEFREE(buffer);

				buffer=(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)GRS_CALLOC(returnLength);

				if (NULL == buffer) 
				{
					GRS_PRINTF(_T("\nError: Allocation failure\n"));
					return (2);
				}
			} 
			else 
			{
				GRS_PRINTF(_T("\nError %d\n"), GetLastError());
				return (3);
			}
		} 
		else done = TRUE;
	}

	DWORD procCoreCount = 0;
	DWORD procCacheCount = 0;
	DWORD procNumaCount = 0;
	DWORD procPackageCount = 0;
	DWORD byteOffset = 0;

	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = buffer;
	while (byteOffset < returnLength) 
	{
		switch (ptr->Relationship) 
		{
		case RelationNumaNode:
			//  NUMA node number is in ptr->NumaNode.
			//  On a non-NUMA system, ptr->NumaNode is always 1.
			procNumaCount++;
			break;

		case RelationProcessorCore:
			if(ptr->ProcessorCore.Flags)
			{
				//  Hyperthreading or SMT is enabled.
				//  Logical processors are on the same core.
				procCoreCount += 1;
			}
			else
			{
				//  Logical processors are on different cores.
				procCoreCount += CountBits(ptr->ProcessorMask);
			}
			break;

		case RelationCache:
			//  Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
			procCacheCount++;
			break;

		case RelationProcessorPackage:
			//  Logical processors share a physical package.
			procPackageCount++;
			break;

		default:
			GRS_PRINTF(_T("\nError: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n"));
			break;
		}
		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}

	GRS_PRINTF(_T("\nGetLogicalProcessorInformation results:\n"));
	GRS_PRINTF(_T("Number of NUMA nodes: %d\n"),procNumaCount);
	GRS_PRINTF(_T("Number of cores: %d\n"),	procCoreCount);
	GRS_PRINTF(_T("Number of physical packages: %d\n"),procPackageCount);
	GRS_PRINTF(_T("Number of caches: %d\n"),procCacheCount);

	GRS_SAFEFREE (buffer);

	_tsystem(_T("PAUSE"));

	return 0;
}

