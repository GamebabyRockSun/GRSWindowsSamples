#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define BUF_SIZE 256
#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

TCHAR szName[]=TEXT("Global\\MyFileMappingObject");

int _tmain()
{
	GRS_USEPRINTF();
	HANDLE hMapFile = NULL;
	LPCTSTR pBuf = NULL;

	//打开与SharedMemoryA同名的内存映射文件对象
	hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,szName);
	if (hMapFile == NULL) 
	{ 
		GRS_PRINTF(_T("OpenFileMapping调用失败,错误码:0x%08x\n"),GetLastError());
		return 1;
	} 
	//创建数据视口
	pBuf = (LPTSTR) MapViewOfFile(hMapFile,FILE_MAP_ALL_ACCESS,0,0,BUF_SIZE);                   
	if (pBuf == NULL) 
	{ 
		GRS_PRINTF(_T("MapViewOfFile调用失败,错误码:0x%08x\n"),GetLastError());
		CloseHandle(hMapFile);
		return 2;
	}

	GRS_PRINTF(_T("SharedMemoryB 进程读取到共享内存中的信息为:%s\n"),pBuf);
	_tsystem(_T("PAUSE"));

	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	return 0;
}
