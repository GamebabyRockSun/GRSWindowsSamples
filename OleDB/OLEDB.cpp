#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define COM_NO_WINDOWS_H    //����Ѿ�������Windows.h��ʹ������Windows�⺯��ʱ
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
		GRS_PRINTF(_T("��֧��'%s'�ӿ�\n"),_T(#iid));\
	}\
	else\
	{\
		GRS_PRINTF(_T("֧��'%s'�ӿ�\n"),_T(#iid));\
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
	//������IDBInitialize�ӿڵĵȼ۽ӿڶ���
	//1������֧�ֵĽӿ�
	GRS_DEF_INTERFACE(IDBInitialize);
	GRS_DEF_INTERFACE(IDBCreateSession);
	GRS_DEF_INTERFACE(IDBProperties);
	GRS_DEF_INTERFACE(IPersist);
	//2����ѡ֧�ֵĽӿ�
	GRS_DEF_INTERFACE(IConnectionPointContainer);
	GRS_DEF_INTERFACE(IDBAsynchStatus);
	GRS_DEF_INTERFACE(IDBDataSourceAdmin);
	GRS_DEF_INTERFACE(IDBInfo);
	GRS_DEF_INTERFACE(IPersistFile);
	GRS_DEF_INTERFACE(ISupportErrorInfo);

	HWND hWndParent = GetDesktopWindow();
	CLSID clsid = {};
	
	//1��ʹ��OLEDB���ݿ����ӶԻ������ӵ�����Դ
	HRESULT hr = CoCreateInstance(CLSID_DataLinks, NULL, CLSCTX_INPROC_SERVER,
		IID_IDBPromptInitialize, (void **)&pIDBPromptInitialize);
	GRS_COM_CHECK(hr,_T("�޷�����IDBPromptInitialize�ӿڣ������룺0x%08x\n"),hr);

	//������佫�������ݿ����ӶԻ���
	hr = pIDBPromptInitialize->PromptDataSource(NULL, hWndParent,
		DBPROMPTOPTIONS_PROPERTYSHEET, 0, NULL, NULL, IID_IDBInitialize,
		(IUnknown **)&pIDBInitialize);
	GRS_COM_CHECK(hr,_T("�������ݿ�Դ�Ի�����󣬴����룺0x%08x\n"),hr);

    hr = pIDBPromptInitialize->QueryInterface(IID_IDataInitialize,(void**)&pIDataInitialize);
    hr = pIDataInitialize->GetInitializationString(pIDBInitialize,TRUE,&pLinkString);

	hr = pIDBInitialize->Initialize();//���ݶԻ���ɼ��Ĳ������ӵ�ָ�������ݿ�
	GRS_COM_CHECK(hr,_T("��ʼ������Դ���Ӵ��󣬴����룺0x%08x\n"),hr);

	//���Զ�֧����Щ�ӿ�
	GRS_QUERYINTERFACE(pIDBInitialize,IDBCreateSession);
	GRS_QUERYINTERFACE(pIDBInitialize,IDBProperties);
	GRS_QUERYINTERFACE(pIDBInitialize,IPersist);

	GRS_QUERYINTERFACE(pIDBInitialize,IConnectionPointContainer);
	GRS_QUERYINTERFACE(pIDBInitialize,IDBAsynchStatus);
	GRS_QUERYINTERFACE(pIDBInitialize,IDBDataSourceAdmin);
	GRS_QUERYINTERFACE(pIDBInitialize,IDBInfo);
	GRS_QUERYINTERFACE(pIDBInitialize,IPersistFile);
	GRS_QUERYINTERFACE(pIDBInitialize,ISupportErrorInfo);


	//�Ͽ����ݿ�����
	pIDBInitialize->Uninitialize();
	GRS_SAFERELEASE(pIDBInitialize);
	GRS_SAFERELEASE(pIDBPromptInitialize);
	
	//2��ʹ�ô����뷽ʽ���ӵ�ָ��������Դ
	hr = CoCreateInstance(CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER,
		IID_IDataInitialize,(void**)&pIDataInitialize);
	GRS_COM_CHECK(hr,_T("�޷�����IDataInitialize�ӿڣ������룺0x%08x\n"),hr);
	
	
	hr = CLSIDFromProgID(_T("SQLOLEDB"), &clsid);
	GRS_COM_CHECK(hr,_T("��ȡ\"SQLOLEDB\"��CLSID���󣬴����룺0x%08x\n"),hr);
	//��������Դ���Ӷ���ͳ�ʼ���ӿ�
	hr = pIDataInitialize->CreateDBInstance(clsid, NULL,
		CLSCTX_INPROC_SERVER, NULL, IID_IDBInitialize,
		(IUnknown**)&pIDBInitialize);
	GRS_COM_CHECK(hr,_T("����IDataInitialize->CreateDBInstance����IDBInitialize�ӿڴ��󣬴����룺0x%08x\n"),hr);

	//׼����������
	//ע����Ȼ����ֻʹ����4�����ԣ�������ȻҪ����ͳ�ʼ��4+1������
	//Ŀ���������������������һ���յ�������Ϊ�����β
	DBPROP InitProperties[5] = {};
	DBPROPSET rgInitPropSet[1] = {};

	//ָ�����ݿ�ʵ����������ʹ���˱���local��ָ������Ĭ��ʵ��
	InitProperties[0].dwPropertyID      = DBPROP_INIT_DATASOURCE;
	InitProperties[0].vValue.vt         = VT_BSTR;
	InitProperties[0].vValue.bstrVal    = SysAllocString(L"ASUS-PC\\SQL2008");
	InitProperties[0].dwOptions         = DBPROPOPTIONS_REQUIRED;
	InitProperties[0].colid             = DB_NULLID;
	//ָ�����ݿ���
	InitProperties[1].dwPropertyID      = DBPROP_INIT_CATALOG;
	InitProperties[1].vValue.vt         = VT_BSTR;
	InitProperties[1].vValue.bstrVal    = SysAllocString(L"Study");
	InitProperties[1].dwOptions         = DBPROPOPTIONS_REQUIRED;
	InitProperties[1].colid             = DB_NULLID;
	////ָ�������֤��ʽΪ���ɰ�ȫģʽ��SSPI��
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


	//����һ��GUIDΪDBPROPSET_DBINIT�����Լ��ϣ���Ҳ�ǳ�ʼ������ʱ��Ҫ��Ψһһ//�����Լ���
	rgInitPropSet[0].guidPropertySet = DBPROPSET_DBINIT;
	rgInitPropSet[0].cProperties = 4;
	rgInitPropSet[0].rgProperties = InitProperties;

	//�õ����ݿ��ʼ�������Խӿ�
	hr = pIDBInitialize->QueryInterface(IID_IDBProperties, (void **)&pIDBProperties);
	GRS_COM_CHECK(hr,_T("��ȡIDBProperties�ӿ�ʧ�ܣ������룺0x%08x\n"),hr);
	hr = pIDBProperties->SetProperties(1, rgInitPropSet); 
	GRS_COM_CHECK(hr,_T("������������ʧ�ܣ������룺0x%08x\n"),hr);
	//����һ��������ɣ���Ӧ�ĽӿھͿ����ͷ���
	GRS_SAFERELEASE(pIDBProperties);


	//����ָ�����������ӵ����ݿ�
	hr = pIDBInitialize->Initialize();
	GRS_COM_CHECK(hr,_T("ʹ���������ӵ�����Դʧ�ܣ������룺0x%08x\n"),hr);
	GRS_PRINTF(_T("ʹ�����Լ���ʽ���ӵ�����Դ�ɹ�!\n"));

	pIDBInitialize->Uninitialize();
	GRS_SAFERELEASE(pIDBInitialize);


	//L"Provider=Microsoft.Jet.OLEDB.4.0;User ID=Admin;Data Source=D:\\DBTest\\db1.mdb;Mode=Share Deny None;"
	//3��ʹ�������ַ����õ�һ��IDBInitialize�ӿ�
	hr = pIDataInitialize->GetDataSource(NULL,
		CLSCTX_INPROC_SERVER,
		L"Provider=SQLOLEDB.1;Persist Security Info=False;User ID=sa;Password = 999999;Initial Catalog=Study;Data Source=ASUS-PC\\SQL2008;",
		__uuidof(IDBInitialize), 
		(IUnknown**)&pIDBInitialize);
	GRS_COM_CHECK(hr,_T("ʹ�������ַ�����ȡ����Դ�ӿ�IDBInitializeʧ�ܣ������룺0x%08x\n"),hr);
	//���ӵ����ݿ�
	hr = pIDBInitialize ->Initialize();
	GRS_COM_CHECK(hr,_T("ʹ�������ַ������ӵ�����Դʧ�ܣ������룺0x%08x\n"),hr);

	GRS_PRINTF(_T("ʹ���ַ�����ʽ���ӵ�����Դ�ɹ�!\n"));

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