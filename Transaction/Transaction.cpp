#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define COM_NO_WINDOWS_H    //如果已经包含了Windows.h或不使用其他Windows库函数时
#define DBINITCONSTANTS
#define INITGUID
#define OLEDBVER 0x0260		//OLEDB2.6引擎
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
    int                     iMIDMax                     = 5;    //插入iMIDMax行从表记录

	if(!CreateDBSession(pIOpenRowset))
	{
		goto CLEAR_UP;
	}

    hr = pIOpenRowset->QueryInterface(IID_ITransactionLocal,(void**)&pITransaction);
    GRS_COM_CHECK(hr,_T("获取ITransactionLocal接口失败，错误码：0x%08X\n"),hr);

	PropSet[0].guidPropertySet  = DBPROPSET_ROWSET;       //注意属性集合的名称
	PropSet[0].cProperties      = 3;							 //属性集中一共有两个属性
	PropSet[0].rgProperties     = Prop;

	//设置属性
    //设置属性
    Prop[0].dwPropertyID    = DBPROP_UPDATABILITY;
    Prop[0].vValue.vt       = VT_I4;
    //设置执行后的结果集可以进行增删改操作
    Prop[0].vValue.lVal     = DBPROPVAL_UP_CHANGE   //打开Update属性
            |DBPROPVAL_UP_DELETE					//打开Delete属性
            |DBPROPVAL_UP_INSERT;					//打开Insert属性
    Prop[0].dwOptions       = DBPROPOPTIONS_REQUIRED;
    Prop[0].colid = DB_NULLID;

    //打开IRowsetUpdate接口也就是支持延迟的更新特性
    Prop[1].dwPropertyID    = DBPROP_IRowsetUpdate;
    Prop[1].vValue.vt       = VT_BOOL;
    Prop[1].vValue.boolVal  = VARIANT_TRUE;         
    Prop[1].dwOptions       = DBPROPOPTIONS_REQUIRED;
    Prop[1].colid           = DB_NULLID;

    //以下这个属性也是要紧句，不设置那么就不能即更新删除又同时插入新行
    Prop[2].dwPropertyID    = DBPROP_CANHOLDROWS;
    Prop[2].vValue.vt       = VT_BOOL;
    Prop[2].vValue.boolVal  = VARIANT_TRUE;         
    Prop[2].dwOptions       = DBPROPOPTIONS_REQUIRED;
    Prop[2].colid           = DB_NULLID;

    //开始事务
    //注意使用ISOLATIONLEVEL_CURSORSTABILITY表示最终Commint以后,才能读取这两个表的数据
    hr = pITransaction->StartTransaction(ISOLATIONLEVEL_CURSORSTABILITY,0,NULL,NULL);

    //获取主表主键的最大值
    pRetData = RunSqlGetValue(pIOpenRowset,_T("Select Max(K_PID) As PMax From T_Primary"));
    if(NULL == pRetData)
    {
        goto CLEAR_UP;
    }
    iPID = *(int*)((BYTE*)pRetData + sizeof(DBSTATUS) + sizeof(ULONG));

    //最大值总是加1,这样即使取得的是空值,起始值也是正常的1
    ++iPID;

    TableID.eKind           = DBKIND_NAME;
    TableID.uName.pwszName  = (LPOLESTR)pszPrimaryTable;

	hr = pIOpenRowset->OpenRowset(NULL,&TableID
        ,NULL,IID_IRowsetChange,1,PropSet,(IUnknown**)&pIRowsetChange);
	GRS_COM_CHECK(hr,_T("打开表对象'%s'失败，错误码：0x%08X\n"),pszPrimaryTable,hr);

    ulChangeOffset = CreateAccessor(pIRowsetChange,pIAccessor,hChangeAccessor,pChangeBindings,ulRealCols);

    if(0 == ulChangeOffset
        || NULL == hChangeAccessor 
        || NULL == pIAccessor
        || NULL == pChangeBindings
        || 0 == ulRealCols)
    {
        goto CLEAR_UP;
    }
    //分配一个新行数据 设置数据后 插入
    pbNewData = (BYTE*)GRS_CALLOC(ulChangeOffset);

    //设置第一个字段 K_PID
    *(DBLENGTH *)((BYTE *)pbNewData + pChangeBindings[0].obLength) = sizeof(int);
    *(int*) (pbNewData + pChangeBindings[0].obValue) = iPID;
 
    //设置第二个字段 F_MValue
    *(DBLENGTH *)((BYTE *)pbNewData + pChangeBindings[1].obLength) = 8;
    StringCchCopy((WCHAR*) (pbNewData + pChangeBindings[1].obValue)
        ,pChangeBindings[1].cbMaxLen/sizeof(WCHAR),_T("主表数据"));

    //插入新数据
    hr = pIRowsetChange->InsertRow(NULL,hChangeAccessor,pbNewData,NULL);
    GRS_COM_CHECK(hr,_T("调用InsertRow插入新行失败，错误码：0x%08X\n"),hr);

    hr = pIRowsetChange->QueryInterface(IID_IRowsetUpdate,(void**)&pIRowsetUpdate);
    GRS_COM_CHECK(hr,_T("获取IRowsetUpdate接口失败，错误码：0x%08X\n"),hr);

    hr = pIRowsetUpdate->Update(NULL,0,NULL,NULL,NULL,NULL);
    GRS_COM_CHECK(hr,_T("调用Update提交更新失败，错误码：0x%08X\n"),hr);

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

    //插入第二个也就是从表的数据
    TableID.eKind           = DBKIND_NAME;
    TableID.uName.pwszName  = (LPOLESTR)pszMinorTable;

    hr = pIOpenRowset->OpenRowset(NULL,&TableID
        ,NULL,IID_IRowsetChange,1,PropSet,(IUnknown**)&pIRowsetChange);
    GRS_COM_CHECK(hr,_T("打开表对象'%s'失败，错误码：0x%08X\n"),pszMinorTable,hr);

    ulChangeOffset = CreateAccessor(pIRowsetChange,pIAccessor,hChangeAccessor,pChangeBindings,ulRealCols);

    if(0 == ulChangeOffset
        || NULL == hChangeAccessor 
        || NULL == pIAccessor
        || NULL == pChangeBindings
        || 0 == ulRealCols)
    {
        goto CLEAR_UP;
    }

    //分配一个新行数据 设置数据后 插入
    pbNewData = (BYTE*)GRS_CALLOC(ulChangeOffset);

    //设置第一个字段 K_MID
    *(DBLENGTH *)((BYTE *)pbNewData + pChangeBindings[0].obLength) = sizeof(int);
    //设置第二个字段 K_PID
    *(DBLENGTH *)((BYTE *)pbNewData + pChangeBindings[1].obLength) = sizeof(int);
    *(int*) (pbNewData + pChangeBindings[1].obValue) = iPID;

    //设置第二个字段
    *(DBLENGTH *)((BYTE *)pbNewData + pChangeBindings[2].obLength) = 8;
    StringCchCopy((WCHAR*) (pbNewData + pChangeBindings[2].obValue)
        ,pChangeBindings[2].cbMaxLen/sizeof(WCHAR),_T("从表数据"));

    for(int i = iMIDS; i <= iMIDMax; i++)
    {//循环插入新数据
        //设置第一个字段 K_MID
        *(int*) (pbNewData + pChangeBindings[0].obValue) = i;

        hr = pIRowsetChange->InsertRow(NULL,hChangeAccessor,pbNewData,NULL);
        GRS_COM_CHECK(hr,_T("调用InsertRow插入新行失败，错误码：0x%08X\n"),hr);
    }

    hr = pIRowsetChange->QueryInterface(IID_IRowsetUpdate,(void**)&pIRowsetUpdate);
    GRS_COM_CHECK(hr,_T("获取IRowsetUpdate接口失败，错误码：0x%08X\n"),hr);

    hr = pIRowsetUpdate->Update(NULL,0,NULL,NULL,NULL,NULL);
    GRS_COM_CHECK(hr,_T("调用Update提交更新失败，错误码：0x%08X\n"),hr);

    //所有操作都成功了,提交事务释放资源
    hr = pITransaction->Commit(FALSE, XACTTC_SYNC, 0);
    GRS_COM_CHECK(hr,_T("事务提交失败，错误码：0x%08X\n"),hr);

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

    GRS_PRINTF(_T("主从表数据插入成功!\n"));
    _tsystem(_T("PAUSE"));
    return 0;

CLEAR_UP:
    //操作失败,回滚事务先,然后释放资源
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
    GRS_COM_CHECK(hr,_T("获取ITransactionLocal接口失败，错误码：0x%08X\n"),hr);

    hr = pIColumnsInfo->GetColumnInfo(&ulCols,&pColInfo,&pStringBuffer);
    GRS_COM_CHECK(hr,_T("获取列信息失败，错误码：0x%08X\n"),hr);

    pBinding = (DBBINDING*)GRS_CALLOC(ulCols*sizeof(DBBINDING));

    for(iCol = 0; iCol < ulCols;iCol++)
    {
        if( 0 == pColInfo[iCol].iOrdinal )
        {//跳过bookmark
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
    GRS_COM_CHECK(hr,_T("获取IAccessor接口失败，错误码：0x%08X\n"),hr);

    hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA,iBinding
        ,pBinding,ulChangeOffset,&hAccessor,NULL);
    GRS_COM_CHECK(hr,_T("创建行更新访问器失败，错误码：0x%08X\n"),hr);
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
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: 获取IDBCreateCommand接口失败，错误码：0x%08X\n"),hr);

    hr = pICreateCommand->CreateCommand(NULL,IID_ICommandText,(IUnknown**)&pICommandText);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: 创建ICommandText接口失败，错误码：0x%08X\n"),hr);

    hr = pICommandText->SetCommandText(DBGUID_DEFAULT,pszSQL);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: 设置SQL语句[%s]失败，错误码：0x%08X\n"),pszSQL,hr);

    hr = pICommandText->QueryInterface(IID_ICommand,(void**)&pICommand);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: 获取ICommand接口失败，错误码：0x%08X\n"),hr);

    hr = pICommand->Execute(NULL,IID_IRowset,NULL,NULL,(IUnknown**)&pIRowset);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: 执行SQL语句失败，错误码：0x%08X\n"),hr);
    
    hr = pIRowset->QueryInterface(IID_IColumnsInfo,(void**)&pIColumnsInfo);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: 获取IColumnsInfo接口失败，错误码：0x%08X\n"),hr);

    hr = pIColumnsInfo->GetColumnInfo(&ulCols,&pColInfo,&pStringBuffer);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: 获取结果集列信息失败，错误码：0x%08X\n"),hr);

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
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: 获取IAccessor接口失败，错误码：0x%08X\n"),hr);

    hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA,ulCols,pBinding,ulOffset,&hAccessor,NULL);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: RunSqlGetValue: 创建行更新访问器失败，错误码：0x%08X\n"),hr);

    hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER,0,1,&pRowsObtained,&phRows);
    GRS_COM_CHECK(hr,_T("RunSqlGetValue: 获取行数据失败，错误码：0x%08X\n"),hr);

    pRetData = GRS_CALLOC(ulOffset);
    hr = pIRowset->GetData(phRows[0],hAccessor,pRetData);
    if(FAILED(hr))
    {
        GRS_PRINTF(_T("RunSqlGetValue: 获取查询结果数据失败,错误码:0x%08X\n"),hr);
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

	//2、使用纯代码方式连接到指定的数据源
	HRESULT hr = CoCreateInstance(CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER,
		IID_IDataInitialize,(void**)&pIDataInitialize);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("无法创建IDataInitialize接口，错误码：0x%08x\n"),hr);
		return FALSE;
	}

	hr = CLSIDFromProgID(_T("SQLOLEDB"), &clsid);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("获取\"SQLOLEDB\"的CLSID错误，错误码：0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		return FALSE;
	}

	//创建数据源连接对象和初始化接口
	hr = pIDataInitialize->CreateDBInstance(clsid, NULL,
		CLSCTX_INPROC_SERVER, NULL, IID_IDBInitialize,
		(IUnknown**)&pDBConnect);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("调用IDataInitialize->CreateDBInstance创建IDBInitialize接口错误，错误码：0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}

	//准备连接属性
	//注意虽然后面只使用了n个属性，但是任然要定义和初始化n+1个属性
	//目的是让属性数组最后总有一个空的属性作为数组结尾
	DBPROP InitProperties[5] = {};
	DBPROPSET rgInitPropSet[1] = {};
	//指定数据库实例名，这里使用了别名local，指定本地默认实例
	InitProperties[0].dwPropertyID = DBPROP_INIT_DATASOURCE;
	InitProperties[0].vValue.vt = VT_BSTR;
	InitProperties[0].vValue.bstrVal= SysAllocString(L"ASUS-PC\\SQL2008");
	InitProperties[0].dwOptions = DBPROPOPTIONS_REQUIRED;
	InitProperties[0].colid = DB_NULLID;
	//指定数据库名
	InitProperties[1].dwPropertyID = DBPROP_INIT_CATALOG;
	InitProperties[1].vValue.vt = VT_BSTR;
	InitProperties[1].vValue.bstrVal = SysAllocString(L"Study");
	InitProperties[1].dwOptions = DBPROPOPTIONS_REQUIRED;
	InitProperties[1].colid = DB_NULLID;

	////指定身份验证方式为集成安全模式“SSPI”
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

	//创建一个GUID为DBPROPSET_DBINIT的属性集合，这也是初始化连接时需要的唯一一//个属性集合
	rgInitPropSet[0].guidPropertySet = DBPROPSET_DBINIT;
	rgInitPropSet[0].cProperties = 4;
	rgInitPropSet[0].rgProperties = InitProperties;

	//得到数据库初始化的属性接口
	hr = pDBConnect->QueryInterface(IID_IDBProperties, (void **)&pIDBProperties);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("获取IDBProperties接口失败，错误码：0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}

	hr = pIDBProperties->SetProperties(1, rgInitPropSet); 
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("设置连接属性失败，错误码：0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		GRS_SAFERELEASE(pDBConnect);
		GRS_SAFERELEASE(pIDBProperties);
		return FALSE;
	}
	//属性一但设置完成，相应的接口就可以释放了
	GRS_SAFERELEASE(pIDBProperties);


	//根据指定的属性连接到数据库
	hr = pDBConnect->Initialize();
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("使用属性连接到数据源失败，错误码：0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}

	GRS_PRINTF(_T("使用属性集方式连接到数据源成功!\n"));

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
	GRS_SAFERELEASE(pIDataInitialize);

	return TRUE;

	//1、使用OLEDB数据库连接对话框连接到数据源
	hr = CoCreateInstance(CLSID_DataLinks, NULL, CLSCTX_INPROC_SERVER,
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
