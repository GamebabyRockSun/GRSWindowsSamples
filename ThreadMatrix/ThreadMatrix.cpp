#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <time.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

//定义Edit框的回车换行
#define GRS_RETURN _T("\r\n")
//声明自己的内存分配和释放函数
#define GRS_CALLOC(s) ::HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,s)
#define GRS_SAFEFREE(p)	if(NULL != p){::HeapFree(GetProcessHeap(),0,p);p = NULL;}

//CPU索引号到CPU掩码的转换宏
#define CPUINDEXTOMASK(dwCPUIndex) (1 << (dwCPUIndex))

#define GRS_ASSERT(s) if(!(s)) {::DebugBreak();}

//定义矩阵的数据类型
#define M_RAWTYPE double			//元素类型
#define M_TYPE M_RAWTYPE*			//矩阵类型
#define M_INDEX unsigned int		//下标类型

//函数声明部分
M_TYPE MatrixCreate(M_INDEX m, M_INDEX n);
VOID MatrixRand(M_TYPE,M_INDEX,M_INDEX);
VOID MatrixMutiply(M_TYPE, M_TYPE, M_TYPE,M_INDEX,M_INDEX,M_INDEX);
VOID MatrixFree(M_TYPE);
VOID MatrixPrintf(M_TYPE,M_INDEX,M_INDEX);

//全局变量
const int m = 500;
const int r = 500;
const int n = 500;

M_TYPE A = NULL;
M_TYPE B = NULL;
M_TYPE C = NULL; 
M_TYPE D = NULL; 

DWORD WINAPI ThreadFunction( LPVOID lpParam )
{//线程函数
	//3重循环算法,少了一层,其实被并行的线程代替了
	UINT i = (UINT) lpParam;

	for( M_INDEX j = 0; j < n; j++ )
	{
		for( M_INDEX k = 0; k < r; k++ )
		{
			*(D + i * n + j) += *(A + i * r + k) * *(B + k * n + j);
		}
	}
	//相当于调用原始矩阵乘法算法,计算行向量与矩阵的乘法,结果放到对应的行中
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

	//生成随机矩阵
	MatrixRand(A,m,r);
	MatrixRand(B,r,n);

	DWORD dwStart = GetTickCount();
	//传统算法计算
	MatrixMutiply(A,B,C,m,r,n);
	DWORD dwEnd = GetTickCount();
	GRS_PRINTF(_T("标准定义算法耗时:%I64dms\n"),dwEnd - dwStart);
	
	//创建线程句柄数组,A矩阵行数个线程
	HANDLE*hThread = (HANDLE*)GRS_CALLOC(m * sizeof(HANDLE));
	DWORD dwThreadID = 0;

	//获取CPU个数
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);

	//创建m个行向量与矩阵B相乘,结果放到矩阵D里
	for(int i = 0; i < m; i++)
	{
		hThread[i] = CreateThread(NULL,0,ThreadFunction,(LPVOID)i,CREATE_SUSPENDED,&dwThreadID);
		//利用取余运算将线程平均分派到指定的CPU上去计算
		//因为线程数与CPU数量有时不能整除,所以这种指派只是近似平均
		//这个指派可以注释,以观察系统自身调度多线程到多CPU的能力
		SetThreadAffinityMask(hThread[i] ,CPUINDEXTOMASK( i % si.dwNumberOfProcessors ));
		ResumeThread(hThread[i]);
	}

	//等待所有线程退出 一次最多只能等待MAXIMUM_WAIT_OBJECTS个线程
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
	//关闭所有线程句柄,计算线程耗时情况
	for(DWORD i = 0; i < m; i ++)
	{
		GetExitCodeThread(hThread[i],&dwExitCode);
		//GRS_PRINTF(_T("线程[H:0x%08X]退出，退出码:%u\n"),hThread[i],dwExitCode);
		GetThreadTimes(hThread[i],&tmCreation,&tmExit,&tmKernel,&tmUser);
		bigTmp1.HighPart	= tmKernel.dwHighDateTime;
		bigTmp1.LowPart		= tmKernel.dwLowDateTime;
		bigTmp2.HighPart	= tmUser.dwHighDateTime;
		bigTmp2.LowPart		= tmUser.dwLowDateTime;
		bigSum.QuadPart		+= bigTmp1.QuadPart + bigTmp2.QuadPart;
		CloseHandle(hThread[i]);
	}

	//输出多线程耗时情况,这个结果是多个CPU同时执行的合计值,所以时间还要缩小CPU的个数倍
	GRS_PRINTF(_T("多线程算法耗时:%I64dms\n"),(bigSum.QuadPart/10000)/si.dwNumberOfProcessors);

	//释放线程句柄数组
	GRS_SAFEFREE(hThread);

	//打印结果
	//GRS_PRINTF(_T("单线程计算结果:\n"))
	//MatrixPrintf(C,m,n);
	//GRS_PRINTF(_T("\n多线程计算结果:\n"))
	//MatrixPrintf(D,m,n);

	_tsystem(_T("PAUSE"));
	return 0;
}

//为矩阵动态分配内存的函数
M_TYPE MatrixCreate(M_INDEX m, M_INDEX n)
{
	GRS_ASSERT( m > 0 && n > 0 );
	return ( M_TYPE ) GRS_CALLOC( m * n * sizeof( M_RAWTYPE ) );
}

//产生一个随机矩阵
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

//原始矩阵乘法函数
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

/* 释放矩阵内存函数 */
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
