#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
//ʹ��ATL��OLEDBʹ����ģ��
#include <atldbcli.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);
//ע��������е�AtlTraceErrorRecords����
#define GRS_COM_CHECK(hr,...) if(FAILED(hr)){GRS_PRINTF(__VA_ARGS__);AtlTraceErrorRecords(hr);goto CLEAR_UP;}

//��̬�������������һ�����õĽ�������� 
typedef CCommand<CDynamicAccessor,CRowset,CMultipleResults> CGRSRowSet;
typedef CTable<CDynamicAccessor,CRowset> CGRSTable;
//CDynamicStringAccessor�������������е��ж�ת�����ַ�����
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
    GRS_COM_CHECK(hr,_T("��������Դ����ʧ��,������:0x%08X\n"),hr);
    
    hr = ss.Open(db);
    GRS_COM_CHECK(hr,_T("�����Ự����ʧ��,������:0x%08X\n"),hr);
    
    TableID.eKind           = DBKIND_NAME;
	TableID.uName.pwszName  = (LPOLESTR)pszTableName;

    hr = tb1.Open(ss,TableID);
    //Ҳ����д�����������,ֱ�Ӹ�����
    //hr = tb1.Open(ss,pszTableName);
    GRS_COM_CHECK(hr,_T("�򿪱����ʧ��,������:0x%08X\n"),hr);
    hr = tb1.MoveFirst();
    GRS_COM_CHECK(hr,_T("�������û������,������:0x%08X\n"),hr);
    
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