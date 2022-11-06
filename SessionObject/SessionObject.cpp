#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define COM_NO_WINDOWS_H    //如果已经包含了Windows.h或不使用其他Windows库函数时
#define DBINITCONSTANTS
#define INITGUID
#define OLEDBVER 0x0260 
#include <oledb.h>
#include <oledberr.h>
#include <msdasc.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_SAFERELEASE(I) if(NULL != I){I->Release();I=NULL;}
#define GRS_COM_CHECK(hr,...) if(FAILED(hr)){GRS_PRINTF(__VA_ARGS__);goto CLEAR_UP;}

#define GRS_DEF_INTERFACE(i) i* p##i = NULL;
#define GRS_QUERYINTERFACE(i,iid) \
	if(FAILED(hr = i->QueryInterface(IID_##iid, (void **)&p##iid)) )\
	{\
	GRS_PRINTF(_T("不支持'%s'接口\n"),_T(#iid));\
	}\
	else\
	{\
	GRS_PRINTF(_T("支持'%s'接口\n"),_T(#iid));\
	}\
	GRS_SAFERELEASE(p##iid);

BOOL DlgConnectDB(IDBInitialize*& pDBConnect);


int _tmain()
{
	GRS_USEPRINTF();
	CoInitialize(NULL);
	HRESULT					hr					= S_OK;
	IDBInitialize*			pIDBConnect			= NULL;
	IDBCreateSession *		pIDBCreateSession	= NULL;

    GRS_DEF_INTERFACE(IGetDataSource);
    GRS_DEF_INTERFACE(IOpenRowset);
    GRS_DEF_INTERFACE(ISessionProperties);
    GRS_DEF_INTERFACE(IAlterIndex);
    GRS_DEF_INTERFACE(IAlterTable);
    GRS_DEF_INTERFACE(IBindResource);
    GRS_DEF_INTERFACE(ICreateRow);
    GRS_DEF_INTERFACE(IDBCreateCommand);
    GRS_DEF_INTERFACE(IDBSchemaRowset);
    GRS_DEF_INTERFACE(IIndexDefinition);
    GRS_DEF_INTERFACE(ISupportErrorInfo);
    GRS_DEF_INTERFACE(ITableCreation);
    GRS_DEF_INTERFACE(ITableDefinition);
    GRS_DEF_INTERFACE(ITableDefinitionWithConstraints);
    GRS_DEF_INTERFACE(ITransaction);
    GRS_DEF_INTERFACE(ITransactionJoin);
    GRS_DEF_INTERFACE(ITransactionLocal);
    GRS_DEF_INTERFACE(ITransactionObject);

	if(!DlgConnectDB(pIDBConnect))
	{
		_tsystem(_T("PAUSE"));
		return 0;
	}
	//首先从连接对象接口找到等价的CreateSession接口
	hr = pIDBConnect->QueryInterface(
		IID_IDBCreateSession, (void**)&pIDBCreateSession);
	GRS_COM_CHECK(hr,_T("获取IDBCreateSession接口失败，错误码：0x%08X\n"),hr);

	//创建一个IOpenRowset接口
	hr = pIDBCreateSession->CreateSession(
		NULL,              
		IID_IOpenRowset,
		(IUnknown**)&pIOpenRowset);
	GRS_COM_CHECK(hr,_T("创建IOpenRowset接口失败，错误码：0x%08X\n"),hr);
	//IDBCreateSession可以释放了，需要时再Query出来就行了           
	GRS_SAFERELEASE( pIDBCreateSession );

	//测试数据源支持事务对象的哪些接口
    GRS_QUERYINTERFACE(pIOpenRowset,IGetDataSource);
    GRS_QUERYINTERFACE(pIOpenRowset,ISessionProperties);
    GRS_QUERYINTERFACE(pIOpenRowset,IAlterIndex);
    GRS_QUERYINTERFACE(pIOpenRowset,IAlterTable);
    GRS_QUERYINTERFACE(pIOpenRowset,IBindResource);
    GRS_QUERYINTERFACE(pIOpenRowset,ICreateRow);
    GRS_QUERYINTERFACE(pIOpenRowset,IDBCreateCommand);
    GRS_QUERYINTERFACE(pIOpenRowset,IDBSchemaRowset);
    GRS_QUERYINTERFACE(pIOpenRowset,IIndexDefinition);
    GRS_QUERYINTERFACE(pIOpenRowset,ISupportErrorInfo);
    GRS_QUERYINTERFACE(pIOpenRowset,ITableCreation);
    GRS_QUERYINTERFACE(pIOpenRowset,ITableDefinition);
    GRS_QUERYINTERFACE(pIOpenRowset,ITableDefinitionWithConstraints);
    GRS_QUERYINTERFACE(pIOpenRowset,ITransaction);
    GRS_QUERYINTERFACE(pIOpenRowset,ITransactionJoin);
    GRS_QUERYINTERFACE(pIOpenRowset,ITransactionLocal);
    GRS_QUERYINTERFACE(pIOpenRowset,ITransactionObject);

CLEAR_UP:
	GRS_SAFERELEASE(pIOpenRowset);
	GRS_SAFERELEASE(pIDBConnect);
	GRS_SAFERELEASE(pIDBCreateSession);
	_tsystem(_T("PAUSE"));
	CoUninitialize();
	return 0;
}


BOOL DlgConnectDB(IDBInitialize*& pDBConnect)
{
	GRS_USEPRINTF();
	GRS_SAFERELEASE(pDBConnect);

	IDBPromptInitialize* pIDBPromptInitialize = NULL;

	HWND hWndParent = GetDesktopWindow();
	CLSID clsid = {};

	//1、使用OLEDB数据库连接对话框连接到数据源
	HRESULT hr = CoCreateInstance(CLSID_DataLinks, NULL, CLSCTX_INPROC_SERVER,
		IID_IDBPromptInitialize, (void **)&pIDBPromptInitialize);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("无法创建IDBPromptInitialize接口，错误码：0x%08x\n"),hr);
		GRS_SAFERELEASE(pDBConnect);
		GRS_SAFERELEASE(pIDBPromptInitialize);
		return FALSE;
	}

	//下面这句将弹出数据库连接对话框
	hr = pIDBPromptInitialize->PromptDataSource(NULL, hWndParent,
		DBPROMPTOPTIONS_PROPERTYSHEET, 0, NULL, NULL, IID_IDBInitialize,
		(IUnknown **)&pDBConnect);
	if( FAILED(hr) )
	{
		GRS_PRINTF(_T("弹出数据库源对话框错误，错误码：0x%08x\n"),hr);
		GRS_SAFERELEASE(pDBConnect);
		GRS_SAFERELEASE(pIDBPromptInitialize);
		return FALSE;
	}
	GRS_SAFERELEASE(pIDBPromptInitialize);
	hr = pDBConnect->Initialize();//根据对话框采集的参数连接到指定的数据库
	if( FAILED(hr) )
	{
		GRS_PRINTF(_T("初始化数据源连接错误，错误码：0x%08x\n"),hr);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}
	return TRUE;
}