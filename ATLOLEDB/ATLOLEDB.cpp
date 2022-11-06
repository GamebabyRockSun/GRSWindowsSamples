#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
//使用ATL的OLEDB使用者模版
#include <atldbcli.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);
//注意这个宏中的AtlTraceErrorRecords调用
#define GRS_COM_CHECK(hr,...) if(FAILED(hr)){GRS_PRINTF(__VA_ARGS__);AtlTraceErrorRecords(hr);goto CLEAR_UP;}

//动态访问器对象组成一个好用的结果集对象 
typedef CCommand<CDynamicAccessor,CRowset,CMultipleResults> CGRSRowSet;
typedef CTable<CDynamicAccessor,CRowset> CGRSTable;
//CDynamicStringAccessor访问器对象将所有的列都转换成字符串型
typedef CTable<CDynamicStringAccessor,CRowset> CGRSTableString;

VOID _tmain()
{
    GRS_USEPRINTF();
    CoInitialize(NULL);

    CDataSource             db;
    CSession                ss;
    CGRSTableString         tb1;
    HRESULT                 hr                          = S_OK;
    TCHAR*					pszTableName				= _T("T_Decimal");
    DBID					TableID						= {};

    hr = db.Open();
    GRS_COM_CHECK(hr,_T("创建数据源对象失败,错误码:0x%08X\n"),hr);
    
    hr = ss.Open(db);
    GRS_COM_CHECK(hr,_T("创建会话对象失败,错误码:0x%08X\n"),hr);
    
    TableID.eKind           = DBKIND_NAME;
	TableID.uName.pwszName  = (LPOLESTR)pszTableName;

    hr = tb1.Open(ss,TableID);
    //也可以写成下面的样子,直接给表名
    //hr = tb1.Open(ss,pszTableName);
    GRS_COM_CHECK(hr,_T("打开表对象失败,错误码:0x%08X\n"),hr);
    hr = tb1.MoveFirst();
    GRS_COM_CHECK(hr,_T("表对象中没有数据,错误码:0x%08X\n"),hr);
    
    do 
    {
        GRS_PRINTF(_T("|%6s|%-40s|\n")
            ,tb1.GetString(1)
            ,tb1.GetString(2));
        hr = tb1.MoveNext();
    } 
    while( SUCCEEDED(hr) && DB_S_ENDOFROWSET != hr);

CLEAR_UP:
    _tsystem(_T("PAUSE"));
    CoUninitialize();
}