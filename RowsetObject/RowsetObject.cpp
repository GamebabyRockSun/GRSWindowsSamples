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

	ICommand*			pICommand			= NULL;
	ICommandText*		pICommandText		= NULL;
	ICommandProperties* pICommandProperties = NULL;

	GRS_DEF_INTERFACE(IAccessor);
	GRS_DEF_INTERFACE(IColumnsInfo);
	GRS_DEF_INTERFACE(IConvertType);
	GRS_DEF_INTERFACE(IRowset);
	GRS_DEF_INTERFACE(IRowsetInfo);
	GRS_DEF_INTERFACE(IChapteredRowset);
	GRS_DEF_INTERFACE(IColumnsInfo2);
	GRS_DEF_INTERFACE(IColumnsRowset);
	GRS_DEF_INTERFACE(IConnectionPointContainer);
	GRS_DEF_INTERFACE(IDBAsynchStatus);
	GRS_DEF_INTERFACE(IGetRow);
	GRS_DEF_INTERFACE(IRowsetChange);
	GRS_DEF_INTERFACE(IRowsetChapterMember);
	GRS_DEF_INTERFACE(IRowsetCurrentIndex);
	GRS_DEF_INTERFACE(IRowsetFind);
	GRS_DEF_INTERFACE(IRowsetIdentity);
	GRS_DEF_INTERFACE(IRowsetIndex);
	GRS_DEF_INTERFACE(IRowsetLocate);
	GRS_DEF_INTERFACE(IRowsetRefresh);
	GRS_DEF_INTERFACE(IRowsetScroll);
	GRS_DEF_INTERFACE(IRowsetUpdate);
	GRS_DEF_INTERFACE(IRowsetView);
	GRS_DEF_INTERFACE(ISupportErrorInfo);
	GRS_DEF_INTERFACE(IRowsetBookmark);

	TCHAR* pSQL = _T("Select * From T_State");

	DBPROPSET ps[1] = {};
	DBPROP prop[20]	= {};

	if(!CreateDBSession(pIOpenRowset))
	{
		_tsystem(_T("PAUSE"));
		return -1;
	}
	hr = pIOpenRowset->QueryInterface(IID_IDBCreateCommand,(void**)&pIDBCreateCommand);
	GRS_COM_CHECK(hr,_T("获取IDBCreateCommand接口失败，错误码：0x%08X\n"),hr);

	hr = pIDBCreateCommand->CreateCommand(NULL,IID_ICommand,(IUnknown**)&pICommand);
	GRS_COM_CHECK(hr,_T("创建ICommand接口失败，错误码：0x%08X\n"),hr);

	//执行一个命令
	hr = pICommand->QueryInterface(IID_ICommandText,(void**)&pICommandText);
	GRS_COM_CHECK(hr,_T("获取ICommandText接口失败，错误码：0x%08X\n"),hr);
	hr = pICommandText->SetCommandText(DBGUID_DEFAULT,pSQL);
	GRS_COM_CHECK(hr,_T("设置SQL语句失败，错误码：0x%08X\n"),hr);

	//设置属性
	prop[0].dwPropertyID    = DBPROP_UPDATABILITY;
	prop[0].vValue.vt       = VT_I4;
	//设置执行后的结果集可以进行增删改操作
	prop[0].vValue.lVal     =   DBPROPVAL_UP_CHANGE     //打开Update属性
		|DBPROPVAL_UP_DELETE					//打开Delete属性
		|DBPROPVAL_UP_INSERT;					//打开Insert属性
	prop[0].dwOptions       = DBPROPOPTIONS_REQUIRED;
	prop[0].colid           = DB_NULLID;

	//申请打开书签功能
	prop[1].dwPropertyID    = DBPROP_BOOKMARKS;
	prop[1].vValue.vt       = VT_BOOL;
	prop[1].vValue.boolVal  = VARIANT_TRUE;
	prop[1].dwOptions       = DBPROPOPTIONS_OPTIONAL;
	prop[1].colid           = DB_NULLID;

	//申请打开行查找功能
	prop[2].dwPropertyID    = DBPROP_IRowsetFind;
	prop[2].vValue.vt       = VT_BOOL;
	prop[2].vValue.boolVal  = VARIANT_TRUE;
	prop[2].dwOptions       = DBPROPOPTIONS_OPTIONAL;
	prop[2].colid           = DB_NULLID;

	//申请打开行索引
	prop[3].dwPropertyID    = DBPROP_IRowsetIndex;
	prop[3].vValue.vt       = VT_BOOL;
	prop[3].vValue.boolVal  = VARIANT_TRUE;
	prop[3].dwOptions       = DBPROPOPTIONS_OPTIONAL;
	prop[3].colid           = DB_NULLID;

	//申请打开行定位功能
	prop[4].dwPropertyID    = DBPROP_IRowsetLocate;
	prop[4].vValue.vt       = VT_BOOL;
	prop[4].vValue.boolVal  = VARIANT_TRUE;
	prop[4].dwOptions       = DBPROPOPTIONS_OPTIONAL;
	prop[4].colid           = DB_NULLID;

	//申请打开行滚动功能
	prop[5].dwPropertyID    = DBPROP_IRowsetScroll;
	prop[5].vValue.vt       = VT_BOOL;
	prop[5].vValue.boolVal  = VARIANT_TRUE;
	prop[5].dwOptions       = DBPROPOPTIONS_OPTIONAL;
	prop[5].colid           = DB_NULLID;

	//申请打开行集视图功能
	prop[6].dwPropertyID    = DBPROP_IRowsetView;
	prop[6].vValue.vt       = VT_BOOL;
	prop[6].vValue.boolVal  = VARIANT_TRUE;
	prop[6].dwOptions       = DBPROPOPTIONS_OPTIONAL;
	prop[6].colid           = DB_NULLID;

	//申请打开行集刷新功能
	prop[7].dwPropertyID    = DBPROP_IRowsetRefresh;
	prop[7].vValue.vt       = VT_BOOL;
	prop[7].vValue.boolVal  = VARIANT_TRUE;
	prop[7].dwOptions       = DBPROPOPTIONS_OPTIONAL;
	prop[7].colid           = DB_NULLID;

	//申请打开列信息扩展接口
	prop[8].dwPropertyID    = DBPROP_IColumnsInfo2;
	prop[8].vValue.vt       = VT_BOOL;
	prop[8].vValue.boolVal  = VARIANT_TRUE;
	prop[8].dwOptions       = DBPROPOPTIONS_OPTIONAL;
	prop[8].colid           = DB_NULLID;

	//申请打开数据库同步状态接口
	prop[9].dwPropertyID    = DBPROP_IDBAsynchStatus;
	prop[9].vValue.vt       = VT_BOOL;
	prop[9].vValue.boolVal  = VARIANT_TRUE;
	prop[9].dwOptions       = DBPROPOPTIONS_OPTIONAL;
	prop[9].colid           = DB_NULLID;

	//申请打开行集分章功能
	prop[10].dwPropertyID   = DBPROP_IChapteredRowset;
	prop[10].vValue.vt      = VT_BOOL;
	prop[10].vValue.boolVal = VARIANT_TRUE;
	prop[10].dwOptions      = DBPROPOPTIONS_OPTIONAL;
	prop[10].colid          = DB_NULLID;

	prop[11].dwPropertyID   = DBPROP_IRowsetCurrentIndex;
	prop[11].vValue.vt      = VT_BOOL;
	prop[11].vValue.boolVal = VARIANT_TRUE;
	prop[11].dwOptions      = DBPROPOPTIONS_OPTIONAL;
	prop[11].colid          = DB_NULLID;

	prop[12].dwPropertyID   = DBPROP_IGetRow;
	prop[12].vValue.vt      = VT_BOOL;
	prop[12].vValue.boolVal = VARIANT_TRUE;
	prop[12].dwOptions      = DBPROPOPTIONS_OPTIONAL;
	prop[12].colid          = DB_NULLID;

	//申请打开行集更新功能
	prop[13].dwPropertyID   = DBPROP_IRowsetUpdate;
	prop[13].vValue.vt      = VT_BOOL;
	prop[13].vValue.boolVal = VARIANT_TRUE;
	prop[13].dwOptions      = DBPROPOPTIONS_OPTIONAL;
	prop[13].colid          = DB_NULLID;

	prop[14].dwPropertyID   = DBPROP_IConnectionPointContainer;
	prop[14].vValue.vt      = VT_BOOL;
	prop[14].vValue.boolVal = VARIANT_TRUE;
	prop[14].dwOptions      = DBPROPOPTIONS_OPTIONAL;
	prop[14].colid          = DB_NULLID;
	
	ps[0].guidPropertySet   = DBPROPSET_ROWSET;       //注意属性集合的名称
	ps[0].cProperties       = 15;
	ps[0].rgProperties      = prop;

	hr = pICommandText->QueryInterface(IID_ICommandProperties,
		(void**)& pICommandProperties);
	GRS_COM_CHECK(hr,_T("获取ICommandProperties接口失败，错误码：0x%08X\n"),hr);
	hr = pICommandProperties->SetProperties(1,ps);//注意必须在Execute前设定属性
	GRS_COM_CHECK(hr,_T("设置Command对象属性失败，错误码：0x%08X\n"),hr);
	hr = pICommandText->Execute(NULL,IID_IRowsetChange,NULL,NULL,(IUnknown**)&pIRowsetChange);
	GRS_COM_CHECK(hr,_T("执行SQL语句失败，错误码：0x%08X\n"),hr);

	GRS_QUERYINTERFACE(pIRowsetChange,IRowset);
	GRS_QUERYINTERFACE(pIRowsetChange,IAccessor);
	GRS_QUERYINTERFACE(pIRowsetChange,IColumnsInfo);
	GRS_QUERYINTERFACE(pIRowsetChange,IConvertType);
	GRS_QUERYINTERFACE(pIRowsetChange,IRowsetInfo);
	GRS_QUERYINTERFACE(pIRowsetChange,IChapteredRowset);
	GRS_QUERYINTERFACE(pIRowsetChange,IColumnsInfo2);
	GRS_QUERYINTERFACE(pIRowsetChange,IColumnsRowset);
	GRS_QUERYINTERFACE(pIRowsetChange,IConnectionPointContainer);
	GRS_QUERYINTERFACE(pIRowsetChange,IDBAsynchStatus);
	GRS_QUERYINTERFACE(pIRowsetChange,IGetRow);
	GRS_QUERYINTERFACE(pIRowsetChange,IRowsetChapterMember);
	GRS_QUERYINTERFACE(pIRowsetChange,IRowsetCurrentIndex);
	GRS_QUERYINTERFACE(pIRowsetChange,IRowsetFind);
	GRS_QUERYINTERFACE(pIRowsetChange,IRowsetIdentity);
	GRS_QUERYINTERFACE(pIRowsetChange,IRowsetIndex);
	GRS_QUERYINTERFACE(pIRowsetChange,IRowsetLocate);
	GRS_QUERYINTERFACE(pIRowsetChange,IRowsetRefresh);
	GRS_QUERYINTERFACE(pIRowsetChange,IRowsetScroll);
	GRS_QUERYINTERFACE(pIRowsetChange,IRowsetUpdate);
	GRS_QUERYINTERFACE(pIRowsetChange,IRowsetView);
	GRS_QUERYINTERFACE(pIRowsetChange,ISupportErrorInfo);
	GRS_QUERYINTERFACE(pIRowsetChange,IRowsetBookmark);

CLEAR_UP:
	GRS_SAFERELEASE(pIRowsetChange);
	GRS_SAFERELEASE(pICommandProperties);
	GRS_SAFERELEASE(pICommand);
	GRS_SAFERELEASE(pICommandText);
	GRS_SAFERELEASE(pIOpenRowset);
	GRS_SAFERELEASE(pIDBCreateCommand);

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
