#include <tchar.h>
#include <windows.h>
#include <time.h>

int _tmain(int argc, _TCHAR* argv[])
{
	//1、保留并提交内存
	VOID* pRecv = VirtualAlloc(NULL,1024*1024,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	//2、内存写入操作
	float* pfArray = (float*) pRecv;
	for(int i = 0; i < (1024*1024)/sizeof(float); i ++ )
	{
		pfArray[i] = 1.0f * rand();
	}
	//3、更改保护属性为只读
	DWORD dwOldProtect = 0;
	VirtualProtect(pRecv,1024*1024,PAGE_READONLY,&dwOldProtect);

	//4、读取所有值进行求和
	float fSum = 0.0f;
	for(int i = 0; i < (1024*1024)/sizeof(float); i ++ )
	{
		fSum += pfArray[i];
	}
	//5、视图写入第10元素，这将引起异常
	pfArray[9] = 0.0f;

	//6、直接释放
	VirtualFree(pRecv,0,MEM_RELEASE);

	return 0;
}