#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);


#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}


int _tmain()
{
	GRS_USEPRINTF();

	TCHAR pDiskName[] = _T("\\\\.\\PhysicalDrive0");
	DWORD dwSectorsPerCluster		= 0;
	DWORD dwBytesPerSector			= 0;
	DWORD dwNumberOfFreeClusters	= 0;
	DWORD dwTotalNumberOfClusters	= 0;

	if(!GetDiskFreeSpace(_T("C:\\"),
		&dwSectorsPerCluster,&dwBytesPerSector,&dwNumberOfFreeClusters,&dwTotalNumberOfClusters))
	{
		GRS_PRINTF(_T("GetDiskFreeSpace(C:\\)����ʧ��,������:0x%08x\n")
			,GetLastError());
		dwBytesPerSector = 512;	//Ϊ�����ֽ�������һ��ȫ���綼���е�ֵ
	}

	//�򿪵�һ������Ӳ��,������0��ʼ,ע��Ҫʹ��FILE_SHARE_WRITE����ʽ
	//ʹ��OPEN_EXISTING������־,��ΪӲ�̿϶����Ѿ����ڵ���
	//ʹ��ֻ�����Դ�,��ֹ��������,�����ȡ�˴�0������ʼ������8������������
	//����ͨ���ϵ�ķ�ʽ�鿴��������
	//д�������Σ��,��û�и��������������ԭ��֮ǰ��ҪäĿ��д����ʵ��,����Ӳ�����ݶ�ʧ
	//��Vista 2008 Win7��ϵͳ�϶����ַ�ʽ��Щ�����������Ķ�MSDN��CreateFile˵���е�Remark��
	HANDLE hDisk = CreateFile(pDiskName,GENERIC_READ/*|GENERIC_WRITE*/,FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if(INVALID_HANDLE_VALUE == hDisk)
	{
		GRS_PRINTF(_T("CreateFile(%s)����ʧ��,������:0x%08x\n")
			,pDiskName,GetLastError());
		_tsystem(_T("PAUSE"));
		return -1;
	}

	//��ȡ8������Ҳ����4k��ҳ��
	BYTE* pBuf = (BYTE*)GRS_CALLOC(8 * dwBytesPerSector);
	DWORD dwRead = 0;
	if(!ReadFile(hDisk,pBuf,8*dwBytesPerSector,&dwRead,NULL))
	{
		GRS_PRINTF(_T("ReadFile(%s)����ʧ��,������:0x%08x\n")
			,pDiskName,GetLastError());
		_tsystem(_T("PAUSE"));
	}
	CloseHandle(hDisk);
	_tsystem(_T("PAUSE"));
	return 0;
}