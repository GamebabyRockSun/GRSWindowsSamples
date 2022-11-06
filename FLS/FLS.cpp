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

VOID __stdcall FiberFunc(LPVOID lpParameter);

#define FIBER_COUNT 3
#define PRIMARY_FIBER 0 // array index to primary fiber

LPVOID g_lpFiber[FIBER_COUNT] = {};
DWORD g_dwFlsIndex = 0;

int __cdecl _tmain()
{
    GRS_USEPRINTF();
    //主线程变成纤程
    g_lpFiber[PRIMARY_FIBER] = ConvertThreadToFiber(NULL);
    if (g_lpFiber[PRIMARY_FIBER] == NULL)
    {
        GRS_PRINTF(_T("ConvertThreadToFiber error (%d)\n"), GetLastError());
        return -1;
    }

    g_dwFlsIndex = FlsAlloc(NULL);
    if(FLS_OUT_OF_INDEXES == g_dwFlsIndex)
    {
        GRS_PRINTF(_T("FlsAlloc error (%d)\n"), GetLastError());
        return -1;
    }
  
    for(int i = 1;i < FIBER_COUNT;i++ )
    {
        g_lpFiber[i] = CreateFiber(0,FiberFunc,NULL);
        if (g_lpFiber[i] == NULL)
        {
            GRS_PRINTF(_T("CreateFiber error (%d)\n"), GetLastError());
            return -1;
        }
    }

    for(int i = 1;i < FIBER_COUNT;i++ )
    {//轮流调度
        SwitchToFiber(g_lpFiber[i]);
    }

    for(int i = 1;i < FIBER_COUNT;i++ )
    {
        DeleteFiber(g_lpFiber[i]);
    }

    FlsFree(g_dwFlsIndex);
    //纤程变回线程
    ConvertFiberToThread();

    _tsystem(_T("PAUSE"));
    return 0;
}

VOID __stdcall FiberFunc(LPVOID lpParameter)
{
    GRS_USEPRINTF();
    FlsSetValue(g_dwFlsIndex,GetCurrentFiber());
    GRS_PRINTF(_T("Fiber[0x%x] Save Fls Value(%u)\n")
        ,GetCurrentFiber(),FlsGetValue(g_dwFlsIndex));
    SwitchToFiber(g_lpFiber[PRIMARY_FIBER]);
}

