#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};
#define GRS_PRINTF(...) \
    StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

#define GRS_BEGINTHREAD(Fun,Param) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Fun,Param,0,NULL)


DWORD WINAPI ThreadProc(LPVOID lpParameter);

int _tmain()
{
    GRS_USEPRINTF();

    int iWaitLen = 0;
    do
    {
        GRS_PRINTF(_T("\n������һ���ȴ�ʱ��ֵ,��λ��(����0�˳�):"))
        _tscanf(_T("%i"),&iWaitLen);
        if(iWaitLen > 0)
        {
            //����Ĵ�����ʾ��,ʹ��CreateThread��QueueUserWorkItemʵ��Ч��һ��
            //��Ȼ�̲߳������������,����̺߳ܶ�ʱһ��Ҫʹ��QueueUserWorkItem
            //GRS_BEGINTHREAD(ThreadProc,(PVOID)iWaitLen);
            QueueUserWorkItem(ThreadProc,(PVOID)iWaitLen,WT_EXECUTELONGFUNCTION);
        }
    }
    while(iWaitLen);

    return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{//�̺߳���,������CreateThread�������߳�,Ҳ����ʹ��QueueUserWorkItem����Ϊ�̳߳���ʽ
    int iWaitLen = (int)lpParameter;
    
    GRS_USEPRINTF();
    GRS_PRINTF(_T("\n�߳�[ID:0x%x]���ȴ�%u��......"),GetCurrentThreadId(),lpParameter);
    Sleep((DWORD)lpParameter * 1000);
    GRS_PRINTF(_T("\n�߳�[ID:0x%x]�ȴ�����\n"),GetCurrentThreadId(),lpParameter);
    return 0;
}