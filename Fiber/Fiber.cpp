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

VOID __stdcall ReadFiberFunc(LPVOID lpParameter);
VOID __stdcall WriteFiberFunc(LPVOID lpParameter);
void DisplayFiberInfo(void);

typedef struct
{
    DWORD dwParameter;          // DWORD parameter to fiber (unused)
    DWORD dwFiberResultCode;    // GetLastError() result code
    HANDLE hFile;               // handle to operate on
    DWORD dwBytesProcessed;     // number of bytes processed
} FIBERDATASTRUCT, *PFIBERDATASTRUCT, *LPFIBERDATASTRUCT;

#define RTN_OK 0
#define RTN_USAGE 1
#define RTN_ERROR 13

#define BUFFER_SIZE 32768   // read/write buffer size
#define FIBER_COUNT 3       // max fibers (including primary)

#define PRIMARY_FIBER 0 // array index to primary fiber
#define READ_FIBER 1    // array index to read fiber
#define WRITE_FIBER 2   // array index to write fiber

LPVOID g_lpFiber[FIBER_COUNT];
LPBYTE g_lpBuffer;
DWORD g_dwBytesRead;

__inline VOID GetAppPath(LPTSTR pszBuffer)
{
    DWORD dwLen = 0;
    if(0 == (dwLen = ::GetModuleFileName(NULL,pszBuffer,MAX_PATH)))
    {
        return;			
    }
    DWORD i = dwLen;
    for(; i > 0; i -- )
    {
        if( '\\' == pszBuffer[i] )
        {
            pszBuffer[i + 1] = '\0';
            break;
        }
    }
}

int __cdecl _tmain()
{
    GRS_USEPRINTF();
    LPFIBERDATASTRUCT fs = NULL;

    TCHAR pSrcFile[MAX_PATH] = {};
    TCHAR pDstFile[MAX_PATH] = {};

    GetAppPath(pSrcFile);
    GetAppPath(pDstFile);
    StringCchCat(pSrcFile,MAX_PATH,_T("2.jpg"));
    StringCchCat(pDstFile,MAX_PATH,_T("2_Cpy.jpg"));

    fs = (LPFIBERDATASTRUCT)GRS_CALLOC(sizeof(FIBERDATASTRUCT) * FIBER_COUNT);
    if (fs == NULL)
    {
        GRS_PRINTF(_T("HeapAlloc error (%d)\n"), GetLastError());
        return RTN_ERROR;
    }

    g_lpBuffer = (LPBYTE)GRS_CALLOC(BUFFER_SIZE);
    
    if (g_lpBuffer == NULL)
    {
        GRS_PRINTF(_T("HeapAlloc error (%d)\n"), GetLastError());
        return RTN_ERROR;
    }
    fs[READ_FIBER].hFile = CreateFile(pSrcFile,
        GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);

    if ( INVALID_HANDLE_VALUE == fs[READ_FIBER].hFile )
    {
        GRS_PRINTF(_T("CreateFile error (%d)\n"), GetLastError());
        return RTN_ERROR;
    }

    fs[WRITE_FIBER].hFile = CreateFile(pDstFile,
        GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,NULL);

    if (INVALID_HANDLE_VALUE == fs[WRITE_FIBER].hFile )
    {
        GRS_PRINTF(_T("CreateFile error (%d)\n"), GetLastError());
        return RTN_ERROR;
    }

    //线程变成纤程
    g_lpFiber[PRIMARY_FIBER] = ConvertThreadToFiber(&fs[PRIMARY_FIBER]);

    if (g_lpFiber[PRIMARY_FIBER] == NULL)
    {
        GRS_PRINTF(_T("ConvertThreadToFiber error (%d)\n"), GetLastError());
        return RTN_ERROR;
    }
    fs[PRIMARY_FIBER].dwParameter = 0;
    fs[PRIMARY_FIBER].dwFiberResultCode = 0;
    fs[PRIMARY_FIBER].hFile = INVALID_HANDLE_VALUE;

    g_lpFiber[READ_FIBER] = CreateFiber(0,ReadFiberFunc,&fs[READ_FIBER]);

    if (g_lpFiber[READ_FIBER] == NULL)
    {
        GRS_PRINTF(_T("CreateFiber error (%d)\n"), GetLastError());
        return RTN_ERROR;
    }

    fs[READ_FIBER].dwParameter = 0x12345678;
    g_lpFiber[WRITE_FIBER]=CreateFiber(0,WriteFiberFunc,&fs[WRITE_FIBER]);

    if (g_lpFiber[WRITE_FIBER] == NULL)
    {
        GRS_PRINTF(_T("CreateFiber error (%d)\n"), GetLastError());
        return RTN_ERROR;
    }
    fs[WRITE_FIBER].dwParameter = 0x54545454;
    SwitchToFiber(g_lpFiber[READ_FIBER]);

    GRS_PRINTF(_T("ReadFiber: result code is %lu, %lu bytes processed\n"),
        fs[READ_FIBER].dwFiberResultCode, fs[READ_FIBER].dwBytesProcessed);

    GRS_PRINTF(_T("WriteFiber: result code is %lu, %lu bytes processed\n"),
        fs[WRITE_FIBER].dwFiberResultCode, fs[WRITE_FIBER].dwBytesProcessed);

    DeleteFiber(g_lpFiber[READ_FIBER]);
    DeleteFiber(g_lpFiber[WRITE_FIBER]);

    CloseHandle(fs[READ_FIBER].hFile);
    CloseHandle(fs[WRITE_FIBER].hFile);

    GRS_SAFEFREE(g_lpBuffer);
    GRS_SAFEFREE(fs);

    //纤程变回线程
    ConvertFiberToThread();

    _tsystem(_T("PAUSE"));
    return RTN_OK;
}

VOID __stdcall ReadFiberFunc(LPVOID lpParameter)
{
    GRS_USEPRINTF();
    LPFIBERDATASTRUCT fds = (LPFIBERDATASTRUCT)lpParameter;
    if (fds == NULL)
    {
        GRS_PRINTF(_T("Passed NULL fiber data; exiting current thread.\n"));
        return;
    }
   
    fds->dwBytesProcessed = 0;
    while (1)
    {
        DisplayFiberInfo();
        if (!ReadFile(fds->hFile, g_lpBuffer, BUFFER_SIZE,&g_dwBytesRead, NULL))
        {
            break;
        }
        if (g_dwBytesRead == 0)
        {
            break;
        }
        fds->dwBytesProcessed += g_dwBytesRead;
        SwitchToFiber(g_lpFiber[WRITE_FIBER]);
    } // while
    fds->dwFiberResultCode = GetLastError();
    SwitchToFiber(g_lpFiber[PRIMARY_FIBER]);
}

VOID __stdcall WriteFiberFunc(LPVOID lpParameter)
{
    GRS_USEPRINTF();
    LPFIBERDATASTRUCT fds = (LPFIBERDATASTRUCT)lpParameter;
    DWORD dwBytesWritten;
    if (fds == NULL)
    {
        GRS_PRINTF(_T("Passed NULL fiber data; exiting current thread.\n"));
        return;
    }
    

    fds->dwBytesProcessed = 0;
    fds->dwFiberResultCode = ERROR_SUCCESS;

    while (1)
    {
        DisplayFiberInfo();
        if (!WriteFile(fds->hFile, g_lpBuffer, g_dwBytesRead,&dwBytesWritten, NULL))
        {
            break;
        }
        fds->dwBytesProcessed += dwBytesWritten;
        SwitchToFiber(g_lpFiber[READ_FIBER]);
    }  // while
    fds->dwFiberResultCode = GetLastError();
    SwitchToFiber(g_lpFiber[PRIMARY_FIBER]);
}

void DisplayFiberInfo(void)
{
    GRS_USEPRINTF();
    LPFIBERDATASTRUCT fds = (LPFIBERDATASTRUCT)GetFiberData();
    LPVOID lpCurrentFiber = GetCurrentFiber();

    if (lpCurrentFiber == g_lpFiber[READ_FIBER])
    {
        GRS_PRINTF(_T("Read fiber entered"));
    }
    else
    {
        if (lpCurrentFiber == g_lpFiber[WRITE_FIBER])
        {
            GRS_PRINTF(_T("Write fiber entered"));
        }
        else
        {
            if (lpCurrentFiber == g_lpFiber[PRIMARY_FIBER])
            {
                GRS_PRINTF(_T("Primary fiber entered"));
            }
            else
            {
                GRS_PRINTF(_T("Unknown fiber entered"));
            }
        }
    }
    GRS_PRINTF(_T(" (dwParameter is 0x%lx)\n"), fds->dwParameter);
}

