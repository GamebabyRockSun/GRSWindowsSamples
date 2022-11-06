#include <TCHAR.h>
#include <windows.h>
#include <time.h>

int _tmain(int argc,_TCHAR* argv[])
{
	srand((unsigned int)time(NULL));
	BYTE* pBuffer = NULL;
	//1、使用GetLargePageMinimum得到大页面的尺寸
	SIZE_T szLagePage = ::GetLargePageMinimum();
	//2、保留空间
	pBuffer = (BYTE*)VirtualAlloc(0,64 * szLagePage, MEM_RESERVE, PAGE_READWRITE);
	//3、使用MEM_LARGE_PAGES标志提交内存
	VirtualAlloc(pBuffer,2 * szLagePage, MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
	//4、使用内存
	double* pdbArray = (double*)pBuffer;
	for(int i = 0; i < ( 2 * szLagePage / sizeof(double) ) ;i ++)
	{
		pdbArray[i] = 1.0f * rand();
	}
	//5、释放
	VirtualFree(pBuffer,0,MEM_RELEASE);
	return 0;
}