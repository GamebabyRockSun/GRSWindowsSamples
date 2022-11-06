#include <tchar.h>
#include <Windows.h>

int _tmain()
{
	HANDLE hLFHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS,0,0);
	ULONG  HeapFragValue = 2;
	if( HeapSetInformation(hLFHeap
		,HeapCompatibilityInformation
		,&HeapFragValue
		,sizeof(HeapFragValue) ) )
	{
		//LFH Open
	}
	HeapDestroy(hLFHeap);
	return 0;
}