#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define COM_NO_WINDOWS_H    //����Ѿ�������Windows.h��ʹ������Windows�⺯��ʱ
#define DBINITCONSTANTS
#define INITGUID
#define OLEDBVER 0x0260		//OLEDB2.6����
#include <oledb.h>
#include <oledberr.h>
#include <msdasc.h>
#include <msdaguid.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_SAFERELEASE(I) if(NULL != I){I->Release();I=NULL;}
#define GRS_COM_CHECK(hr,...) if(FAILED(hr)){GRS_PRINTF(__VA_ARGS__);goto CLEAR_UP;}

#define GRS_ROUNDUP_AMOUNT 8 
#define GRS_ROUNDUP_(size,amount) (((ULONG)(size)+((amount)-1))&~((amount)-1)) 
#define GRS_ROUNDUP(size) GRS_ROUNDUP_(size, GRS_ROUNDUP_AMOUNT) 

#define MAX_DISPLAY_SIZE	40
#define MIN_DISPLAY_SIZE    3

BOOL CreateDBSession(IOpenRowset*&pIOpenRowset);
void* RunSqlGetValue(IUnknown*pIUnknown,LPCTSTR pszSQL);
ULONG CreateAccessor(IUnknown*pIUnknown,IAccessor*&pIAccessor
                     ,HACCESSOR&hAccessor,DBBINDING*&pBinding,ULONG& ulCols);
int _tmain()
{
	GRS_USEPRINTF();
	CoInitialize(NULL);
	HRESULT					hr							= S_OK;
	IOpenRowset*			pIOpenRowset				= NULL;
    ITransactionLocal*      pITransaction               = NULL;
    IRowsetChange*          pIRowsetChange              = NULL;
    IRowsetUpdate*          pIRowsetUpdate              = NULL;
    IAccessor*              pIAccessor                  = NULL;

	TCHAR*					pszPrimaryTable				= _T("T_Primary");
    TCHAR*                  pszMinorTable               = _T("T_Minor");
	DBID					TableID			    = {};
	DBPROPSET				PropSet[1]					= {};
	DBPROP					Prop[10]					= {};

    HACCESSOR			    hChangeAccessor		        = NULL;
    DBBINDING*			    pChangeBindings             = NULL;
    ULONG				    ulRealCols			        = 0;
    ULONG                   ulChangeOffset              = 0;

    BYTE*                   pbNewData                   = NULL;

    VOID*                   pRetData                    = NULL;
    int                     iPID                        = 1;    //PrimaryID 
    int                     iMIDS                       = 1;    //Minor ID Start
    int                     iMIDMax                     = 5;    //����iMIDMax�дӱ��¼

	if(!CreateDBSession(pIOpenRowset))
	{
		goto CLEAR_UP;
	}

    hr = pIOpenRowset->QueryInterface(IID_ITransactionLocal,(void**)&pITransaction);
    GRS_COM_CHECK(hr,_T("��ȡITransactionLocal�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

	PropSet[0].guidPropertySet  = DBPROPSET_ROWSET;       //ע�����Լ��ϵ�����
	PropSet[0].cProperties      = 3;							 //���Լ���һ������������
	PropSet[0].rgProperties     = Prop;

	//��������
    //��������
    Prop[0].dwPropertyID    = DBPROP_UPDATABILITY;
    Prop[0].vValue.vt       = VT_I4;
    //����ִ�к�Ľ�������Խ�����ɾ�Ĳ���
    Prop[0].vValue.lVal     = DBPROPVAL_UP_CHANGE   //��Update����
            |DBPROPVAL_UP_DELETE					//��Delete����
            |DBPROPVAL_UP_INSERT;					//��Insert����
    Prop[0].dwOptions       = DBPROPOPTIONS_REQUIRED;
    Prop[0].colid = DB_NULLID;

    //��IRowsetUpdate�ӿ�Ҳ����֧���ӳٵĸ�������
    Prop[1].dwPropertyID    = DBPROP_IRowsetUpdate;
    Prop[1].vValue.vt       = VT_BOOL;
    Prop[1].vValue.boolVal  = VARIANT_TRUE;         
    Prop[1].dwOptions       = DBPROPOPTIONS_REQUIRED;
    Prop[1].colid           = DB_NULLID;

    //�����������Ҳ��Ҫ���䣬��������ô�Ͳ��ܼ�����ɾ����ͬʱ��������
    Prop[2].dwPropertyID    = DBPROP_CANHOLDROWS;
    Prop[2].vValue.vt       = VT_BOOL;
    Prop[2].vValue.boolVal  = VARIANT_TRUE;         
    Prop[2].dwOptions       = DBPROPOPTIONS_REQUIRED;
    Prop[2].colid           = DB_NULLID;

    //��ʼ����
    //ע��ʹ��ISOLATIONLEVEL_CURSORSTABILITY��ʾ����Commint�Ժ�,���ܶ�ȡ�������������
    hr = pITransaction->StartTransaction(ISOLATIONLEVEL_CURSORSTABILITY,0,NULL,NULL);

    //��ȡ�������������ֵ
    pRetData = RunSqlGetValue(pIOpenRowset,_T("Select Max(K_PID) As PMax From T_Primary"));
    if(NULL == pRetData)
    {
        goto CLEAR_UP;
    }
    iPID = *(int*)((BYTE*)pRetData + sizeof(DBSTATUS) + sizeof(ULONG));

    //���ֵ���Ǽ�1,������ʹȡ�õ��ǿ�ֵ,��ʼֵҲ��������1
    ++iPID;

    TableID.eKind           = DBKIND_NAME;
    TableID.uName.pwszName  = (LPOLESTR)pszPrimaryTable;

	hr = pIOpenRowset->OpenRowset(NULL,&TableID
        ,NULL,IID_IRowsetChange,1,PropSet,(IUnknown**)&pIRowsetChange);
	GRS_COM_CHECK(hr,_T("�򿪱����'%s'ʧ�ܣ������룺0x%08X\n"),pszPrimaryTable,hr);

    ulChangeOffset = CreateAccessor(pIRowsetChange,pIAccessor,hChangeAccessor,pChangeBindings,ulRealCols);

    if(0 == ulChangeOffset
        || NULL == hChangeAccessor 
        || NULL == pIAccessor
        || NULL == pChangeBindings
        || 0 == ulRealCols)
    {
        goto CLEAR_UP;
    }
    //����һ���������� �������ݺ� ����
    pbNewData = (BYTE*)GRS_CALLOC(ulChangeOffset);

    //���õ�һ���ֶ� K_PID
    *(DBLENGTH *)((BYTE *)pbNewData + pChangeBindings[0].obLength) = sizeof(int);
    *(int*) (pbNewData + pChangeBindings[0].obValue) = iPID;
 
    //���õڶ����ֶ� F_MValue
    *(DBLENGTH *)((BYTE *)pbNewData + pChangeBindings[1].obLength) = 8;
    StringCchCopy((WCHAR*) (pbNewData + pChangeBindings[1].obValue)
        ,pChangeBindings[1].cbMaxLen/sizeof(WCHAR),_T("��������"));

    //����������
    hr = pIRowsetChange->InsertRow(NULL,hChangeAccessor,pbNewData,NULL);
    GRS_COM_CHECK(hr,_T("����InsertRow��������ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pIRowsetChange->QueryInterface(IID_IRowsetUpdate,(void**)&pIRowsetUpdate);
    GRS_COM_CHECK(hr,_T("��ȡIRowsetUpdate�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pIRowsetUpdate->Update(NULL,0,NULL,NULL,NULL,NULL);
    GRS_COM_CHECK(hr,_T("����Update�ύ����ʧ�ܣ������룺0x%08X\n"),hr);

    GRS_SAFEFREE(pChangeBindings);
    GRS_SAFEFREE(pRetData);
    GRS_SAFEFREE(pbNewData);
    if(NULL != hChangeAccessor && NULL != pIAccessor)
    {
        pIAccessor->ReleaseAccessor(hChangeAccessor,NULL);
        hChangeAccessor = NULL;
    }
    GRS_SAFERELEASE(pIAccessor);
    GRS_SAFERELEASE(pIRowsetChange);
    GRS_SAFERELEASE(pIRowsetUpdate);

    //����ڶ���Ҳ���Ǵӱ������
    TableID.eKind           = DBKIND_NAME;
    TableID.uName.pwszName  = (LPOLESTR)pszMinorTable;

    hr = pIOpenRowset->OpenRowset(NULL,&TableID
        ,NULL,IID_IRowsetChange,1,PropSet,(IUnknown**)&pIRowsetChange);
    GRS_COM_CHECK(hr,_T("�򿪱����'%s'ʧ�ܣ������룺0x%08X\n"),pszMinorTable,hr);

    ulChangeOffset = CreateAccessor(pIRowsetChange,pIAccessor,hChangeAccessor,pChangeBindings,ulRealCols);

    if(0 == ulChangeOffset
        || NULL == hChangeAccessor 
        || NULL == pIAccessor
        || NULL == pChangeBindings
        || 0 == ulRealCols)
    {
        goto CLEAR_UP;
    }

    //����һ���������� �������ݺ� ����
    pbNewData = (BYTE*)GRS_CALLOC(ulChangeOffset);

    //���õ�һ���ֶ� K_MID
    *(DBLENGTH *)((BYTE *)pbNewData + pChangeBindings[0].obLength) = sizeof(int);
    //���õڶ����ֶ� K_PID
    *(DBLENGTH *)((BYTE *)pbNewData + pChangeBindings[1].obLength) = sizeof(int);
    *(int*) (pbNewData + pChangeBindings[1].obValue) = iPID;

    //���õڶ����ֶ�
    *(DBLENGTH *)((BYTE *)pbNewData + pChangeBindings[2].obLength) = 8;
    StringCchCopy((WCHAR*) (pbNewData + pChangeBindings[2].obValue)
        ,pChangeBindings[2].cbMaxLen/sizeof(WCHAR),_T("�ӱ�����"));

    for(int i = iMIDS; i <= iMIDMax; i++)
    {//ѭ������������
        //���õ�һ���ֶ� K_MID
        *(int*) (pbNewData + pChangeBindings[0].obValue) = i;

        hr = pIRowsetChange->InsertRow(NULL,hChangeAccessor,pbNewData,NULL);
        GRS_COM_CHECK(hr,_T("����InsertRow��������ʧ�ܣ������룺0x%08X\n"),hr);
    }

    hr = pIRowsetChange->QueryInterface(IID_IRowsetUpdate,(void**)&pIRowsetUpdate);
    GRS_COM_CHECK(hr,_T("��ȡIRowsetUpdate�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pIRowsetUpdate->Update(NULL,0,NULL,NULL,NULL,NULL);
    GRS_COM_CHECK(hr,_T("����Update�ύ����ʧ�ܣ������룺0x%08X\n"),hr);

    //���в������ɹ���,�ύ�����ͷ���Դ
    hr = pITransaction->Commit(FALSE, XACTTC_SYNC, 0);
    GRS_COM_CHECK(hr,_T("�����ύʧ�ܣ������룺0x%08X\n"),hr);

    GRS_SAFEFREE(pChangeBindings);
    GRS_SAFEFREE(pRetData);
    GRS_SAFEFREE(pbNewData);

    if(NULL != hChangeAccessor && NULL != pIAccessor)
    {
        pIAccessor->ReleaseAccessor(hChangeAccessor,NULL);
        hChangeAccessor = NULL;
    }
    GRS_SAFERELEASE(pIAccessor);
    GRS_SAFERELEASE(pIRowsetChange);
    GRS_SAFERELEASE(pIRowsetUpdate);
    GRS_SAFERELEASE(pIOpenRowset);
    GRS_SAFERELEASE(pITransaction);

    GRS_PRINTF(_T("���ӱ����ݲ���ɹ�!\n"));
    _tsystem(_T("PAUSE"));
    return 0;

CLEAR_UP:
    //����ʧ��,�ع�������,Ȼ���ͷ���Դ
    hr = pITransaction->Abort(NULL, FALSE, FALSE);

    GRS_SAFEFREE(pRetData);
    GRS_SAFEFREE(pbNewData);
    if(NULL != hChangeAccessor && NULL != pIAccessor)
    {
        pIAccessor->ReleaseAccessor(hChangeAccessor,NULL);
        hChangeAccessor = NULL;
    }
    GRS_SAFERELEASE(pIAccessor);
    GRS_SAFERELEASE(pIRowsetChange);
    GRS_SAFERELEASE(pIRowsetUpdate);

    GRS_SAFERELEASE(pITransaction);
	GRS_SAFERELEASE(pIOpenRowset);

	_tsystem(_T("PAUSE"));
	CoUninitialize();
	return 0;
}

ULONG CreateAccessor(IUnknown*pIUnknown,IAccessor*&pIAccessor,HACCESSOR&hAccessor,DBBINDING*&pBinding,ULONG& ulCols)
{
    GRS_USEPRINTF();
    HRESULT hr = S_OK;

    IColumnsInfo*   pIColumnsInfo       = NULL;
    
    DBCOLUMNINFO*   pColInfo            = NULL;
    LPWSTR          pStringBuffer       = NULL;
    ULONG           iCol                = 0;
    ULONG           iBinding            = 0;
    ULONG           ulChangeOffset      = 0;

    hr = pIUnknown->QueryInterface(IID_IColumnsInfo,(void**)&pIColumnsInfo);
    GRS_COM_CHECK(hr,_T("��ȡITransactionLocal�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pIColumnsInfo->GetColumnInfo(&ulCols,&pColInfo,&pStringBuffer);
    GRS_COM_CHECK(hr,_T("��ȡ����Ϣʧ�ܣ������룺0x%08X\n"),hr);

    pBinding = (DBBINDING*)GRS_CALLOC(ulCols*sizeof(DBBINDING));

    for(iCol = 0; iCol < ulCols;iCol++)
    {
        if( 0 == pColInfo[iCol].iOrdinal )
        {//����bookmark
            continue;
        }
        pBinding[iBinding].iOrdinal   = pColInfo[iCol].iOrdinal;
        pBinding[iBinding].dwPart     = DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS;
        pBinding[iBinding].obStatus   = ulChangeOffset;
        pBinding[iBinding].obLength   = ulChangeOffset + sizeof(DBSTATUS);
        pBinding[iBinding].obValue    = ulChangeOffset + sizeof(DBSTATUS) + sizeof(ULONG);
        pBinding[iBinding].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
        pBinding[iBinding].eParamIO   = DBPARAMIO_NOTPARAM;
        pBinding[iBinding].bPrecision = pColInfo[iCol].bPrecision;
        pBinding[iBinding].bScale     = pColInfo[iCol].bScale;
        pBinding[iBinding].wType      = pColInfo[iCol].wType;
        pBinding[iBinding].cbMaxLen   = pColInfo[iCol].ulColumnSize * sizeof(WCHAR);

        ulChangeOffset = pBinding[iBinding].cbMaxLen + pBinding[iBinding].obValue;
        ulChangeOffset = GRS_ROUNDUP(ulChangeOffset);
        ++ iBinding;
    }

    hr = pIUnknown->QueryInterface(IID_IAccessor,(void**)&pIAccessor);
    GRS_COM_CHECK(hr,_T("��ȡIAccessor�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA,iBinding
        ,pBinding,ulChangeOffset,&hAccessor,NULL);
    GRS_COM_CHECK(hr,_T("�����и��·�����ʧ�ܣ������룺0x%08X\n"),hr);
CLEAR_UP:
    CoTaskMemFree(pColInfo);
    CoTaskMemFree(pStringBuffer);
    GRS_SAFERELEASE(pIColumnsInfo);

    return ulChangeOffset;
}

void* RunSqlGetValue(IUnknown*pIUnknown,LPCTSTR pszSQL)
{
    GRS_USEPRINTF();
    HRESULT             hr                      = S_OK;
    VOID*               pRetData                = NULL;

    IDBCreateCommand*   pICreateCommand         = NULL;
    ICommand*           pICommand               = NULL;
    ICommandText*       pICommandText           = NULL;
    IRowset*            pIRowset                = NULL;
    IColumnsInfo*       pIColumnsInfo           = NULL;
    IAccessor*          pIAccessor              = NULL;

    ULONG               ulCols                  = NULL;
    DBBINDING*          pBinding                = NULL;
    DBCOLUMNINFO*		pColInfo			    = NULL;
    LPWSTR              pStringBuffer           = NULL;
    ULONG               iCol                    = NULL;
    ULONG               ulOffset                = NULL;
    HACCESSOR           hAccessor               = NULL;

    ULONG               pRowsObtained		    = 0;
    HROW *              phRows				    = NULL;

    hr = pIUnknown->QueryInterface(IID_IDBCreateCommand,(void**)&pICreateCommand);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: ��ȡIDBCreateCommand�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pICreateCommand->CreateCommand(NULL,IID_ICommandText,(IUnknown**)&pICommandText);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: ����ICommandText�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pICommandText->SetCommandText(DBGUID_DEFAULT,pszSQL);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: ����SQL���[%s]ʧ�ܣ������룺0x%08X\n"),pszSQL,hr);

    hr = pICommandText->QueryInterface(IID_ICommand,(void**)&pICommand);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: ��ȡICommand�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pICommand->Execute(NULL,IID_IRowset,NULL,NULL,(IUnknown**)&pIRowset);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: ִ��SQL���ʧ�ܣ������룺0x%08X\n"),hr);
    
    hr = pIRowset->QueryInterface(IID_IColumnsInfo,(void**)&pIColumnsInfo);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: ��ȡIColumnsInfo�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pIColumnsInfo->GetColumnInfo(&ulCols,&pColInfo,&pStringBuffer);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: ��ȡ���������Ϣʧ�ܣ������룺0x%08X\n"),hr);

    pBinding = (DBBINDING*)GRS_CALLOC(ulCols*sizeof(DBBINDING));
    for( iCol = 0; iCol < ulCols; iCol++ )
    {
        pBinding[iCol].iOrdinal   = pColInfo[iCol].iOrdinal;
        pBinding[iCol].dwPart     = DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;
        pBinding[iCol].obStatus   = ulOffset;
        pBinding[iCol].obLength   = ulOffset + sizeof(DBSTATUS);
        pBinding[iCol].obValue    = ulOffset + sizeof(DBSTATUS) + sizeof(ULONG);
        pBinding[iCol].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
        pBinding[iCol].eParamIO   = DBPARAMIO_NOTPARAM;
        pBinding[iCol].bPrecision = pColInfo[iCol].bPrecision;
        pBinding[iCol].bScale     = pColInfo[iCol].bScale;
        pBinding[iCol].wType      = pColInfo[iCol].wType;
        pBinding[iCol].cbMaxLen   = pColInfo[iCol].ulColumnSize;
        ulOffset = pBinding[iCol].cbMaxLen + pBinding[iCol].obValue;
        ulOffset = GRS_ROUNDUP(ulOffset);
    }

    hr = pIRowset->QueryInterface(IID_IAccessor,(void**)&pIAccessor);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: ��ȡIAccessor�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA,ulCols,pBinding,ulOffset,&hAccessor,NULL);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: RunSqlGetValue: �����и��·�����ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER,0,1,&pRowsObtained,&phRows);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: ��ȡ������ʧ�ܣ������룺0x%08X\n"),hr);

    pRetData = GRS_CALLOC(ulOffset);
    hr = pIRowset->GetData(phRows[0],hAccessor,pRetData);
    if(FAILED(hr))
    {
        GRS_PRINTF(_T("RunSqlGetValue: ��ȡ��ѯ�������ʧ��,������:0x%08X\n"),hr);
        GRS_SAFEFREE(pRetData);
    }

CLEAR_UP:
    GRS_SAFEFREE(pBinding);
    if(NULL != phRows)
    {
        pIRowset->ReleaseRows(1,phRows,NULL,NULL,NULL);
        phRows = NULL;
    }
    if(NULL != pIAccessor && NULL != hAccessor)
    {
        pIAccessor->ReleaseAccessor(hAccessor,NULL);
        hAccessor = NULL;
    }
    CoTaskMemFree(pColInfo);
    CoTaskMemFree(pStringBuffer);

    GRS_SAFERELEASE(pIAccessor);
    GRS_SAFERELEASE(pIColumnsInfo);
    GRS_SAFERELEASE(pICommand);
    GRS_SAFERELEASE(pICommandText);
    GRS_SAFERELEASE(pIRowset);
    GRS_SAFERELEASE(pICreateCommand);

    return pRetData;
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

	////ָ�������֤��ʽΪ���ɰ�ȫģʽ��SSPI��
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
