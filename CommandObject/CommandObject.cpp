#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define COM_NO_WINDOWS_H    //如果已经包含了Windows.h或不使用其他Windows库函数时
#define DBINITCONSTANTS
#define INITGUID
#define OLEDBVER 0x0260		//为了ICommandStream接口定义为2.6版
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

BOOL CreateDBSession(IOpenRowset*&pIOpenRowset);

int _tmain()
{
	GRS_USEPRINTF();
	CoInitialize(NULL);
	HRESULT				hr					= S_OK;
	IOpenRowset*		pIOpenRowset		= NULL;
	IDBCreateCommand*	pIDBCreateCommand	= NULL;

	GRS_DEF_INTERFACE(IAccessor);
	GRS_DEF_INTERFACE(IColumnsInfo);
	GRS_DEF_INTERFACE(ICommand);
	GRS_DEF_INTERFACE(ICommandProperties);
	GRS_DEF_INTERFACE(ICommandText);
	GRS_DEF_INTERFACE(IConvertType);
	GRS_DEF_INTERFACE(IColumnsRowset);
	GRS_DEF_INTERFACE(ICommandPersist);
	GRS_DEF_INTERFACE(ICommandPrepare);
	GRS_DEF_INTERFACE(ICommandWithParameters);
	GRS_DEF_INTERFACE(ISupportErrorInfo);
	GRS_DEF_INTERFACE(ICommandStream);

	if(!CreateDBSession(pIOpenRowset))
	{
		_tsystem(_T("PAUSE"));
		return -1;
	}
	hr = pIOpenRowset->QueryInterface(IID_IDBCreateCommand,(void**)&pIDBCreateCommand);
	GRS_COM_CHECK(hr,_T("获取IDBCreateCommand接口失败，错误码：0x%08X\n"),hr);

	hr = pIDBCreateCommand->CreateCommand(NULL,IID_ICommand,(IUnknown**)&pICommand);
	GRS_COM_CHECK(hr,_T("创建ICommand接口失败，错误码：0x%08X\n"),hr);

	GRS_QUERYINTERFACE(pICommand,IAccessor);
	GRS_QUERYINTERFACE(pICommand,IColumnsInfo);
	GRS_QUERYINTERFACE(pICommand,ICommandProperties);
	GRS_QUERYINTERFACE(pICommand,ICommandText);
	GRS_QUERYINTERFACE(pICommand,IConvertType);
	GRS_QUERYINTERFACE(pICommand,IColumnsRowset);
	GRS_QUERYINTERFACE(pICommand,ICommandPersist);
	GRS_QUERYINTERFACE(pICommand,ICommandPrepare);
	GRS_QUERYINTERFACE(pICommand,ICommandWithParameters);
	GRS_QUERYINTERFACE(pICommand,ISupportErrorInfo);
	GRS_QUERYINTERFACE(pICommand,ICommandStream);

CLEAR_UP:
	GRS_SAFERELEASE(pIOpenRowset);
	GRS_SAFERELEASE(pIDBCreateCommand);
	GRS_SAFERELEASE(pICommand);
	_tsystem(_T("PAUSE"));
	CoUninitialize();
	return 0;
}

BOOL CreateDBSession(IOpenRowset*&pIOpenRowset)
{
	GRS_USEPRINTF();
	GRS_SAFERELEASE(pIOpenRowset);

	IDBPromptInitialize*	pIDBPromptInitialize	= NULL;
	IDBInitialize*			pDBConnect				= NULL;
	IDBCreateSession*		pIDBCreateSession		= NULL;
	
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

	if(FAILED(hr = pDBConnect->QueryInterface(IID_IDBCreateSession,(void**)& pIDBCreateSession)))
	{
		GRS_PRINTF(_T("获取IDBCreateSession接口失败，错误码：0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDBCreateSession);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}
	GRS_SAFERELEASE(pDBConnect);

	hr = pIDBCreateSession->CreateSession(NULL,IID_IOpenRowset,(IUnknown**)&pIOpenRowset);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("创建Session对象获取IOpenRowset接口失败，错误码：0x%08x\n"),hr);
		GRS_SAFERELEASE(pDBConnect);
		GRS_SAFERELEASE(pIDBCreateSession);
		GRS_SAFERELEASE(pIOpenRowset);
		return FALSE;
	}
	//代表数据源连接的接口可以不要了，需要时可以通过Session对象的IGetDataSource接口获取
	GRS_SAFERELEASE(pDBConnect);
	GRS_SAFERELEASE(pIDBCreateSession);
	return TRUE;
}
