#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define BUF_SIZE 256
#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

TCHAR szName[]= _T("Global\\MyFileMappingObject");
TCHAR szMsg[] = _T("���� ShareMemoryA ����д��Ĺ�������");

int _tmain()
{
	GRS_USEPRINTF();

	HANDLE hMapFile = NULL;
	LPCTSTR pBuf = NULL;

	//ʹ��INVALID_HANDLE_VALUE��Ϊ�ļ�������������ļ�ӳ�����
	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,
		0,BUF_SIZE,szName);

	if (hMapFile == NULL) 
	{ 
		GRS_PRINTF(_T("CreateFileMapping����ʧ��,������:0x%08x\n"),GetLastError());
		return 1;
	}

	//���������ӿ�
	pBuf = (LPTSTR)MapViewOfFile(hMapFile,FILE_MAP_ALL_ACCESS,0,0,BUF_SIZE);           

	if ( pBuf == NULL ) 
	{ 
		GRS_PRINTF(_T("MapViewOfFile����ʧ��,������:0x%08x\n"),GetLastError()); 
		CloseHandle(hMapFile);
		return 2;
	}

	//д������
	CopyMemory((PVOID)pBuf, szMsg, sizeof(szMsg));

	_tsystem(_T("PAUSE"));

	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	return 0;
}
