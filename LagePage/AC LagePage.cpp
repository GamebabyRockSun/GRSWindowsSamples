#include <TCHAR.h>
#include <windows.h>
#include <time.h>

int _tmain(int argc,_TCHAR* argv[])
{
	srand((unsigned int)time(NULL));
	BYTE* pBuffer = NULL;
	//1��ʹ��GetLargePageMinimum�õ���ҳ��ĳߴ�
	SIZE_T szLagePage = ::GetLargePageMinimum();
	//2�������ռ�
	pBuffer = (BYTE*)VirtualAlloc(0,64 * szLagePage, MEM_RESERVE, PAGE_READWRITE);
	//3��ʹ��MEM_LARGE_PAGES��־�ύ�ڴ�
	VirtualAlloc(pBuffer,2 * szLagePage, MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
	//4��ʹ���ڴ�
	double* pdbArray = (double*)pBuffer;
	for(int i = 0; i < ( 2 * szLagePage / sizeof(double) ) ;i ++)
	{
		pdbArray[i] = 1.0f * rand();
	}
	//5���ͷ�
	VirtualFree(pBuffer,0,MEM_RELEASE);
	return 0;
}