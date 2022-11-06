#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define BUF_SIZE 256
#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

TCHAR szName[]= _T("Global\\MyFileMappingObject");
TCHAR szMsg[] = _T("这是 ShareMemoryA 进程写入的共享数据");

int _tmain()
{
	GRS_USEPRINTF();

	HANDLE hMapFile = NULL;
	LPCTSTR pBuf = NULL;

	//使用INVALID_HANDLE_VALUE作为文件句柄参数创建文件映射对象
	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,
		0,BUF_SIZE,szName);

	if (hMapFile == NULL) 
	{ 
		GRS_PRINTF(_T("CreateFileMapping调用失败,错误码:0x%08x\n"),GetLastError());
		return 1;
	}

	//创建数据视口
	pBuf = (LPTSTR)MapViewOfFile(hMapFile,FILE_MAP_ALL_ACCESS,0,0,BUF_SIZE);           

	if ( pBuf == NULL ) 
	{ 
		GRS_PRINTF(_T("MapViewOfFile调用失败,错误码:0x%08x\n"),GetLastError()); 
		CloseHandle(hMapFile);
		return 2;
	}

	//写入数据
	CopyMemory((PVOID)pBuf, szMsg, sizeof(szMsg));

	_tsystem(_T("PAUSE"));

	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	return 0;
}
