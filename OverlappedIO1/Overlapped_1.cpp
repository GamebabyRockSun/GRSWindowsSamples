#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

#define GRS_BEGINTHREAD(Fun,Param) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Fun,Param,0,NULL)

#define GRS_MAXWRITEPERTHREAD 100 //每个线程最大写入次数
#define GRS_MAXWTHREAD		  20  //写入线程数量

#define GRS_OP_READ		0x1
#define GRS_OP_WRITE	0x2

struct ST_MY_OVERLAPPED
{
	OVERLAPPED m_ol;				//Overlapped 结构,不一定非得是第一个成员
	HANDLE	   m_hFile;				//操作的文件句柄
	DWORD	   m_dwOp;				//操作类型GRS_OP_READ/GRS_OP_WRITE
	LPVOID	   m_pData;				//操作的数据
	UINT	   m_nLen;				//操作的数据长度
	DWORD	   m_dwTimestamp;		//起始操作的时间戳
};

//IO完成回调函数
VOID CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode,DWORD dwNumberOfBytesTransfered,
									  LPOVERLAPPED lpOverlapped);
//写文件的线程
DWORD WINAPI WThread(LPVOID lpParameter);
//当前操作的文件对象的指针
LARGE_INTEGER g_liFilePointer = {};

int _tmain()
{
	GRS_USEPRINTF();
	HANDLE ahWThread[GRS_MAXWTHREAD] = {};
	TCHAR pFileName[MAX_PATH] = _T("OverlappedIO_1.txt");
	DWORD dwWrited = 0;
	HANDLE hTxtFile = CreateFile(pFileName
		,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,NULL);
	if(INVALID_HANDLE_VALUE == hTxtFile)
	{
		GRS_PRINTF(_T("CreateFile(%s)失败,错误码:0x%08x\n")
			,pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}
	
	//写入UNICODE文件的前缀码,以便正确打开
	ST_MY_OVERLAPPED* pOl = (ST_MY_OVERLAPPED*)GRS_CALLOC(sizeof(ST_MY_OVERLAPPED));
	pOl->m_dwOp = GRS_OP_WRITE;
	pOl->m_hFile = hTxtFile;
	pOl->m_pData = GRS_CALLOC(sizeof(WORD));
	*((WORD*)pOl->m_pData) = MAKEWORD(0xff,0xfe);//UNICODE文本文件需要的前缀
	pOl->m_nLen = sizeof(WORD);

	//偏移文件指针
	pOl->m_ol.Offset = g_liFilePointer.LowPart;
	pOl->m_ol.OffsetHigh = g_liFilePointer.HighPart;
	g_liFilePointer.QuadPart += pOl->m_nLen;

	pOl->m_dwTimestamp = GetTickCount();//记录时间戳

	WriteFileEx((HANDLE)hTxtFile,pOl->m_pData,pOl->m_nLen,(LPOVERLAPPED)&pOl->m_ol,FileIOCompletionRoutine);

	for(int i = 0; i < GRS_MAXWTHREAD; i ++)
	{
		ahWThread[i] = GRS_BEGINTHREAD(WThread,hTxtFile);
	}

	//让主线等待这些写入线程结束,同时进入可警告状态等待自己调用的写入是否完成
	while(WAIT_IO_COMPLETION == WaitForMultipleObjectsEx(GRS_MAXWTHREAD,ahWThread,TRUE,INFINITE,TRUE))
	{
		GRS_PRINTF(_T("线程[0x%x]成功执行一次IO完成回调,继续进入等待.....\n")
			,GetCurrentThreadId());
	}

	for(int i = 0; i < GRS_MAXWTHREAD; i ++)
	{
		CloseHandle(ahWThread[i]);
	}
	 
	if(INVALID_HANDLE_VALUE != hTxtFile )
	{
		CloseHandle(hTxtFile);
		hTxtFile = INVALID_HANDLE_VALUE;
	}


	_tsystem(_T("PAUSE"));
	return 0;
}


VOID CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode,DWORD dwNumberOfBytesTransfered,
									  LPOVERLAPPED lpOverlapped)
{
	
	ST_MY_OVERLAPPED* pMyOl = CONTAINING_RECORD(lpOverlapped,ST_MY_OVERLAPPED,m_ol);
	GRS_USEPRINTF();
	DWORD dwCurTimestamp = GetTickCount();
	GRS_PRINTF(_T("线程[0x%x]执行IO完成回调函数,完成操作(%s),缓冲(0x%08x)长度(%ubytes),写入时间戳(%u)当前时间戳(%u)时差(%u)\n"),
		GetCurrentThreadId(),GRS_OP_WRITE == pMyOl->m_dwOp?_T("Write"):_T("Read"),
		pMyOl->m_pData,pMyOl->m_nLen,pMyOl->m_dwTimestamp,dwCurTimestamp,dwCurTimestamp - pMyOl->m_dwTimestamp);
	//删除缓冲
	GRS_SAFEFREE(pMyOl->m_pData);
	GRS_SAFEFREE(pMyOl);
}

#define MAX_LOGLEN 256
DWORD WINAPI WThread(LPVOID lpParameter)
{
	GRS_USEPRINTF();
	//GRS_PRINTF(_T("线程[0x%x]开始写入......\n"),GetCurrentThreadId());
	TCHAR pTxtContext[MAX_LOGLEN] = {};
	ST_MY_OVERLAPPED* pOl = NULL;
	size_t szLen = 0;
	
	StringCchPrintf(pTxtContext,MAX_LOGLEN,_T("这是一条模拟的日志记录,由线程[0x%x]写入\r\n")
		,GetCurrentThreadId());
	StringCchLength(pTxtContext,MAX_LOGLEN,&szLen);
	szLen += 1;
	int i = 0;
	for(;i < GRS_MAXWRITEPERTHREAD; i ++)
	{
		LPTSTR pWriteText = (LPTSTR)GRS_CALLOC(szLen * sizeof(TCHAR));
		StringCchCopy(pWriteText,szLen,pTxtContext);

		//为每个操作申请一个自定义OL结构体,实际应用中这里考虑使用内存池
		pOl = (ST_MY_OVERLAPPED*)GRS_CALLOC(sizeof(ST_MY_OVERLAPPED));
		pOl->m_dwOp		= GRS_OP_WRITE;
		pOl->m_hFile	= (HANDLE)lpParameter;
		pOl->m_pData	= pWriteText;
		pOl->m_nLen		= szLen * sizeof(TCHAR);

		//这里使用原子操作同步文件指针,写入不会相互覆盖
		//这个地方体现了lock-free算法的精髓,使用了基本的CAS操作控制文件指针
		//比传统的使用关键代码段并等待的方法,这里用的方法要轻巧的多,付出的代价也小
		*((LONGLONG*)&pOl->m_ol.Pointer) = InterlockedCompareExchange64(&g_liFilePointer.QuadPart,
			g_liFilePointer.QuadPart + pOl->m_nLen,g_liFilePointer.QuadPart);
	
		//文件指针操作不进行同步,写入会相互覆盖
		//*((LONGLONG*)&pOl->m_ol.Pointer) = g_liFilePointer.QuadPart;
		//g_liFilePointer.QuadPart += szLen;

		pOl->m_dwTimestamp = GetTickCount();//记录时间戳
		//写入
		WriteFileEx((HANDLE)lpParameter,pOl->m_pData,pOl->m_nLen,(LPOVERLAPPED)&pOl->m_ol
			,FileIOCompletionRoutine);

		//WriteFileEx 和 SleepEx中间可以干很多事情,当线程需要知道那个写入是否完成时再进入可警告状态等待
		//do something.....

		if( WAIT_IO_COMPLETION == SleepEx(INFINITE,TRUE))
		{
			//GRS_PRINTF(_T("线程[0x%x]成功执行一次IO完成回调,继续执行.....\n")
			//		,GetCurrentThreadId());
		}
	}

	return i;
}