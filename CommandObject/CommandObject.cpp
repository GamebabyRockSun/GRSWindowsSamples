#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define COM_NO_WINDOWS_H    //����Ѿ�������Windows.h��ʹ������Windows�⺯��ʱ
#define DBINITCONSTANTS
#define INITGUID
#define OLEDBVER 0x0260		//Ϊ��ICommandStream�ӿڶ���Ϊ2.6��
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
	GRS_PRINTF(_T("��֧��'%s'�ӿ�\n"),_T(#iid));\
	}\
	else\
	{\
	GRS_PRINTF(_T("֧��'%s'�ӿ�\n"),_T(#iid));\
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
	GRS_COM_CHECK(hr,_T("��ȡIDBCreateCommand�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

	hr = pIDBCreateCommand->CreateCommand(NULL,IID_ICommand,(IUnknown**)&pICommand);
	GRS_COM_CHECK(hr,_T("����ICommand�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

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

	//1��ʹ��OLEDB���ݿ����ӶԻ������ӵ�����Դ
	HRESULT hr = CoCreateInstance(CLSID_DataLinks, NULL, CLSCTX_INPROC_SERVER,
		IID_IDBPromptInitialize, (void **)&pIDBPromptInitialize);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("�޷�����IDBPromptInitialize�ӿڣ������룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pDBConnect);
		GRS_SAFERELEASE(pIDBPromptInitialize);
		return FALSE;
	}

	//������佫�������ݿ����ӶԻ���
	hr = pIDBPromptInitialize->PromptDataSource(NULL, hWndParent,
		DBPROMPTOPTIONS_PROPERTYSHEET, 0, NULL, NULL, IID_IDBInitialize,
		(IUnknown **)&pDBConnect);
	if( FAILED(hr) )
	{
		GRS_PRINTF(_T("�������ݿ�Դ�Ի�����󣬴����룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pDBConnect);
		GRS_SAFERELEASE(pIDBPromptInitialize);
		return FALSE;
	}
	GRS_SAFERELEASE(pIDBPromptInitialize);
	
	hr = pDBConnect->Initialize();//���ݶԻ���ɼ��Ĳ������ӵ�ָ�������ݿ�
	if( FAILED(hr) )
	{
		GRS_PRINTF(_T("��ʼ������Դ���Ӵ��󣬴����룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}

	if(FAILED(hr = pDBConnect->QueryInterface(IID_IDBCreateSession,(void**)& pIDBCreateSession)))
	{
		GRS_PRINTF(_T("��ȡIDBCreateSession�ӿ�ʧ�ܣ������룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDBCreateSession);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}
	GRS_SAFERELEASE(pDBConnect);

	hr = pIDBCreateSession->CreateSession(NULL,IID_IOpenRowset,(IUnknown**)&pIOpenRowset);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("����Session�����ȡIOpenRowset�ӿ�ʧ�ܣ������룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pDBConnect);
		GRS_SAFERELEASE(pIDBCreateSession);
		GRS_SAFERELEASE(pIOpenRowset);
		return FALSE;
	}
	//��������Դ���ӵĽӿڿ��Բ�Ҫ�ˣ���Ҫʱ����ͨ��Session�����IGetDataSource�ӿڻ�ȡ
	GRS_SAFERELEASE(pDBConnect);
	GRS_SAFERELEASE(pIDBCreateSession);
	return TRUE;
}
