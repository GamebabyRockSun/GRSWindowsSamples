#include <tchar.h>
#include <windows.h>
#include <time.h>

int _tmain(int argc, _TCHAR* argv[])
{
	//设置进程的工作集配额
	SetProcessWorkingSetSize(GetCurrentProcess(),4 * 1024 * 1024,1024 * 1024 * 1024);

	//1、保留并提交内存
	VOID* pRecv = VirtualAlloc(NULL,1024*1024,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);

	//2、锁定内存
	VirtualLock(pRecv,1024*1024);
	//3、内存操作
	float* pfArray = (float*) pRecv;
	for(int i = 0; i < (1024*1024)/sizeof(float); i ++ )
	{
		pfArray[i] = 1.0f * rand();
	}
	//4、解锁内存
	VirtualUnlock(pRecv,1024*1024);
	//5、直接释放
	VirtualFree(pRecv,0,MEM_RELEASE);

	return 0;
}