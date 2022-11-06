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

#define GRS_ROUNDUP_AMOUNT 8 
#define GRS_ROUNDUP_(size,amount) (((ULONG)(size)+((amount)-1))&~((amount)-1)) 
#define GRS_ROUNDUP(size) GRS_ROUNDUP_(size, GRS_ROUNDUP_AMOUNT) 

#define MAX_DISPLAY_SIZE	30
#define MIN_DISPLAY_SIZE    3

BOOL CreateDBSession(IOpenRowset*&pIOpenRowset);
VOID DisplayColumnNames(DBORDINAL        cColumns
						,DBCOLUMNINFO*   rgColumnInfo
						,ULONG*          rgDispSize );
HRESULT DisplayRow(DBCOUNTITEM iRow,
				   DBCOUNTITEM cBindings,
				   DBBINDING * rgBindings,
				   void * pData,
				   ULONG * rgDispSize );

VOID UpdateDisplaySize (DBCOUNTITEM   cBindings,
						DBBINDING *   rgBindings,
						void *        pData,
						ULONG *       rgDispSize );

int _tmain()
{
	GRS_USEPRINTF();
	CoInitialize(NULL);
	HRESULT					hr						= S_OK;
	DWORD					dwReadChars				= 0;
	IOpenRowset*			pIOpenRowset			= NULL;
	IDBCreateCommand*		pIDBCreateCommand		= NULL;

	ICommand*				pICommand				= NULL;
	ICommandText*			pICommandText			= NULL;
	ICommandPrepare*        pICommandPrepare        = NULL;
	ICommandWithParameters* pICommandWithParameters = NULL;
	IAccessor*				pICommandAccessor		= NULL;

	GRS_DEF_INTERFACE(IAccessor);
	GRS_DEF_INTERFACE(IColumnsInfo);
	GRS_DEF_INTERFACE(IRowset);
	//ע��SQL�����ģ�����
	TCHAR* pSQL = _T("Select * From T_State Where Left(K_StateCode,2) = ?");

	DB_UPARAMS			iParamCnt			= 0;
	DBPARAMINFO*		pParamInfo			= 0;
	ULONG               cColumns			= 0;
	DBCOLUMNINFO *      rgColumnInfo        = NULL;
	LPWSTR              pStringBuffer       = NULL;
	LPWSTR				pStringBufferParam	= NULL;
	ULONG               iCol				= 0;
	ULONG               dwOffset            = 0;
	DBBINDING *         rgBindings          = NULL;
	DBBINDING *         rgParamBindings     = NULL;
	DBBINDSTATUS*		pBindingStatus		= NULL;
	HACCESSOR			hParamAccessor		= NULL;
	HACCESSOR			hAccessor			= NULL;
	DBPARAMS			dbParam				= {};
	void *              pData               = NULL;
	ULONG               cRowsObtained		= 0;
	HROW *              rghRows             = NULL;
	ULONG               iRow				= 0;
	ULONG				lAllRows			= 0;
	LONG                cRows               = 100;//һ�ζ�ȡ100��
	void *              pCurData			= NULL;

	LONG				lRowsetNum			= 0;

	ULONG*				plDisplayLen		= NULL;
	BOOL				bShowColumnName		= FALSE;


	if(!CreateDBSession(pIOpenRowset))
	{
		_tsystem(_T("PAUSE"));
		return -1;
	}
	hr = pIOpenRowset->QueryInterface(IID_IDBCreateCommand,(void**)&pIDBCreateCommand);
	GRS_COM_CHECK(hr,_T("��ȡIDBCreateCommand�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

	hr = pIDBCreateCommand->CreateCommand(NULL,IID_ICommand,(IUnknown**)&pICommand);
	GRS_COM_CHECK(hr,_T("����ICommand�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

	//ִ������
	hr = pICommand->QueryInterface(IID_ICommandText,(void**)&pICommandText);
	GRS_COM_CHECK(hr,_T("��ȡICommandText�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);
	hr = pICommandText->SetCommandText(DBGUID_DEFAULT,pSQL);
	GRS_COM_CHECK(hr,_T("����SQL���ʧ�ܣ������룺0x%08X\n"),hr);

	hr = pICommandText->QueryInterface(IID_ICommandPrepare,(void**)&pICommandPrepare);
	GRS_COM_CHECK(hr,_T("��ȡICommandPrepare�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);
	//Ԥ�����������������
	hr = pICommandPrepare->Prepare(0);
	GRS_COM_CHECK(hr,_T("����Prepare����ʧ�ܣ������룺0x%08X\n"),hr);

	//ICommandPrepare�ӿڿ����ͷ���
	GRS_SAFERELEASE(pICommandPrepare);

	hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParameters);
	GRS_COM_CHECK(hr,_T("��ȡICommandWithParameters�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);
	//��ò��������Ͳ�����Ϣ�ṹ����
	hr = pICommandWithParameters->GetParameterInfo(&iParamCnt,&pParamInfo,&pStringBufferParam);
	GRS_COM_CHECK(hr,_T("��ȡ��ѯ�Ĳ�����Ϣʧ�ܣ������룺0x%08X\n"),hr);

	//ICommandWithParameters�ӿڿ����ͷ���
	GRS_SAFERELEASE(pICommandWithParameters);

	if(iParamCnt > 0 )
	{
		//��Command������IAccessor�ӿ������Rowset������IAccessor�ӿ������������;��ͬ
		hr = pICommandText->QueryInterface(IID_IAccessor,(void**)&pICommandAccessor);
		GRS_COM_CHECK(hr,_T("��ȡCommand�����IAccessor�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);
		rgParamBindings = (DBBINDING*)GRS_CALLOC(iParamCnt * sizeof(DBBINDING));
		for(ULONG i = 0;i < iParamCnt; i++)
		{
			rgParamBindings[i].iOrdinal   = pParamInfo[i].iOrdinal;
			rgParamBindings[i].dwPart	  = DBPART_VALUE  | DBPART_LENGTH;//������ֵ�ͳ��ȾͿ�����
			rgParamBindings[i].obStatus   = 0;
			rgParamBindings[i].obLength   = dwOffset;
			rgParamBindings[i].obValue    = dwOffset + sizeof(ULONG);
			rgParamBindings[i].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
			rgParamBindings[i].eParamIO   = DBPARAMIO_INPUT;				//�ܹؼ��ı�־
			rgParamBindings[i].bPrecision = pParamInfo[i].bPrecision;
			rgParamBindings[i].bScale     = pParamInfo[i].bScale;
			rgParamBindings[i].wType      = DBTYPE_WSTR;//pParamInfo[i].wType;//ǿ�Ʋ���ΪWCHAR*����������
			//��Ҫע�����pPatamInfo�ṹ��ulParamSize�ĳ���ʵ���ǣ�ռλ���ĳ���
			//������ʵ�������ֶεĳ��ȣ������Ҫ�Լ�ָ���ȽϺ��ʵĳ���
			rgParamBindings[i].cbMaxLen   = 12 * sizeof(WCHAR);//pParamInfo[i].ulParamSize;

			dwOffset = rgParamBindings[i].cbMaxLen + rgParamBindings[i].obValue;
			dwOffset = GRS_ROUNDUP(dwOffset);
		}        
		pBindingStatus = (DBBINDSTATUS*)GRS_CALLOC(iParamCnt * sizeof(DBBINDSTATUS));
		hr = pICommandAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,
			iParamCnt,rgParamBindings,dwOffset,&hParamAccessor, pBindingStatus);
		GRS_SAFEFREE(pBindingStatus);

		GRS_COM_CHECK(hr,_T("��������������ʧ�ܣ������룺0x%08X\n"),hr);
		
		//׼������
		dbParam.cParamSets = iParamCnt;
		dbParam.hAccessor = hParamAccessor;
		dbParam.pData = GRS_CALLOC(dwOffset);
		for(ULONG i = 0; i < iParamCnt; i++)
		{
			GRS_PRINTF(_T("���������[%ld]:"),i);
			ReadConsole(GetStdHandle(STD_INPUT_HANDLE)
				,(BYTE*)dbParam.pData + rgParamBindings[i].obValue
				,rgParamBindings[i].cbMaxLen / sizeof(WCHAR)
				,&dwReadChars
				,NULL);
			//��¼ʵ������Ĳ������ȣ�����ط�û��У����ʵĲ������ȣ�ʵ�ʴ�������Ҫ
			//����׼���������ݵĻ���Ȼ���ٸ�������ĳ��Ⱥ��������������Ҫ��У��ʹ���
			*((ULONG*)((BYTE*)dbParam.pData+rgParamBindings[i].obLength)) = dwReadChars;
		}
		//_tcscpy((TCHAR*)dbParam.pData,_T("11"));
	}
	hr = pICommandText->Execute(NULL,IID_IRowset,&dbParam,NULL,(IUnknown**)&pIRowset);
	GRS_COM_CHECK(hr,_T("ִ��SQL���ʧ�ܣ������룺0x%08X\n"),hr);
	
	//׼������Ϣ
	hr = pIRowset->QueryInterface(IID_IColumnsInfo,(void**)&pIColumnsInfo);
	GRS_COM_CHECK(hr,_T("���IColumnsInfo�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);
	hr = pIColumnsInfo->GetColumnInfo(&cColumns, &rgColumnInfo,&pStringBuffer);
	GRS_COM_CHECK(hr,_T("��ȡ����Ϣʧ�ܣ������룺0x%08X\n"),hr);

	//����󶨽ṹ������ڴ�	
	rgBindings = (DBBINDING*)GRS_CALLOC(cColumns * sizeof(DBBINDING));
	//��������Ϣ ���󶨽ṹ��Ϣ���� ע���д�С����8�ֽڱ߽��������
	for( iCol = 0; iCol < cColumns; iCol++ )
	{
		rgBindings[iCol].iOrdinal   = rgColumnInfo[iCol].iOrdinal;
		rgBindings[iCol].dwPart     = DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS;
		rgBindings[iCol].obStatus   = dwOffset;
		rgBindings[iCol].obLength   = dwOffset + sizeof(DBSTATUS);
		rgBindings[iCol].obValue    = dwOffset+sizeof(DBSTATUS)+sizeof(ULONG);
		rgBindings[iCol].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
		rgBindings[iCol].eParamIO   = DBPARAMIO_NOTPARAM;
		rgBindings[iCol].bPrecision = rgColumnInfo[iCol].bPrecision;
		rgBindings[iCol].bScale     = rgColumnInfo[iCol].bScale;
		rgBindings[iCol].wType      = DBTYPE_WSTR;//rgColumnInfo[iCol].wType;	//ǿ��תΪWSTR
		rgBindings[iCol].cbMaxLen   = rgColumnInfo[iCol].ulColumnSize * sizeof(WCHAR);
		dwOffset = rgBindings[iCol].cbMaxLen + rgBindings[iCol].obValue;
		dwOffset = GRS_ROUNDUP(dwOffset);
	}
	plDisplayLen = (ULONG*)GRS_CALLOC(cColumns * sizeof(ULONG));

	//�����з�����
	hr = pIRowset->QueryInterface(IID_IAccessor,(void**)&pIAccessor);
	GRS_COM_CHECK(hr,_T("���IAccessor�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);
	hr = pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,cColumns,rgBindings,0,&hAccessor,NULL);
	GRS_COM_CHECK(hr,_T("�����з�����ʧ�ܣ������룺0x%08X\n"),hr);

	//��ȡ����
	//����cRows�����ݵĻ��壬Ȼ�󷴸���ȡcRows�е����Ȼ�����д���֮
	pData = GRS_CALLOC(dwOffset * cRows);
	lAllRows = 0;

	while( S_OK == hr )    
	{//ѭ����ȡ���ݣ�ÿ��ѭ��Ĭ�϶�ȡcRows�У�ʵ�ʶ�ȡ��cRowsObtained��
		hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER,0,cRows,&cRowsObtained, &rghRows);

		//ÿ������ʾһ������ʱ������0һ�¿������
		ZeroMemory(plDisplayLen,cColumns * sizeof(ULONG));
		for( iRow = 0; iRow < cRowsObtained; iRow++ )
		{
			//����ÿ��������ʾ���
			pCurData    = (BYTE*)pData + (dwOffset * iRow);
			pIRowset->GetData(rghRows[iRow],hAccessor,pCurData);
			UpdateDisplaySize(cColumns,rgBindings,pCurData,plDisplayLen);
		}

		if(!bShowColumnName)
		{//��ʾ�ֶ���Ϣ
			DisplayColumnNames(cColumns,rgColumnInfo,plDisplayLen);
			bShowColumnName = TRUE;
		}

		for( iRow = 0; iRow < cRowsObtained; iRow++ )
		{
			//��ʾ����
			pCurData    = (BYTE*)pData + (dwOffset * iRow);
			DisplayRow(++lAllRows,cColumns,rgBindings,pCurData,plDisplayLen);
		}

		if( cRowsObtained )
		{//�ͷ��о������
			pIRowset->ReleaseRows(cRowsObtained,rghRows,NULL,NULL,NULL);
		}
		bShowColumnName = FALSE;
		CoTaskMemFree(rghRows);
		rghRows = NULL;
	}

CLEAR_UP:

	if(hAccessor && pIAccessor)
	{
		pIAccessor->ReleaseAccessor(hAccessor,NULL);
	}

	if(hParamAccessor && pICommandAccessor)
	{
		pICommandAccessor->ReleaseAccessor(hParamAccessor,NULL);
	}
	GRS_SAFEFREE(dbParam.pData);
	GRS_SAFEFREE(pData);
	GRS_SAFEFREE(rgBindings);
	GRS_SAFEFREE(plDisplayLen);
	CoTaskMemFree(pStringBufferParam);
	CoTaskMemFree(rgColumnInfo);
	CoTaskMemFree(pStringBuffer);
	GRS_SAFERELEASE(pIRowset);
	GRS_SAFERELEASE(pIAccessor);
	GRS_SAFERELEASE(pIColumnsInfo);
	GRS_SAFERELEASE(pICommandPrepare);
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
	
	IDataInitialize*		pIDataInitialize		= NULL;
	IDBPromptInitialize*	pIDBPromptInitialize	= NULL;
	IDBInitialize*			pDBConnect				= NULL;
	IDBProperties*			pIDBProperties			= NULL;
	IDBCreateSession*		pIDBCreateSession		= NULL;

	HWND hWndParent = GetDesktopWindow();
	CLSID clsid = {};

	//2��ʹ�ô����뷽ʽ���ӵ�ָ��������Դ
	HRESULT hr = CoCreateInstance(CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER,
		IID_IDataInitialize,(void**)&pIDataInitialize);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("�޷�����IDataInitialize�ӿڣ������룺0x%08x\n"),hr);
		return FALSE;
	}

	hr = CLSIDFromProgID(_T("SQLOLEDB"), &clsid);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("��ȡ\"SQLOLEDB\"��CLSID���󣬴����룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		return FALSE;
	}
	
	//��������Դ���Ӷ���ͳ�ʼ���ӿ�
	hr = pIDataInitialize->CreateDBInstance(clsid, NULL,
		CLSCTX_INPROC_SERVER, NULL, IID_IDBInitialize,
		(IUnknown**)&pDBConnect);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("����IDataInitialize->CreateDBInstance����IDBInitialize�ӿڴ��󣬴����룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}
	
	//׼����������
	//ע����Ȼ����ֻʹ����n�����ԣ�������ȻҪ����ͳ�ʼ��n+1������
	//Ŀ���������������������һ���յ�������Ϊ�����β
	DBPROP InitProperties[5] = {};
	DBPROPSET rgInitPropSet[1] = {};
	//ָ�����ݿ�ʵ����������ʹ���˱���local��ָ������Ĭ��ʵ��
	InitProperties[0].dwPropertyID = DBPROP_INIT_DATASOURCE;
	InitProperties[0].vValue.vt = VT_BSTR;
	InitProperties[0].vValue.bstrVal= SysAllocString(L"ASUS-PC\\SQL2008");
	InitProperties[0].dwOptions = DBPROPOPTIONS_REQUIRED;
	InitProperties[0].colid = DB_NULLID;
	//ָ�����ݿ���
	InitProperties[1].dwPropertyID = DBPROP_INIT_CATALOG;
	InitProperties[1].vValue.vt = VT_BSTR;
	InitProperties[1].vValue.bstrVal = SysAllocString(L"Study");
	InitProperties[1].dwOptions = DBPROPOPTIONS_REQUIRED;
	InitProperties[1].colid = DB_NULLID;
	
 //   //ָ�������֤��ʽΪ���ɰ�ȫģʽ��SSPI��
	//InitProperties[2].dwPropertyID = DBPROP_AUTH_INTEGRATED;
	//InitProperties[2].vValue.vt = VT_BSTR;
	//InitProperties[2].vValue.bstrVal = SysAllocString(L"SSPI");
	//InitProperties[2].dwOptions = DBPROPOPTIONS_REQUIRED;
	//InitProperties[2].colid = DB_NULLID;

    // User ID
    InitProperties[2].dwPropertyID = DBPROP_AUTH_USERID;
    InitProperties[2].vValue.vt = VT_BSTR;
    InitProperties[2].vValue.bstrVal = SysAllocString(OLESTR("sa"));

    // Password
    InitProperties[3].dwPropertyID = DBPROP_AUTH_PASSWORD;
    InitProperties[3].vValue.vt = VT_BSTR;
    InitProperties[3].vValue.bstrVal = SysAllocString(OLESTR("999999"));

	//����һ��GUIDΪDBPROPSET_DBINIT�����Լ��ϣ���Ҳ�ǳ�ʼ������ʱ��Ҫ��Ψһһ//�����Լ���
	rgInitPropSet[0].guidPropertySet = DBPROPSET_DBINIT;
	rgInitPropSet[0].cProperties = 4;
	rgInitPropSet[0].rgProperties = InitProperties;

	//�õ����ݿ��ʼ�������Խӿ�
	hr = pDBConnect->QueryInterface(IID_IDBProperties, (void **)&pIDBProperties);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("��ȡIDBProperties�ӿ�ʧ�ܣ������룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}

	hr = pIDBProperties->SetProperties(1, rgInitPropSet); 
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("������������ʧ�ܣ������룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		GRS_SAFERELEASE(pDBConnect);
		GRS_SAFERELEASE(pIDBProperties);
		return FALSE;
	}
	//����һ��������ɣ���Ӧ�ĽӿھͿ����ͷ���
	GRS_SAFERELEASE(pIDBProperties);


	//����ָ�����������ӵ����ݿ�
	hr = pDBConnect->Initialize();
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("ʹ���������ӵ�����Դʧ�ܣ������룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}

	GRS_PRINTF(_T("ʹ�����Լ���ʽ���ӵ�����Դ�ɹ�!\n"));
	
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
	GRS_SAFERELEASE(pIDataInitialize);
	
	return TRUE;

	//1��ʹ��OLEDB���ݿ����ӶԻ������ӵ�����Դ
	hr = CoCreateInstance(CLSID_DataLinks, NULL, CLSCTX_INPROC_SERVER,
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


VOID DisplayColumnNames(DBORDINAL        cColumns
						,DBCOLUMNINFO*   rgColumnInfo
						,ULONG*          rgDispSize )
{
	GRS_USEPRINTF();
	WCHAR            wszColumn[MAX_DISPLAY_SIZE + 1];
	LPWSTR           pwszColName;
	DBORDINAL        iCol;
	ULONG            cSpaces;
	ULONG            iSpace;
	size_t			 szLen;

	// Display the title of the row index column.
	GRS_PRINTF(_T("   Row    | "));

	for( iCol = 0; iCol < cColumns; iCol++ ) 
	{
		pwszColName = rgColumnInfo[iCol].pwszName;
		if( !pwszColName ) 
		{
			if( !rgColumnInfo[iCol].iOrdinal )
			{
				pwszColName = _T("Bmk");
			}
			else
			{
				pwszColName = _T("(null)");
			}
		}

		StringCchCopy(wszColumn,MAX_DISPLAY_SIZE + 1, pwszColName);
		StringCchLength(wszColumn,MAX_DISPLAY_SIZE + 1,&szLen);
		rgDispSize[iCol] = max( min(rgDispSize[iCol], MAX_DISPLAY_SIZE + 1) , szLen);
		cSpaces = rgDispSize[iCol] - szLen;

		GRS_PRINTF(_T("%s"), wszColumn);
		for(iSpace = 0; iSpace < cSpaces; iSpace++ )
		{
			GRS_PRINTF(_T(" "));
		}
		if( iCol < cColumns - 1 )
		{
			GRS_PRINTF(_T(" | "));
		}
	}
	GRS_PRINTF(_T("\n"));
}

HRESULT DisplayRow(DBCOUNTITEM iRow,
				   DBCOUNTITEM cBindings,
				   DBBINDING * rgBindings,
				   void * pData,
				   ULONG * rgDispSize )
{
	GRS_USEPRINTF();
	HRESULT               hr = S_OK;
	WCHAR                 wszColumn[MAX_DISPLAY_SIZE + 1];
	DBSTATUS              dwStatus;
	DBLENGTH              ulLength;
	void *                pvValue;
	DBCOUNTITEM           iCol;
	ULONG                 cbRead;
	ISequentialStream *   pISeqStream = NULL;
	ULONG                 cSpaces;
	ULONG                 iSpace;
	size_t				  szLen;

	GRS_PRINTF(_T("[%8d]| "), iRow);
	for( iCol = 0; iCol < cBindings; iCol++ ) 
	{
		dwStatus   = *(DBSTATUS *)((BYTE *)pData + rgBindings[iCol].obStatus);
		ulLength   = *(DBLENGTH *)((BYTE *)pData + rgBindings[iCol].obLength);
		pvValue    = (BYTE *)pData + rgBindings[iCol].obValue;
		switch( dwStatus ) 
		{
		case DBSTATUS_S_ISNULL:
			StringCchCopy(wszColumn,MAX_DISPLAY_SIZE + 1, L"(null)");
			break;
		case DBSTATUS_S_TRUNCATED:
		case DBSTATUS_S_OK:
		case DBSTATUS_S_DEFAULT:
			{
				switch( rgBindings[iCol].wType )
				{
				case DBTYPE_WSTR:
					{   
						StringCchCopy(wszColumn,MAX_DISPLAY_SIZE + 1,(WCHAR*)pvValue);
						break;
					}
				case DBTYPE_IUNKNOWN:
					{
						pISeqStream = *(ISequentialStream**)pvValue;
						hr = pISeqStream->Read(
							wszColumn,                     
							MAX_DISPLAY_SIZE,              
							&cbRead                        
							);

						wszColumn[cbRead / sizeof(WCHAR)] = L'\0';
						pISeqStream->Release();
						pISeqStream = NULL;
						break;
					}
				}
				break;
			}
		default:
			StringCchCopy(wszColumn, MAX_DISPLAY_SIZE + 1, _T("(error status)"));
			break;
		}
		StringCchLength(wszColumn,MAX_DISPLAY_SIZE + 1,&szLen);
		cSpaces = min(rgDispSize[iCol], MAX_DISPLAY_SIZE + 1) - szLen;

		GRS_PRINTF(_T("%s"), wszColumn);

		for(iSpace = 0; iSpace < cSpaces; iSpace++ )
		{
			GRS_PRINTF(_T(" "));
		}


		if( iCol < cBindings - 1 )
		{
			GRS_PRINTF(_T(" | "));
		}
	}

	GRS_SAFERELEASE(pISeqStream )
		GRS_PRINTF(_T("\n"));
	return hr;
}

VOID UpdateDisplaySize (DBCOUNTITEM   cBindings,
						DBBINDING *   rgBindings,
						void *        pData,
						ULONG *       rgDispSize )
{
	DBSTATUS      dwStatus;
	ULONG         cchLength;
	DBCOUNTITEM   iCol;

	for( iCol = 0; iCol < cBindings; iCol++ )
	{
		dwStatus = *(DBSTATUS *)((BYTE *)pData + rgBindings[iCol].obStatus);
		cchLength = ((*(ULONG *)((BYTE *)pData + rgBindings[iCol].obLength))
			/ sizeof(WCHAR));
		switch( dwStatus )
		{
		case DBSTATUS_S_ISNULL:
			cchLength = 6;                              // "(null)"
			break;

		case DBSTATUS_S_TRUNCATED:
		case DBSTATUS_S_OK:
		case DBSTATUS_S_DEFAULT:
			if( rgBindings[iCol].wType == DBTYPE_IUNKNOWN )
				cchLength = 2 + 8;                      // "0x%08lx"

			cchLength = max(cchLength, MIN_DISPLAY_SIZE);
			break;

		default:
			cchLength = 14;                        // "(error status)"
			break;
		}

		if( rgDispSize[iCol] < cchLength )
		{
			rgDispSize[iCol] = cchLength;
		}
	}
}