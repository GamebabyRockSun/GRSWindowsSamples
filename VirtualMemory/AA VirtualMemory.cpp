#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN // �� Windows ͷ���ų�����ʹ�õ�����
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

	//һ��ֱ�ӱ����ύ��ʽ
	//1���������ύ�ڴ�
	VOID* pRecv = VirtualAlloc(NULL,1024*1024,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	//2���ڴ����
	float* pfArray = (float*) pRecv;
	for(int i = 0; i < (1024*1024)/sizeof(float); i ++ )
	{
		pfArray[i] = 1.0f * rand();
	}
	//3��ֱ���ͷ�
	VirtualFree(pRecv,0,MEM_RELEASE);

	//�����ύ�ͱ����ֿ���ʽ
	//1�������ռ�
	BYTE* pMem = (BYTE*)VirtualAlloc(NULL,1024*1024,MEM_RESERVE,PAGE_NOACCESS);
	//2���ύҳ��
	VirtualAlloc(pMem + 4096,8 * 4096,MEM_COMMIT,PAGE_READWRITE);
	//3���������ύ��ҳ��
	double* pdbArray = (double*)(pMem + 4096);
	for(int i = 0; i < (8 * 4096)/sizeof(double); i ++ )
	{
		pdbArray[i] = 1.0f * rand();
	}
	//4���ͷ�����ҳ
	VirtualFree(pMem + 4 * 4096,4096,MEM_DECOMMIT);
	//5���ͷſռ�
	VirtualFree(pMem,0,MEM_RELEASE);
	return 0;
}