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

__inline BOOL FileExist(LPCTSTR pFileName)
{
	return !(INVALID_FILE_ATTRIBUTES == GetFileAttributes(pFileName) && ERROR_FILE_NOT_FOUND == GetLastError());
}

int _tmain()
{
	GRS_USEPRINTF();	
	TCHAR pFileName[] = _T("C:\\test.dat");//注意C字符串中两个反斜杠表示一个反斜杠的转义
	//创建文件
	HANDLE hFile = CreateFile(pFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(INVALID_HANDLE_VALUE == hFile)
	{
		GRS_PRINTF(_T("创建文件(%s)失败,错误码:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
		return -1;
	}
	DWORD dwWrite = 0; 
	if(!WriteFile(hFile,pFileName,sizeof(pFileName),&dwWrite,NULL))
	{
		GRS_PRINTF(_T("写入文件(%s)失败,错误码:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
	}
	CloseHandle(hFile);

	GRS_PRINTF(_T("创建文件(%s)并写入内容成功\n"),pFileName);
	_tsystem(_T("PAUSE"));


	//打开已存在的文件
	hFile = CreateFile(pFileName,GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(INVALID_HANDLE_VALUE == hFile)
	{
		GRS_PRINTF(_T("打开文件(%s)失败,错误码:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
		return -1;
	}
	dwWrite = 0; 
	if(!WriteFile(hFile,pFileName,sizeof(pFileName),&dwWrite,NULL))
	{
		GRS_PRINTF(_T("写入文件(%s)失败,错误码:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
	}
	CloseHandle(hFile);

	GRS_PRINTF(_T("打开文件(%s)并写入内容成功\n"),pFileName);
	_tsystem(_T("PAUSE"));

	//打开已存在的文件,并将指针移动到末尾,追加内容到文件中
	hFile = CreateFile(pFileName,GENERIC_WRITE|GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(INVALID_HANDLE_VALUE == hFile)
	{
		GRS_PRINTF(_T("打开文件(%s)失败,错误码:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
		return -1;
	}

	//移动文件指针到末尾
	SetFilePointer(hFile,0,0,FILE_END);
	dwWrite = 0; 
	if(!WriteFile(hFile,pFileName,sizeof(pFileName),&dwWrite,NULL))
	{
		GRS_PRINTF(_T("写入文件(%s)失败,错误码:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
	}
	CloseHandle(hFile);

	GRS_PRINTF(_T("打开文件(%s)并追加内容成功\n"),pFileName);
	_tsystem(_T("PAUSE"));

	//删除文件
	DeleteFile(pFileName);

	//判定文件是否存在
	if( FileExist(pFileName) )
	{
		GRS_PRINTF(_T("文件(%s)存在\n"),pFileName);
	}
	else
	{
		GRS_PRINTF(_T("文件(%s)不存在\n"),pFileName);
	}

	_tsystem(_T("PAUSE"));
	return 0;
}