#include <tchar.h>
#include <windows.h>
#include <time.h>

int _tmain(int argc, _TCHAR* argv[])
{
	//1���������ύ�ڴ�
	VOID* pRecv = VirtualAlloc(NULL,1024*1024,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	//2���ڴ�д�����
	float* pfArray = (float*) pRecv;
	for(int i = 0; i < (1024*1024)/sizeof(float); i ++ )
	{
		pfArray[i] = 1.0f * rand();
	}
	//3�����ı�������Ϊֻ��
	DWORD dwOldProtect = 0;
	VirtualProtect(pRecv,1024*1024,PAGE_READONLY,&dwOldProtect);

	//4����ȡ����ֵ�������
	float fSum = 0.0f;
	for(int i = 0; i < (1024*1024)/sizeof(float); i ++ )
	{
		fSum += pfArray[i];
	}
	//5����ͼд���10Ԫ�أ��⽫�����쳣
	pfArray[9] = 0.0f;

	//6��ֱ���ͷ�
	VirtualFree(pRecv,0,MEM_RELEASE);

	return 0;
}