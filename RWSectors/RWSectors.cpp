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
		GRS_PRINTF(_T("GetDiskFreeSpace(C:\\)调用失败,错误码:0x%08x\n")
			,GetLastError());
		dwBytesPerSector = 512;	//为扇区字节数设置一个全世界都流行的值
	}

	//打开第一个物理硬盘,索引从0开始,注意要使用FILE_SHARE_WRITE共享方式
	//使用OPEN_EXISTING创建标志,因为硬盘肯定是已经存在的了
	//使用只读属性打开,防止发生意外,这里读取了从0扇区开始的连续8个扇区的数据
	//可以通过断点的方式查看其中内容
	//写入操作很危险,在没有搞清楚磁盘扇区等原理之前不要盲目做写扇区实验,以免硬盘数据丢失
	//在Vista 2008 Win7等系统上对这种方式有些限制详情请阅读MSDN中CreateFile说明中的Remark段
	HANDLE hDisk = CreateFile(pDiskName,GENERIC_READ/*|GENERIC_WRITE*/,FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if(INVALID_HANDLE_VALUE == hDisk)
	{
		GRS_PRINTF(_T("CreateFile(%s)调用失败,错误码:0x%08x\n")
			,pDiskName,GetLastError());
		_tsystem(_T("PAUSE"));
		return -1;
	}

	//读取8个扇区也就是4k的页面
	BYTE* pBuf = (BYTE*)GRS_CALLOC(8 * dwBytesPerSector);
	DWORD dwRead = 0;
	if(!ReadFile(hDisk,pBuf,8*dwBytesPerSector,&dwRead,NULL))
	{
		GRS_PRINTF(_T("ReadFile(%s)调用失败,错误码:0x%08x\n")
			,pDiskName,GetLastError());
		_tsystem(_T("PAUSE"));
	}
	CloseHandle(hDisk);
	_tsystem(_T("PAUSE"));
	return 0;
}