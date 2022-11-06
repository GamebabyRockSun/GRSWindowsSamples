#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {};
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

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

int _tmain()
{
    GRS_USEPRINTF();
    //��־����"Application"
    TCHAR *logName = _T("Application");
    //��־Դ����
    TCHAR *sourceName = _T("SampleEventSourceName");
    //��Ϣ�ļ�(�����ΪEXE,DLL��)��·��
    TCHAR dllName[MAX_PATH] = {};
    GetAppPath(dllName);
    StringCchCat(dllName,MAX_PATH,_T("ServiceErr.dll"));

    //������־���������,����ֻ����һ��
    DWORD dwCategoryNum = 1;

    HKEY hk; 
    DWORD dwData, dwDisp; 
    TCHAR szBuf[MAX_PATH] = {}; 
    size_t cchSize = MAX_PATH;

    // Create the event source as a subkey of the log. 
    HRESULT hr = StringCchPrintf(szBuf, cchSize, 
        L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\%s\\%s",
        logName, sourceName); 

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szBuf, 
        0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hk, &dwDisp)) 
    {
        GRS_PRINTF(_T("���ܴ���ע����[%s],������:%d\n"),szBuf,GetLastError()); 
        _tsystem(_T("PAUSE"));
        return 0;
    }

    //������Ϣ�ļ���ֵ
    if (RegSetValueEx(hk,         
        _T("EventMessageFile"),   
        0,                        
        REG_EXPAND_SZ,            
        (LPBYTE) dllName,         
        (DWORD) (lstrlen(dllName)+1)*sizeof(TCHAR)))
    {
        GRS_PRINTF(_T("����������Ϣ�ļ���ֵ,������:%d\n"),GetLastError()); 
        RegCloseKey(hk); 
        _tsystem(_T("PAUSE"));
        return 0;
    }

    //����֧�ֵ��¼�����(֧�ִ���/����/��Ϣ3������)
    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
        EVENTLOG_INFORMATION_TYPE; 

    if (RegSetValueEx(hk, 
        _T("TypesSupported"),
        0,                
        REG_DWORD,        
        (LPBYTE) &dwData, 
        sizeof(DWORD)))   
    {
        GRS_PRINTF(_T("��������֧�ֵ���־������,������:%d\n"),GetLastError()); 
        RegCloseKey(hk); 
        _tsystem(_T("PAUSE"));
        return 0;
    }

    //���÷����ļ��ͷ�����Ŀ
    if (RegSetValueEx(hk,           
        L"CategoryMessageFile",     
        0,                         
        REG_EXPAND_SZ,             
        (LPBYTE) dllName,          
        (DWORD) (lstrlen(dllName)+1)*sizeof(TCHAR)))
    {
        GRS_PRINTF(_T("�������÷�����Ϣ�ļ�,������:%d\n"),GetLastError()); 
        RegCloseKey(hk); 
        _tsystem(_T("PAUSE"));
        return 0;
    }

    if (RegSetValueEx(hk,        
        L"CategoryCount",        
        0,                       
        REG_DWORD,               
        (LPBYTE) &dwCategoryNum,  
        sizeof(DWORD)))          
    {
        GRS_PRINTF(_T("�������÷�������,������:%d\n"),GetLastError()); 
        RegCloseKey(hk); 
        _tsystem(_T("PAUSE"));
        return 0;
    }
    RegCloseKey(hk); 
    GRS_PRINTF(_T("��־Դ[%s]��ӳɹ�\n"),sourceName); 
    _tsystem(_T("PAUSE"));
    return 1;

}