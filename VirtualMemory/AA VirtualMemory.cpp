#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN // 从 Windows 头中排除极少使用的资料
#include <windows.h>
#include <time.h>
#include <tchar.h>
#include <stdlib.h>

int _tmain(int argc, _TCHAR* argv[])
{
	srand((unsigned int)time(NULL));
	const int iCnt = 100000;
	float fArray[iCnt];
	for(int i = 0;i < iCnt; i ++)
	{
		fArray[i] = 1.0f * rand();
	}
	_tsystem(_T("PAUSE"));

	//一、直接保留提交方式
	//1、保留并提交内存
	VOID* pRecv = VirtualAlloc(NULL,1024*1024,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	//2、内存操作
	float* pfArray = (float*) pRecv;
	for(int i = 0; i < (1024*1024)/sizeof(float); i ++ )
	{
		pfArray[i] = 1.0f * rand();
	}
	//3、直接释放
	VirtualFree(pRecv,0,MEM_RELEASE);

	//二、提交和保留分开方式
	//1、保留空间
	BYTE* pMem = (BYTE*)VirtualAlloc(NULL,1024*1024,MEM_RESERVE,PAGE_NOACCESS);
	//2、提交页面
	VirtualAlloc(pMem + 4096,8 * 4096,MEM_COMMIT,PAGE_READWRITE);
	//3、操作已提交的页面
	double* pdbArray = (double*)(pMem + 4096);
	for(int i = 0; i < (8 * 4096)/sizeof(double); i ++ )
	{
		pdbArray[i] = 1.0f * rand();
	}
	//4、释放物理页
	VirtualFree(pMem + 4 * 4096,4096,MEM_DECOMMIT);
	//5、释放空间
	VirtualFree(pMem,0,MEM_RELEASE);
	return 0;
}