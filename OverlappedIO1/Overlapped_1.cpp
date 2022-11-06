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

#define GRS_MAXWRITEPERTHREAD 100 //ÿ���߳����д�����
#define GRS_MAXWTHREAD		  20  //д���߳�����

#define GRS_OP_READ		0x1
#define GRS_OP_WRITE	0x2

struct ST_MY_OVERLAPPED
{
	OVERLAPPED m_ol;				//Overlapped �ṹ,��һ���ǵ��ǵ�һ����Ա
	HANDLE	   m_hFile;				//�������ļ����
	DWORD	   m_dwOp;				//��������GRS_OP_READ/GRS_OP_WRITE
	LPVOID	   m_pData;				//����������
	UINT	   m_nLen;				//���������ݳ���
	DWORD	   m_dwTimestamp;		//��ʼ������ʱ���
};

//IO��ɻص�����
VOID CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode,DWORD dwNumberOfBytesTransfered,
									  LPOVERLAPPED lpOverlapped);
//д�ļ����߳�
DWORD WINAPI WThread(LPVOID lpParameter);
//��ǰ�������ļ������ָ��
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
		GRS_PRINTF(_T("CreateFile(%s)ʧ��,������:0x%08x\n")
			,pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}
	
	//д��UNICODE�ļ���ǰ׺��,�Ա���ȷ��
	ST_MY_OVERLAPPED* pOl = (ST_MY_OVERLAPPED*)GRS_CALLOC(sizeof(ST_MY_OVERLAPPED));
	pOl->m_dwOp = GRS_OP_WRITE;
	pOl->m_hFile = hTxtFile;
	pOl->m_pData = GRS_CALLOC(sizeof(WORD));
	*((WORD*)pOl->m_pData) = MAKEWORD(0xff,0xfe);//UNICODE�ı��ļ���Ҫ��ǰ׺
	pOl->m_nLen = sizeof(WORD);

	//ƫ���ļ�ָ��
	pOl->m_ol.Offset = g_liFilePointer.LowPart;
	pOl->m_ol.OffsetHigh = g_liFilePointer.HighPart;
	g_liFilePointer.QuadPart += pOl->m_nLen;

	pOl->m_dwTimestamp = GetTickCount();//��¼ʱ���

	WriteFileEx((HANDLE)hTxtFile,pOl->m_pData,pOl->m_nLen,(LPOVERLAPPED)&pOl->m_ol,FileIOCompletionRoutine);

	for(int i = 0; i < GRS_MAXWTHREAD; i ++)
	{
		ahWThread[i] = GRS_BEGINTHREAD(WThread,hTxtFile);
	}

	//�����ߵȴ���Щд���߳̽���,ͬʱ����ɾ���״̬�ȴ��Լ����õ�д���Ƿ����
	while(WAIT_IO_COMPLETION == WaitForMultipleObjectsEx(GRS_MAXWTHREAD,ahWThread,TRUE,INFINITE,TRUE))
	{
		GRS_PRINTF(_T("�߳�[0x%x]�ɹ�ִ��һ��IO��ɻص�,��������ȴ�.....\n")
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
	GRS_PRINTF(_T("�߳�[0x%x]ִ��IO��ɻص�����,��ɲ���(%s),����(0x%08x)����(%ubytes),д��ʱ���(%u)��ǰʱ���(%u)ʱ��(%u)\n"),
		GetCurrentThreadId(),GRS_OP_WRITE == pMyOl->m_dwOp?_T("Write"):_T("Read"),
		pMyOl->m_pData,pMyOl->m_nLen,pMyOl->m_dwTimestamp,dwCurTimestamp,dwCurTimestamp - pMyOl->m_dwTimestamp);
	//ɾ������
	GRS_SAFEFREE(pMyOl->m_pData);
	GRS_SAFEFREE(pMyOl);
}

#define MAX_LOGLEN 256
DWORD WINAPI WThread(LPVOID lpParameter)
{
	GRS_USEPRINTF();
	//GRS_PRINTF(_T("�߳�[0x%x]��ʼд��......\n"),GetCurrentThreadId());
	TCHAR pTxtContext[MAX_LOGLEN] = {};
	ST_MY_OVERLAPPED* pOl = NULL;
	size_t szLen = 0;
	
	StringCchPrintf(pTxtContext,MAX_LOGLEN,_T("����һ��ģ�����־��¼,���߳�[0x%x]д��\r\n")
		,GetCurrentThreadId());
	StringCchLength(pTxtContext,MAX_LOGLEN,&szLen);
	szLen += 1;
	int i = 0;
	for(;i < GRS_MAXWRITEPERTHREAD; i ++)
	{
		LPTSTR pWriteText = (LPTSTR)GRS_CALLOC(szLen * sizeof(TCHAR));
		StringCchCopy(pWriteText,szLen,pTxtContext);

		//Ϊÿ����������һ���Զ���OL�ṹ��,ʵ��Ӧ�������￼��ʹ���ڴ��
		pOl = (ST_MY_OVERLAPPED*)GRS_CALLOC(sizeof(ST_MY_OVERLAPPED));
		pOl->m_dwOp		= GRS_OP_WRITE;
		pOl->m_hFile	= (HANDLE)lpParameter;
		pOl->m_pData	= pWriteText;
		pOl->m_nLen		= szLen * sizeof(TCHAR);

		//����ʹ��ԭ�Ӳ���ͬ���ļ�ָ��,д�벻���໥����
		//����ط�������lock-free�㷨�ľ���,ʹ���˻�����CAS���������ļ�ָ��
		//�ȴ�ͳ��ʹ�ùؼ�����β��ȴ��ķ���,�����õķ���Ҫ���ɵĶ�,�����Ĵ���ҲС
		*((LONGLONG*)&pOl->m_ol.Pointer) = InterlockedCompareExchange64(&g_liFilePointer.QuadPart,
			g_liFilePointer.QuadPart + pOl->m_nLen,g_liFilePointer.QuadPart);
	
		//�ļ�ָ�����������ͬ��,д����໥����
		//*((LONGLONG*)&pOl->m_ol.Pointer) = g_liFilePointer.QuadPart;
		//g_liFilePointer.QuadPart += szLen;

		pOl->m_dwTimestamp = GetTickCount();//��¼ʱ���
		//д��
		WriteFileEx((HANDLE)lpParameter,pOl->m_pData,pOl->m_nLen,(LPOVERLAPPED)&pOl->m_ol
			,FileIOCompletionRoutine);

		//WriteFileEx �� SleepEx�м���Ըɺܶ�����,���߳���Ҫ֪���Ǹ�д���Ƿ����ʱ�ٽ���ɾ���״̬�ȴ�
		//do something.....

		if( WAIT_IO_COMPLETION == SleepEx(INFINITE,TRUE))
		{
			//GRS_PRINTF(_T("�߳�[0x%x]�ɹ�ִ��һ��IO��ɻص�,����ִ��.....\n")
			//		,GetCurrentThreadId());
		}
	}

	return i;
}