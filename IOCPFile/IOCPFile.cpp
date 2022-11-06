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

#define GRS_OP_READ		0x1		//��ȡ����
#define GRS_OP_WRITE	0x2		//д�����
#define GRS_OP_EXIT		0x3		//�ر�IOCP����

struct ST_MY_OVERLAPPED
{
	OVERLAPPED m_ol;				//Overlapped �ṹ,��һ���ǵ��ǵ�һ����Ա
	HANDLE	   m_hFile;				//�������ļ����
	DWORD	   m_dwOp;				//��������GRS_OP_READ/GRS_OP_WRITE
	LPVOID	   m_pData;				//����������
	UINT	   m_nLen;				//���������ݳ���
	DWORD	   m_dwTimestamp;		//��ʼ������ʱ���
};


//IOCP�̳߳��̺߳���
DWORD WINAPI IOCPThread(LPVOID lpParameter);
//д�ļ����߳�
DWORD WINAPI WThread(LPVOID lpParameter);
//��ǰ�������ļ������ָ��
LARGE_INTEGER g_liFilePointer = {};

int _tmain()
{
	GRS_USEPRINTF();
	HANDLE* phIocpThread = NULL;
	HANDLE ahWThread[GRS_MAXWTHREAD] = {};
	TCHAR pFileName[MAX_PATH] = _T("IOCPFile.txt");
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
	
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);
	//����IOCP�ں˶���,������󲢷�CPU�������߳�
	HANDLE hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,NULL,si.dwNumberOfProcessors);

	//ʵ�ʴ���2��CPU�������߳�
	phIocpThread = (HANDLE*)GRS_CALLOC(2 * si.dwNumberOfProcessors * sizeof(HANDLE));
	for(DWORD i = 0; i < 2 * si.dwNumberOfProcessors; i ++)
	{
		phIocpThread[i] = GRS_BEGINTHREAD(IOCPThread,hIocp);
	}

	//���ļ��������ɶ˿ڶ����,�ڶ��ε���CreateIoCompletionPort
	CreateIoCompletionPort(hTxtFile,hIocp,NULL,0);

	//д��UNICODE�ļ���ǰ׺��,�Ա���ȷ��
	ST_MY_OVERLAPPED* pMyOl = (ST_MY_OVERLAPPED*)GRS_CALLOC(sizeof(ST_MY_OVERLAPPED));
	pMyOl->m_dwOp = GRS_OP_WRITE;
	pMyOl->m_hFile = hTxtFile;
	pMyOl->m_pData = GRS_CALLOC(sizeof(WORD));
	*((WORD*)pMyOl->m_pData) = MAKEWORD(0xff,0xfe);//UNICODE�ı��ļ���Ҫ��ǰ׺
	pMyOl->m_nLen = sizeof(WORD);

	//ƫ���ļ�ָ��
	pMyOl->m_ol.Offset = g_liFilePointer.LowPart;
	pMyOl->m_ol.OffsetHigh = g_liFilePointer.HighPart;
	g_liFilePointer.QuadPart += pMyOl->m_nLen;

	pMyOl->m_dwTimestamp = GetTickCount();//��¼ʱ���

	WriteFile((HANDLE)hTxtFile,pMyOl->m_pData,pMyOl->m_nLen,NULL,(LPOVERLAPPED)&pMyOl->m_ol);

	for(int i = 0; i < GRS_MAXWTHREAD; i ++)
	{
		ahWThread[i] = GRS_BEGINTHREAD(WThread,hTxtFile);
	}

	//�����ߵȴ���Щд���߳̽���
	WaitForMultipleObjects(GRS_MAXWTHREAD,ahWThread,TRUE,INFINITE);

	for(int i = 0; i < GRS_MAXWTHREAD; i ++)
	{
		CloseHandle(ahWThread[i]);
	}
	 
	if(INVALID_HANDLE_VALUE != hTxtFile )
	{
		CloseHandle(hTxtFile);
		hTxtFile = INVALID_HANDLE_VALUE;
	}

	for(DWORD i = 0; i < 2 * si.dwNumberOfProcessors; i ++)
	{
		//��IOCP�̳߳ط����˳���Ϣ
		pMyOl = (ST_MY_OVERLAPPED*)GRS_CALLOC(sizeof(ST_MY_OVERLAPPED));
		pMyOl->m_dwOp = GRS_OP_EXIT;
		pMyOl->m_dwTimestamp = GetTickCount();
		PostQueuedCompletionStatus(hIocp,0,0,&pMyOl->m_ol);
	}
	WaitForMultipleObjects(2*si.dwNumberOfProcessors,phIocpThread,TRUE,INFINITE);

	for(DWORD i = 0; i < 2 * si.dwNumberOfProcessors; i ++)
	{
		CloseHandle(phIocpThread[i]);
	}
	GRS_SAFEFREE(phIocpThread);

	CloseHandle(hIocp);

	_tsystem(_T("PAUSE"));
	return 0;
}

//IOCP�̳߳��̺߳���
DWORD WINAPI IOCPThread(LPVOID lpParameter)
{
	GRS_USEPRINTF();
	ULONG_PTR    Key = NULL;
	OVERLAPPED* lpOverlapped = NULL;
	ST_MY_OVERLAPPED* pMyOl = NULL;
	DWORD dwBytesTransfered = 0;
	DWORD dwFlags = 0;
	BOOL bRet = TRUE;

	HANDLE hIocp = (HANDLE)lpParameter;
	BOOL bLoop = TRUE;
	while (bLoop)
	{
		bRet = GetQueuedCompletionStatus(hIocp,&dwBytesTransfered,(PULONG_PTR)&Key,&lpOverlapped,INFINITE);
		pMyOl = CONTAINING_RECORD(lpOverlapped,ST_MY_OVERLAPPED,m_ol);

		if( FALSE == bRet )
		{
			GRS_PRINTF(_T("IOCPThread: GetQueuedCompletionStatus ����ʧ��,������: 0x%08x �ڲ�������[0x%08x]\n"),
				GetLastError(), lpOverlapped->Internal);
			continue;
		}

		DWORD dwCurTimestamp = GetTickCount();
		
		switch(pMyOl->m_dwOp)
		{
		case GRS_OP_WRITE:
			{
				GRS_PRINTF(_T("�߳�[0x%x]�õ�IO���֪ͨ,��ɲ���(%s),����(0x%08x)����(%ubytes),д��ʱ���(%u)��ǰʱ���(%u)ʱ��(%u)\n"),
					GetCurrentThreadId(),GRS_OP_WRITE == pMyOl->m_dwOp?_T("Write"):_T("Read"),
					pMyOl->m_pData,pMyOl->m_nLen,pMyOl->m_dwTimestamp,dwCurTimestamp,dwCurTimestamp - pMyOl->m_dwTimestamp);

				GRS_SAFEFREE(pMyOl->m_pData);
				GRS_SAFEFREE(pMyOl);
			}
			break;
		case GRS_OP_READ:
			{

			}
			break;
		case GRS_OP_EXIT:
			{
				GRS_PRINTF(_T("IOCP�߳�[0x%x]�õ��˳�֪ͨ,ʱ���(%u)��ǰʱ���(%u)ʱ��(%u),IOCP�߳��˳�\n"),
					GetCurrentThreadId(),pMyOl->m_dwTimestamp,dwCurTimestamp,dwCurTimestamp - pMyOl->m_dwTimestamp);
				GRS_SAFEFREE(pMyOl->m_pData);
				GRS_SAFEFREE(pMyOl);

				bLoop = FALSE;//�˳�ѭ��
			}
			break;
		default:
			{
				bLoop = FALSE;
			}
			break;
		}

	}

	return 0;
}

#define MAX_LOGLEN 256
DWORD WINAPI WThread(LPVOID lpParameter)
{
	GRS_USEPRINTF();
	//GRS_PRINTF(_T("�߳�[0x%x]��ʼд��......\n"),GetCurrentThreadId());
	TCHAR pTxtContext[MAX_LOGLEN] = {};
	ST_MY_OVERLAPPED* pMyOl = NULL;
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
		pMyOl = (ST_MY_OVERLAPPED*)GRS_CALLOC(sizeof(ST_MY_OVERLAPPED));
		pMyOl->m_dwOp	= GRS_OP_WRITE;
		pMyOl->m_hFile	= (HANDLE)lpParameter;
		pMyOl->m_pData	= pWriteText;
		pMyOl->m_nLen	= szLen * sizeof(TCHAR);

		//����ʹ��ԭ�Ӳ���ͬ���ļ�ָ��,д�벻���໥����
		//����ط�������lock-free�㷨�ľ���,ʹ���˻�����CAS���������ļ�ָ��
		//�ȴ�ͳ��ʹ�ùؼ�����β��ȴ��ķ���,�����õķ���Ҫ���ɵĶ�,�����Ĵ���ҲС
		*((LONGLONG*)&pMyOl->m_ol.Pointer) = InterlockedCompareExchange64(&g_liFilePointer.QuadPart,
			g_liFilePointer.QuadPart + pMyOl->m_nLen,g_liFilePointer.QuadPart);
	
		//�ļ�ָ�����������ͬ��,д����໥����
		//*((LONGLONG*)&pMyOl->m_ol.Pointer) = g_liFilePointer.QuadPart;
		//g_liFilePointer.QuadPart += szLen;

		pMyOl->m_dwTimestamp = GetTickCount();//��¼ʱ���
		//д��
		WriteFile((HANDLE)lpParameter,pMyOl->m_pData,pMyOl->m_nLen,NULL,(LPOVERLAPPED)&pMyOl->m_ol);

		//WriteFileEx �� SleepEx�м���Ըɺܶ�����,���߳���Ҫ֪���Ǹ�д���Ƿ����ʱ�ٽ���ɾ���״̬�ȴ�
		//do something.....
	}

	return i;
}