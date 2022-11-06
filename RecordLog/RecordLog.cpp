#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#include "../AddSource/ServiceErr.h"

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {};
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

void __cdecl wmain(int argc, LPWSTR *argv)
{
    GRS_USEPRINTF();
    //ע�����Դ���ƺ�AddSource�е���ͬ
    wchar_t *sourceName = L"SampleEventSourceName";
    //���ID����ServiceErr.h
    DWORD dwEventID = SVC_ERROR;                    
    WORD cInserts = 1;                              
    LPCWSTR szMsg = L"insertString";                

    HANDLE h; 

    // ע���¼�Դ
    h = RegisterEventSource(NULL,   
        sourceName);   
    if (h == NULL) 
    {
        GRS_PRINTF(_T("����ע����־�¼�Դ,������:%d\n"),GetLastError()); 
        _tsystem(_T("PAUSE"));
        return;
    }

    // ��¼�¼���־

    if (!ReportEvent(h,       // Event log handle. 
        EVENTLOG_ERROR_TYPE,  // Event type. 
        NULL,                 // Event category.  
        dwEventID,            // Event identifier. 
        NULL,                 // No user security identifier. 
        cInserts,             // Number of substitution strings. 
        0,                    // No data. 
        &szMsg,               // Pointer to strings. 
        NULL))                // No data. 
    {
        GRS_PRINTF(_T("�޷���¼��־,������:%d\n"),GetLastError()); 
    }

    DeregisterEventSource(h); 
    _tsystem(_T("PAUSE"));
    return;
}
