#include <tchar.h>
#include <windows.h>
#include <time.h>

int _tmain(int argc, _TCHAR* argv[])
{
	//���ý��̵Ĺ��������
	SetProcessWorkingSetSize(GetCurrentProcess(),4 * 1024 * 1024,1024 * 1024 * 1024);

	//1���������ύ�ڴ�
	VOID* pRecv = VirtualAlloc(NULL,1024*1024,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);

	//2�������ڴ�
	VirtualLock(pRecv,1024*1024);
	//3���ڴ����
	float* pfArray = (float*) pRecv;
	for(int i = 0; i < (1024*1024)/sizeof(float); i ++ )
	{
		pfArray[i] = 1.0f * rand();
	}
	//4�������ڴ�
	VirtualUnlock(pRecv,1024*1024);
	//5��ֱ���ͷ�
	VirtualFree(pRecv,0,MEM_RELEASE);

	return 0;
}