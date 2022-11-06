#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include "shlwapi.h"
#pragma comment (lib,"shlwapi.lib")
#include "winsvc.h"
#pragma comment (lib,"Advapi32.lib")

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {};DWORD dwRead = 0;
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_SCANF() ReadConsole(GetStdHandle(STD_INPUT_HANDLE),pBuf,1,&dwRead,NULL);

#define GRS_BUFLEN 1024

int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
    SC_HANDLE scHandle = OpenSCManager(NULL,NULL,SC_MANAGER_ENUMERATE_SERVICE);
    if(scHandle ==NULL)
    {
        return 0;
    }

    ENUM_SERVICE_STATUS_PROCESS* pessp = NULL;
    DWORD dwBytesRequired;
    DWORD dwTotalServices;
    DWORD ResumeHandle = 0 ;
    HANDLE scHOutPut;

    AllocConsole();
    scHOutPut = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bRet = EnumServicesStatusEx(scHandle,SC_ENUM_PROCESS_INFO ,
        SERVICE_WIN32,SERVICE_STATE_ALL,(LPBYTE)pessp,0,&dwBytesRequired,
        &dwTotalServices,&ResumeHandle,NULL);
    if( 0 != bRet )
    {
        FreeConsole();
        return 0;
    }
    DWORD dwBufferSize = dwBytesRequired;
    DWORD dwError = GetLastError();
    if(ERROR_MORE_DATA != dwError)
    {
        FreeConsole();
        return 0;
    }
    
    pessp = (ENUM_SERVICE_STATUS_PROCESS*)GRS_CALLOC(dwBytesRequired);
    bRet = EnumServicesStatusEx(scHandle,SC_ENUM_PROCESS_INFO ,\
        SERVICE_WIN32,SERVICE_STATE_ALL,(LPBYTE)pessp,dwBufferSize,&dwBytesRequired,\
        &dwTotalServices,&ResumeHandle,NULL);

    if(bRet == 0)
    {
        return 0;
    }

    TCHAR* szOutput = (TCHAR*)GRS_CALLOC(GRS_BUFLEN * sizeof(TCHAR));
    size_t szOutLen = 0;
    for(DWORD i = 0 ; i < dwTotalServices ; i++)
    {
        LPBYTE pbDesc = NULL;
        DWORD dwBytesNeeded = 0;

        SC_HANDLE hService = OpenService(scHandle,pessp[i].lpServiceName,SERVICE_QUERY_CONFIG);
        if(NULL == hService)
        {
            continue ;
        }

        QueryServiceConfig2(hService,SERVICE_CONFIG_DESCRIPTION,pbDesc,0,&dwBytesNeeded);
        DWORD dwError = GetLastError();

        if(dwError != ERROR_INSUFFICIENT_BUFFER)
        {
            continue;
        }

        DWORD dwDescSize = dwBytesNeeded;
        pbDesc = (LPBYTE)GRS_CALLOC(dwBytesNeeded);
        if(0 == QueryServiceConfig2(hService,SERVICE_CONFIG_DESCRIPTION,pbDesc,dwDescSize,&dwBytesNeeded))
        {
            GRS_SAFEFREE(pbDesc);
            continue;
        }

        ZeroMemory(szOutput,GRS_BUFLEN * sizeof(TCHAR));

        StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),pessp[i].lpServiceName);
        StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),_T("\n\t"));
        StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),pessp[i].lpDisplayName);
        StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),_T("\n\t"));
        if(NULL != ((SERVICE_DESCRIPTION*)pbDesc)->lpDescription )
        {
            StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),((SERVICE_DESCRIPTION*)pbDesc)->lpDescription);
        }
        switch(pessp[i].ServiceStatusProcess.dwCurrentState)
        {
        case SERVICE_CONTINUE_PENDING:
            {
                StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),_T("\n\t正在重启动......"));
            }
            break;
        case SERVICE_PAUSE_PENDING:
            {
                StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),_T("\n\t正在暂停......"));
            }
            break;
        case SERVICE_PAUSED:
            {
                StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),_T("\n\t暂停"));
            }
            break;
        case SERVICE_RUNNING:
            {
                StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),_T("\n\t运行"));
            }
            break;
        case SERVICE_START_PENDING:
            {
                StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),_T("\n\t正在启动......"));
            }
            break;
        case SERVICE_STOP_PENDING:
            {
                StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),_T("\n\t正在停止......"));
            }
            break;
        case SERVICE_STOPPED:
            {
                StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),_T("\n\t停止"));
            }
        }
        StringCchCat(szOutput,GRS_BUFLEN * sizeof(TCHAR),_T("\n"));

        StringCchLength(szOutput,GRS_BUFLEN * sizeof(TCHAR),&szOutLen);
        WriteConsole(scHOutPut,szOutput,szOutLen,NULL,0);

        GRS_SAFEFREE(pbDesc);
        CloseServiceHandle(hService);
    }

    CloseServiceHandle(scHandle);
    GRS_SAFEFREE(szOutput);
    GRS_SAFEFREE(pessp);

    _tsystem(_T("PAUSE"));

    FreeConsole();
    return 0;
}