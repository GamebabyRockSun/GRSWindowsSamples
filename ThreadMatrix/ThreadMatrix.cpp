#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <time.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

//����Edit��Ļس�����
#define GRS_RETURN _T("\r\n")
//�����Լ����ڴ������ͷź���
#define GRS_CALLOC(s) ::HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,s)
#define GRS_SAFEFREE(p)	if(NULL != p){::HeapFree(GetProcessHeap(),0,p);p = NULL;}

//CPU�����ŵ�CPU�����ת����
#define CPUINDEXTOMASK(dwCPUIndex) (1 << (dwCPUIndex))

#define GRS_ASSERT(s) if(!(s)) {::DebugBreak();}

//����������������
#define M_RAWTYPE double			//Ԫ������
#define M_TYPE M_RAWTYPE*			//��������
#define M_INDEX unsigned int		//�±�����

//������������
M_TYPE MatrixCreate(M_INDEX m, M_INDEX n);
VOID MatrixRand(M_TYPE,M_INDEX,M_INDEX);
VOID MatrixMutiply(M_TYPE, M_TYPE, M_TYPE,M_INDEX,M_INDEX,M_INDEX);
VOID MatrixFree(M_TYPE);
VOID MatrixPrintf(M_TYPE,M_INDEX,M_INDEX);

//ȫ�ֱ���
const int m = 500;
const int r = 500;
const int n = 500;

M_TYPE A = NULL;
M_TYPE B = NULL;
M_TYPE C = NULL; 
M_TYPE D = NULL; 

DWORD WINAPI ThreadFunction( LPVOID lpParam )
{//�̺߳���
	//3��ѭ���㷨,����һ��,��ʵ�����е��̴߳�����
	UINT i = (UINT) lpParam;

	for( M_INDEX j = 0; j < n; j++ )
	{
		for( M_INDEX k = 0; k < r; k++ )
		{
			*(D + i * n + j) += *(A + i * r + k) * *(B + k * n + j);
		}
	}
	//�൱�ڵ���ԭʼ����˷��㷨,���������������ĳ˷�,����ŵ���Ӧ������
	//MatrixMutiply(A + i * r,B,D + i * n ,1,r,n);

	return 0; 
} 

int _tmain()
{
	GRS_USEPRINTF();
	srand((int)time(0));

	A = MatrixCreate(m,r);
	B = MatrixCreate(r,n);
	C = MatrixCreate(m,n); 
	D = MatrixCreate(m,n); 

	//�����������
	MatrixRand(A,m,r);
	MatrixRand(B,r,n);

	DWORD dwStart = GetTickCount();
	//��ͳ�㷨����
	MatrixMutiply(A,B,C,m,r,n);
	DWORD dwEnd = GetTickCount();
	GRS_PRINTF(_T("��׼�����㷨��ʱ:%I64dms\n"),dwEnd - dwStart);
	
	//�����߳̾������,A�����������߳�
	HANDLE*hThread = (HANDLE*)GRS_CALLOC(m * sizeof(HANDLE));
	DWORD dwThreadID = 0;

	//��ȡCPU����
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);

	//����m�������������B���,����ŵ�����D��
	for(int i = 0; i < m; i++)
	{
		hThread[i] = CreateThread(NULL,0,ThreadFunction,(LPVOID)i,CREATE_SUSPENDED,&dwThreadID);
		//����ȡ�����㽫�߳�ƽ�����ɵ�ָ����CPU��ȥ����
		//��Ϊ�߳�����CPU������ʱ��������,��������ָ��ֻ�ǽ���ƽ��
		//���ָ�ɿ���ע��,�Թ۲�ϵͳ������ȶ��̵߳���CPU������
		SetThreadAffinityMask(hThread[i] ,CPUINDEXTOMASK( i % si.dwNumberOfProcessors ));
		ResumeThread(hThread[i]);
	}

	//�ȴ������߳��˳� һ�����ֻ�ܵȴ�MAXIMUM_WAIT_OBJECTS���߳�
	for(int i = 0; i < (m % MAXIMUM_WAIT_OBJECTS) ; i ++)
	{
		WaitForMultipleObjects(min(MAXIMUM_WAIT_OBJECTS,m - (i * MAXIMUM_WAIT_OBJECTS)),hThread + (i * MAXIMUM_WAIT_OBJECTS),TRUE,INFINITE);
	}

	DWORD dwExitCode = 0;
	FILETIME tmCreation = {};
	FILETIME tmExit = {};
	FILETIME tmKernel = {};
	FILETIME tmUser = {};
	ULARGE_INTEGER bigTmp1 = {};
	ULARGE_INTEGER bigTmp2 = {};
	ULARGE_INTEGER bigSum = {};
	//�ر������߳̾��,�����̺߳�ʱ���
	for(DWORD i = 0; i < m; i ++)
	{
		GetExitCodeThread(hThread[i],&dwExitCode);
		//GRS_PRINTF(_T("�߳�[H:0x%08X]�˳����˳���:%u\n"),hThread[i],dwExitCode);
		GetThreadTimes(hThread[i],&tmCreation,&tmExit,&tmKernel,&tmUser);
		bigTmp1.HighPart	= tmKernel.dwHighDateTime;
		bigTmp1.LowPart		= tmKernel.dwLowDateTime;
		bigTmp2.HighPart	= tmUser.dwHighDateTime;
		bigTmp2.LowPart		= tmUser.dwLowDateTime;
		bigSum.QuadPart		+= bigTmp1.QuadPart + bigTmp2.QuadPart;
		CloseHandle(hThread[i]);
	}

	//������̺߳�ʱ���,�������Ƕ��CPUͬʱִ�еĺϼ�ֵ,����ʱ�仹Ҫ��СCPU�ĸ�����
	GRS_PRINTF(_T("���߳��㷨��ʱ:%I64dms\n"),(bigSum.QuadPart/10000)/si.dwNumberOfProcessors);

	//�ͷ��߳̾������
	GRS_SAFEFREE(hThread);

	//��ӡ���
	//GRS_PRINTF(_T("���̼߳�����:\n"))
	//MatrixPrintf(C,m,n);
	//GRS_PRINTF(_T("\n���̼߳�����:\n"))
	//MatrixPrintf(D,m,n);

	_tsystem(_T("PAUSE"));
	return 0;
}

//Ϊ����̬�����ڴ�ĺ���
M_TYPE MatrixCreate(M_INDEX m, M_INDEX n)
{
	GRS_ASSERT( m > 0 && n > 0 );
	return ( M_TYPE ) GRS_CALLOC( m * n * sizeof( M_RAWTYPE ) );
}

//����һ���������
VOID MatrixRand(M_TYPE Matrix,M_INDEX m,M_INDEX n)
{
	int iRand = rand();
	GRS_ASSERT( 0 != iRand );
	for(M_INDEX i = 0; i < m; i++ )
	{
		for( M_INDEX j = 0; j < n; j++ )
		{
			*(Matrix + i * n + j) = ((double)rand())/iRand;
		}
	}
}

//ԭʼ����˷�����
VOID MatrixMutiply(M_TYPE a, M_TYPE b, M_TYPE c,M_INDEX m,M_INDEX r,M_INDEX n)
{
	for( M_INDEX i = 0; i < m; i++ )
	{
		for( M_INDEX j = 0; j < n; j++ )
		{
			for( M_INDEX k = 0; k < r; k++ )
			{
				*(c + i * n + j) += *(a + i * r + k) * *(b + k * n + j);
			}
		}
	}
}

/* �ͷž����ڴ溯�� */
VOID MatrixFree(M_TYPE Matrix)
{
	GRS_SAFEFREE(Matrix);
} 

VOID MatrixPrintf(M_TYPE Matrix,M_INDEX m,M_INDEX n)
{
	GRS_USEPRINTF();
	for( M_INDEX i = 0; i < m; i ++ )
	{
		for( M_INDEX j = 0; j < n; j ++ )
		{
			GRS_PRINTF(_T("%.08f\t"),*(Matrix + i * n + j));
		}
		GRS_PRINTF(_T("%s"),GRS_RETURN);
	}
}
