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
	TCHAR pFileName[] = _T("C:\\test.dat");//ע��C�ַ�����������б�ܱ�ʾһ����б�ܵ�ת��
	//�����ļ�
	HANDLE hFile = CreateFile(pFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(INVALID_HANDLE_VALUE == hFile)
	{
		GRS_PRINTF(_T("�����ļ�(%s)ʧ��,������:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
		return -1;
	}
	DWORD dwWrite = 0; 
	if(!WriteFile(hFile,pFileName,sizeof(pFileName),&dwWrite,NULL))
	{
		GRS_PRINTF(_T("д���ļ�(%s)ʧ��,������:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
	}
	CloseHandle(hFile);

	GRS_PRINTF(_T("�����ļ�(%s)��д�����ݳɹ�\n"),pFileName);
	_tsystem(_T("PAUSE"));


	//���Ѵ��ڵ��ļ�
	hFile = CreateFile(pFileName,GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(INVALID_HANDLE_VALUE == hFile)
	{
		GRS_PRINTF(_T("���ļ�(%s)ʧ��,������:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
		return -1;
	}
	dwWrite = 0; 
	if(!WriteFile(hFile,pFileName,sizeof(pFileName),&dwWrite,NULL))
	{
		GRS_PRINTF(_T("д���ļ�(%s)ʧ��,������:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
	}
	CloseHandle(hFile);

	GRS_PRINTF(_T("���ļ�(%s)��д�����ݳɹ�\n"),pFileName);
	_tsystem(_T("PAUSE"));

	//���Ѵ��ڵ��ļ�,����ָ���ƶ���ĩβ,׷�����ݵ��ļ���
	hFile = CreateFile(pFileName,GENERIC_WRITE|GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(INVALID_HANDLE_VALUE == hFile)
	{
		GRS_PRINTF(_T("���ļ�(%s)ʧ��,������:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
		return -1;
	}

	//�ƶ��ļ�ָ�뵽ĩβ
	SetFilePointer(hFile,0,0,FILE_END);
	dwWrite = 0; 
	if(!WriteFile(hFile,pFileName,sizeof(pFileName),&dwWrite,NULL))
	{
		GRS_PRINTF(_T("д���ļ�(%s)ʧ��,������:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
	}
	CloseHandle(hFile);

	GRS_PRINTF(_T("���ļ�(%s)��׷�����ݳɹ�\n"),pFileName);
	_tsystem(_T("PAUSE"));

	//ɾ���ļ�
	DeleteFile(pFileName);

	//�ж��ļ��Ƿ����
	if( FileExist(pFileName) )
	{
		GRS_PRINTF(_T("�ļ�(%s)����\n"),pFileName);
	}
	else
	{
		GRS_PRINTF(_T("�ļ�(%s)������\n"),pFileName);
	}

	_tsystem(_T("PAUSE"));
	return 0;
}