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
	if(FAILED(hr = (i)->QueryInterface(IID_##iid, (void **)&p##iid)) )\
	{\
		GRS_PRINTF(_T("不支持'%s'接口\n"),_T(#iid));\
	}\
	else\
	{\
		GRS_PRINTF(_T("支持'%s'接口\n"),_T(#iid));\
	}\
	GRS_SAFERELEASE(p##iid);

//FAILED(hr = CLSIDFromProgID(L"SQLOLEDB", &clsid) )
int _tmain()
{
	GRS_USEPRINTF();

	CoInitialize(NULL);
	IDBPromptInitialize*		pIDBPromptInitialize		= NULL;
	IDataInitialize*			pIDataInitialize			= NULL;
    LPOLESTR                    pLinkString                 = NULL;
	//以下是IDBInitialize接口的等价接口定义
	//1、必须支持的接口
	GRS_DEF_INTERFACE(IDBInitialize);
	GRS_DEF_INTERFACE(IDBCreateSession);
	GRS_DEF_INTERFACE(IDBProperties);
	GRS_DEF_INTERFACE(IPersist);
	//2、可选支持的接口
	GRS_DEF_INTERFACE(IConnectionPointContainer);
	GRS_DEF_INTERFACE(IDBAsynchStatus);
	GRS_DEF_INTERFACE(IDBDataSourceAdmin);
	GRS_DEF_INTERFACE(IDBInfo);
	GRS_DEF_INTERFACE(IPersistFile);
	GRS_DEF_INTERFACE(ISupportErrorInfo);

	HWND hWndParent = GetDesktopWindow();
	CLSID clsid = {};
	
	//1、使用OLEDB数据库连接对话框连接到数据源
	HRESULT hr = CoCreateInstance(CLSID_DataLinks, NULL, CLSCTX_INPROC_SERVER,
		IID_IDBPromptInitialize, (void **)&pIDBPromptInitialize);
	GRS_COM_CHECK(hr,_T("无法创建IDBPromptInitialize接口，错误码：0x%08x\n"),hr);

	//下面这句将弹出数据库连接对话框
	hr = pIDBPromptInitialize->PromptDataSource(NULL, hWndParent,
		DBPROMPTOPTIONS_PROPERTYSHEET, 0, NULL, NULL, IID_IDBInitialize,
		(IUnknown **)&pIDBInitialize);
	GRS_COM_CHECK(hr,_T("弹出数据库源对话框错误，错误码：0x%08x\n"),hr);

    hr = pIDBPromptInitialize->QueryInterface(IID_IDataInitialize,(void**)&pIDataInitialize);
    hr = pIDataInitialize->GetInitializationString(pIDBInitialize,TRUE,&pLinkString);

	hr = pIDBInitialize->Initialize();//根据对话框采集的参数连接到指定的数据库
	GRS_COM_CHECK(hr,_T("初始化数据源连接错误，错误码：0x%08x\n"),hr);

	//测试都支持哪些接口
	GRS_QUERYINTERFACE(pIDBInitialize,IDBCreateSession);
	GRS_QUERYINTERFACE(pIDBInitialize,IDBProperties);
	GRS_QUERYINTERFACE(pIDBInitialize,IPersist);

	GRS_QUERYINTERFACE(pIDBInitialize,IConnectionPointContainer);
	GRS_QUERYINTERFACE(pIDBInitialize,IDBAsynchStatus);
	GRS_QUERYINTERFACE(pIDBInitialize,IDBDataSourceAdmin);
	GRS_QUERYINTERFACE(pIDBInitialize,IDBInfo);
	GRS_QUERYINTERFACE(pIDBInitialize,IPersistFile);
	GRS_QUERYINTERFACE(pIDBInitialize,ISupportErrorInfo);


	//断开数据库连接
	pIDBInitialize->Uninitialize();
	GRS_SAFERELEASE(pIDBInitialize);
	GRS_SAFERELEASE(pIDBPromptInitialize);
	
	//2、使用纯代码方式连接到指定的数据源
	hr = CoCreateInstance(CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER,
		IID_IDataInitialize,(void**)&pIDataInitialize);
	GRS_COM_CHECK(hr,_T("无法创建IDataInitialize接口，错误码：0x%08x\n"),hr);
	
	
	hr = CLSIDFromProgID(_T("SQLOLEDB"), &clsid);
	GRS_COM_CHECK(hr,_T("获取\"SQLOLEDB\"的CLSID错误，错误码：0x%08x\n"),hr);
	//创建数据源连接对象和初始化接口
	hr = pIDataInitialize->CreateDBInstance(clsid, NULL,
		CLSCTX_INPROC_SERVER, NULL, IID_IDBInitialize,
		(IUnknown**)&pIDBInitialize);
	GRS_COM_CHECK(hr,_T("调用IDataInitialize->CreateDBInstance创建IDBInitialize接口错误，错误码：0x%08x\n"),hr);

	//准备连接属性
	//注意虽然后面只使用了4个属性，但是任然要定义和初始化4+1个属性
	//目的是让属性数组最后总有一个空的属性作为数组结尾
	DBPROP InitProperties[5] = {};
	DBPROPSET rgInitPropSet[1] = {};

	//指定数据库实例名，这里使用了别名local，指定本地默认实例
	InitProperties[0].dwPropertyID      = DBPROP_INIT_DATASOURCE;
	InitProperties[0].vValue.vt         = VT_BSTR;
	InitProperties[0].vValue.bstrVal    = SysAllocString(L"ASUS-PC\\SQL2008");
	InitProperties[0].dwOptions         = DBPROPOPTIONS_REQUIRED;
	InitProperties[0].colid             = DB_NULLID;
	//指定数据库名
	InitProperties[1].dwPropertyID      = DBPROP_INIT_CATALOG;
	InitProperties[1].vValue.vt         = VT_BSTR;
	InitProperties[1].vValue.bstrVal    = SysAllocString(L"Study");
	InitProperties[1].dwOptions         = DBPROPOPTIONS_REQUIRED;
	InitProperties[1].colid             = DB_NULLID;
	////指定身份验证方式为集成安全模式“SSPI”
	//InitProperties[2].dwPropertyID = DBPROP_AUTH_INTEGRATED;
	//InitProperties[2].vValue.vt = VT_BSTR;
	//InitProperties[2].vValue.bstrVal = SysAllocString(L"SSPI");
	//InitProperties[2].dwOptions = DBPROPOPTIONS_REQUIRED;
	//InitProperties[2].colid = DB_NULLID;

	// User ID
	InitProperties[2].dwPropertyID      = DBPROP_AUTH_USERID;
	InitProperties[2].vValue.vt         = VT_BSTR;
	InitProperties[2].vValue.bstrVal    = SysAllocString(OLESTR("sa"));

	// Password
	InitProperties[3].dwPropertyID      = DBPROP_AUTH_PASSWORD;
	InitProperties[3].vValue.vt         = VT_BSTR;
	InitProperties[3].vValue.bstrVal    = SysAllocString(OLESTR("999999"));


	//创建一个GUID为DBPROPSET_DBINIT的属性集合，这也是初始化连接时需要的唯一一//个属性集合
	rgInitPropSet[0].guidPropertySet = DBPROPSET_DBINIT;
	rgInitPropSet[0].cProperties = 4;
	rgInitPropSet[0].rgProperties = InitProperties;

	//得到数据库初始化的属性接口
	hr = pIDBInitialize->QueryInterface(IID_IDBProperties, (void **)&pIDBProperties);
	GRS_COM_CHECK(hr,_T("获取IDBProperties接口失败，错误码：0x%08x\n"),hr);
	hr = pIDBProperties->SetProperties(1, rgInitPropSet); 
	GRS_COM_CHECK(hr,_T("设置连接属性失败，错误码：0x%08x\n"),hr);
	//属性一但设置完成，相应的接口就可以释放了
	GRS_SAFERELEASE(pIDBProperties);


	//根据指定的属性连接到数据库
	hr = pIDBInitialize->Initialize();
	GRS_COM_CHECK(hr,_T("使用属性连接到数据源失败，错误码：0x%08x\n"),hr);
	GRS_PRINTF(_T("使用属性集方式连接到数据源成功!\n"));

	pIDBInitialize->Uninitialize();
	GRS_SAFERELEASE(pIDBInitialize);


	//L"Provider=Microsoft.Jet.OLEDB.4.0;User ID=Admin;Data Source=D:\\DBTest\\db1.mdb;Mode=Share Deny None;"
	//3、使用连接字符串得到一个IDBInitialize接口
	hr = pIDataInitialize->GetDataSource(NULL,
		CLSCTX_INPROC_SERVER,
		L"Provider=SQLOLEDB.1;Persist Security Info=False;User ID=sa;Password = 999999;Initial Catalog=Study;Data Source=ASUS-PC\\SQL2008;",
		__uuidof(IDBInitialize), 
		(IUnknown**)&pIDBInitialize);
	GRS_COM_CHECK(hr,_T("使用连接字符串获取数据源接口IDBInitialize失败，错误码：0x%08x\n"),hr);
	//连接到数据库
	hr = pIDBInitialize ->Initialize();
	GRS_COM_CHECK(hr,_T("使用连接字符串连接到数据源失败，错误码：0x%08x\n"),hr);

	GRS_PRINTF(_T("使用字符串方式连接到数据源成功!\n"));

CLEAR_UP:
    if(NULL != pIDBInitialize)
    {
        pIDBInitialize->Uninitialize();
    }
	GRS_SAFERELEASE(pIDBProperties);
	GRS_SAFERELEASE(pIDataInitialize);
	GRS_SAFERELEASE(pIDBInitialize);
	GRS_SAFERELEASE(pIDBPromptInitialize);
	
    _tsystem(_T("PAUSE"));
    CoUninitialize();
	return 0;
}