#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#import "C:\Program Files\Common Files\System\ado\msado15.dll" \
    no_namespace rename("EOF", "EndOfFile")

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_COM_CHECK(hr,...) if(FAILED(hr)){GRS_PRINTF(__VA_ARGS__);goto CLEAN_UP;}

int _tmain()
{   
    GRS_USEPRINTF();
    CoInitialize(NULL);
    HRESULT hr = S_OK;
    ULONG   ulRow = 0;
    _bstr_t bsCnct(_T("Provider=sqloledb;Data Source=ASUS-PC\\SQL2008;\
                      Initial Catalog=study;User Id=sa;Password=999999;"));

    _ConnectionPtr adocnct;
    _RecordsetPtr  Rowset;
    _RecordsetPtr Rowset2(__uuidof(Recordset));
   
    hr = adocnct.CreateInstance(_T("ADODB.Connection"));
    GRS_COM_CHECK(hr,_T("创建Connection对象失败,错误码:0x%08X\n"),hr);

    hr = adocnct->Open(bsCnct,_T("sa"),_T("999999"),adModeUnknown);
    GRS_COM_CHECK(hr,_T("链接到数据源失败,错误码:0x%08X\n"),hr);

    if(adocnct->State)
    {
        GRS_PRINTF(_T("连接到数据源成功!\n"));
    }
    else
    {
        GRS_PRINTF(_T("连接到数据源失败!\n"));
        goto CLEAN_UP;
    }

    hr = Rowset.CreateInstance(__uuidof(Recordset));
    GRS_COM_CHECK(hr,_T("创建Recordset对象失败,错误码:0x%08X\n"),hr);

    hr = Rowset->Open(_T("SELECT top 200 * FROM T_State")
        ,adocnct.GetInterfacePtr(),adOpenStatic,adLockOptimistic,adCmdText);
    GRS_COM_CHECK(hr,_T("查询数据失败,错误码:0x%08X\n"),hr);
    
    while(!Rowset->EndOfFile)
    {
        GRS_PRINTF(_T("|%10u|%10s|%-40s|\n")
            ,++ulRow
            ,Rowset->Fields->GetItem("K_StateCode")->Value.bstrVal
            ,Rowset->Fields->GetItem("F_StateName")->Value.bstrVal);
       
        Rowset->MoveNext();
    }

    hr = Rowset2->Open(_T("SELECT top 100 * FROM T_State")
        ,bsCnct,adOpenStatic,adLockOptimistic,adCmdText);
    GRS_COM_CHECK(hr,_T("查询数据失败,错误码:0x%08X\n"),hr);

    GRS_PRINTF(_T("\n输出第二个结果集:\n"));
    ulRow = 0;
    while(!Rowset2->EndOfFile)
    {
        GRS_PRINTF(_T("|%10u|%10s|%-40s|\n")
            ,++ulRow
            ,Rowset2->Fields->GetItem("K_StateCode")->Value.bstrVal
            ,Rowset2->Fields->GetItem("F_StateName")->Value.bstrVal);
       
        Rowset2->MoveNext();
    }

    
CLEAN_UP:
    _tsystem(_T("PAUSE"));
    CoUninitialize();
    return 0;
}