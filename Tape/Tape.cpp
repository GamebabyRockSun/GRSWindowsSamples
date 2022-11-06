#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_ASSERT(s) if(!(s)) {::DebugBreak();}


int _tmain()
{
    HANDLE hTape = CreateFile(_T("\\\\.\\TAPE0")
        ,GENERIC_WRITE|GENERIC_READ,0,0,OPEN_EXISTING,0,NULL);

    //ȡ�÷������� ���С����Ϣ
    DWORD dwBufSz = sizeof(TAPE_GET_MEDIA_PARAMETERS);
    TAPE_GET_MEDIA_PARAMETERS tapeparam = {};
    GetTapeParameters(hTape,GET_TAPE_MEDIA_INFORMATION,
        &dwBufSz,&tapeparam);

    //��ͷ�ƶ�����һ�������Ŀ�ʼλ��
    SetTapePosition(hTape,TAPE_LOGICAL_BLOCK,1,0,0,TRUE);
    
    //׼��������Ļ�����
    BYTE* pData = (BYTE*)GRS_CALLOC(2 * tapeparam.BlockSize);
    
    //��ȡ�������ݿ�
    DWORD dwRead = 0;
    ReadFile(hTape,pData,2 * tapeparam.BlockSize,&dwRead,NULL);

    CloseHandle(hTape);
    return 0;
}