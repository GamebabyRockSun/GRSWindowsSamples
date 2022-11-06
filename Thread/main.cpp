#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define MAX_THREADS 10	//最大线程数

DWORD WINAPI MyThreadFunction( LPVOID lpParam );
void ErrorHandler(LPTSTR lpszFunction);

//自定义线程数据
typedef struct MyData 
{
	int val1;
	int val2;
} MYDATA, *PMYDATA;

int _tmain()
{
	PMYDATA pDataArray[MAX_THREADS];
	DWORD   dwThreadIdArray[MAX_THREADS];
	HANDLE  hThreadArray[MAX_THREADS]; 

	// 创建线程循环
	for( int i=0; i<MAX_THREADS; i++ )
	{
		pDataArray[i] = (PMYDATA)GRS_CALLOC(sizeof(MYDATA));
		pDataArray[i]->val1 = i;
		pDataArray[i]->val2 = i+100;

		hThreadArray[i] = CreateThread(NULL,0,MyThreadFunction,pDataArray[i],0,&dwThreadIdArray[i]); 
		if (hThreadArray[i] == NULL) 
		{
			ErrorHandler(_T("CreateThread"));
			ExitProcess(3);
		}
	} 
	WaitForMultipleObjects(MAX_THREADS, hThreadArray, TRUE, INFINITE);
	
	for(int i=0; i<MAX_THREADS; i++)
	{
		CloseHandle(hThreadArray[i]);
		GRS_SAFEFREE(pDataArray[i]);
	}

	_tsystem(_T("PAUSE"));
	return 0;
}

DWORD WINAPI MyThreadFunction( LPVOID lpParam ) 
{//线程函数
	GRS_USEPRINTF();
	PMYDATA pDataArray = (PMYDATA)lpParam;
	GRS_PRINTF(_T("Parameters = %d, %d\n"),pDataArray->val1, pDataArray->val2); 
	return 0; 
} 

void ErrorHandler(LPTSTR lpszFunction) 
{ 
	GRS_USEPRINTF();
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError(); 
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );
	GRS_PRINTF(_T("%s failed with error %d: %s"),lpszFunction, dw, lpMsgBuf); 
	LocalFree(lpMsgBuf);
}

