#include <tchar.h>
#include <Windows.h>
#include <time.h>

int _tmain()
{
	srand((unsigned int)time(NULL));
	HANDLE hHeap = GetProcessHeap();

	const int iCnt = 1000;
	float* fArray = (float*)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,iCnt * sizeof(float));
	for(int i = 0; i < iCnt; i++)
	{
		fArray[i] = 1.0f * rand();
	}
	fArray = (float*)HeapReAlloc(hHeap,HEAP_ZERO_MEMORY,fArray,2 * iCnt * sizeof(float) );
	for(int i = iCnt; i < 2*iCnt;i ++)
	{
		fArray[i] = 1.0f * rand();
	}
	HeapFree(hHeap,0,fArray);

	hHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS,0,0);
	fArray = (float*)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,iCnt * sizeof(float));
	for(int i = 0; i < iCnt; i++)
	{
		fArray[i] = 1.0f * rand();
	}
	fArray = (float*)HeapReAlloc(hHeap,HEAP_ZERO_MEMORY,fArray,2 * iCnt * sizeof(float) );
	for(int i = iCnt; i < 2*iCnt;i ++)
	{
		fArray[i] = 1.0f * rand();
	}
	HeapFree(hHeap,0,fArray);
	HeapDestroy(hHeap);



	return 0;
}