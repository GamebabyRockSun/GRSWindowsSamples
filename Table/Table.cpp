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
VOID DisplayRowSet(IRowset* pIRowset);
VOID DisplayColumnNames(DBORDINAL        lColumns
						,DBCOLUMNINFO*   pColumnInfo
						,ULONG*          rgDispSize );
HRESULT DisplayRow(DBCOUNTITEM iRow,
				   DBCOUNTITEM cBindings,
				   DBBINDING * pBindings,
				   void * pData,
				   ULONG * rgDispSize );

VOID UpdateDisplaySize (DBCOUNTITEM   cBindings,
						DBBINDING *   pBindings,
						void *        pData,
						ULONG *       rgDispSize );

int _tmain()
{
	GRS_USEPRINTF();
	CoInitialize(NULL);
	HRESULT					hr							= S_OK;
	IOpenRowset*			pIOpenRowset				= NULL;
	IRowset*				pIRowset					= NULL;
    IRowsetChange*          pIRowsetChange              = NULL;
    IRowsetUpdate*          pIRowsetUpdate              = NULL;
	TCHAR*					pszTableName				= _T("T_State");
	DBID					TableID						= {};
	DBPROPSET				PropSet[1]					= {};
	DBPROP					Prop[4]						= {};

	if(!CreateDBSession(pIOpenRowset))
	{
		goto CLEAR_UP;
	}

	PropSet[0].guidPropertySet  = DBPROPSET_ROWSET;          //注意属性集合的名称
	PropSet[0].cProperties      = 3;					     //属性集中属性个数
	PropSet[0].rgProperties     = Prop;

	//设置属性
    Prop[0].dwPropertyID    = DBPROP_UPDATABILITY;
    Prop[0].vValue.vt       = VT_I4;
    //设置执行后的结果集可以进行增删改操作
    Prop[0].vValue.lVal     = DBPROPVAL_UP_CHANGE   //打开Update属性
        |DBPROPVAL_UP_DELETE				//打开Delete属性
        |DBPROPVAL_UP_INSERT;				//打开Insert属性
    Prop[0].dwOptions       = DBPROPOPTIONS_REQUIRED;
    Prop[0].colid           = DB_NULLID;

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

	TableID.eKind           = DBKIND_NAME;
	TableID.uName.pwszName  = (LPOLESTR)pszTableName;
	
	hr = pIOpenRowset->OpenRowset(NULL,&TableID,NULL,IID_IRowsetChange,1,PropSet,(IUnknown**)&pIRowsetChange);
	GRS_COM_CHECK(hr,_T("打开表对象'%s'失败，错误码：0x%08X\n"),pszTableName,hr);

    hr = pIRowsetChange->QueryInterface(IID_IRowset,(void**)&pIRowset);
    GRS_COM_CHECK(hr,_T("获取IRowset接口失败,错误码:0x%08X\n"),hr);
	DisplayRowSet(pIRowset);

    hr = pIRowsetChange->QueryInterface(IID_IRowsetUpdate,(void**)&pIRowsetUpdate);
    GRS_COM_CHECK(hr,_T("获取IRowsetUpdate接口失败,错误码:0x%08X\n"),hr);

CLEAR_UP:
	GRS_SAFERELEASE(pIRowsetChange);
    GRS_SAFERELEASE(pIRowset);
    GRS_SAFERELEASE(pIRowsetUpdate);
	GRS_SAFERELEASE(pIOpenRowset);

	_tsystem(_T("PAUSE"));
	CoUninitialize();
	return 0;
}



VOID DisplayRowSet(IRowset* pIRowset)
{
	GRS_USEPRINTF();
	HRESULT				hr					= S_OK;
	IColumnsInfo*		pIColumnsInfo		= NULL;
	IAccessor*			pIAccessor			= NULL;
	LPWSTR				pStringBuffer		= NULL;
	DBBINDING*			pBindings			= NULL;
	ULONG               lColumns			= 0;
	DBCOLUMNINFO *      pColumnInfo			= NULL;
	ULONG*				plDisplayLen		= NULL;
	BOOL				bShowColumnName		= FALSE;
	HACCESSOR			hAccessor			= NULL;
	LONG				lAllRows			= 0;
	ULONG               ulRowsObtained		= 0;
	ULONG               dwOffset            = 0;
	ULONG               iCol				= 0;
	ULONG				iRow				= 0;
	LONG                cRows               = 100;//一次读取100行
	HROW *              phRows				= NULL;
	VOID*				pData				= NULL;
	VOID*				pCurData			= NULL;

	//准备绑定信息
	hr = pIRowset->QueryInterface(IID_IColumnsInfo,(void**)&pIColumnsInfo);
	GRS_COM_CHECK(hr,_T("获得IColumnsInfo接口失败，错误码：0x%08X\n"),hr);
	hr = pIColumnsInfo->GetColumnInfo(&lColumns, &pColumnInfo,&pStringBuffer);
	GRS_COM_CHECK(hr,_T("获取列信息失败，错误码：0x%08X\n"),hr);

	//分配绑定结构数组的内存	
	pBindings = (DBBINDING*)GRS_CALLOC(lColumns * sizeof(DBBINDING));
	//根据列信息 填充绑定结构信息数组 注意行大小进行8字节边界对齐排列
	for( iCol = 0; iCol < lColumns; iCol++ )
	{
		pBindings[iCol].iOrdinal   = pColumnInfo[iCol].iOrdinal;
		pBindings[iCol].dwPart     = DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS;
		pBindings[iCol].obStatus   = dwOffset;
		pBindings[iCol].obLength   = dwOffset + sizeof(DBSTATUS);
		pBindings[iCol].obValue    = dwOffset+sizeof(DBSTATUS)+sizeof(ULONG);
		pBindings[iCol].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
		pBindings[iCol].eParamIO   = DBPARAMIO_NOTPARAM;
		pBindings[iCol].bPrecision = pColumnInfo[iCol].bPrecision;
		pBindings[iCol].bScale     = pColumnInfo[iCol].bScale;
		pBindings[iCol].wType      = DBTYPE_WSTR;//pColumnInfo[iCol].wType;	//强制转为WSTR
		pBindings[iCol].cbMaxLen   = pColumnInfo[iCol].ulColumnSize * sizeof(WCHAR);
		dwOffset = pBindings[iCol].cbMaxLen + pBindings[iCol].obValue;
		dwOffset = GRS_ROUNDUP(dwOffset);
	}
	plDisplayLen = (ULONG*)GRS_CALLOC(lColumns * sizeof(ULONG));

	//创建行访问器
	hr = pIRowset->QueryInterface(IID_IAccessor,(void**)&pIAccessor);
	GRS_COM_CHECK(hr,_T("获得IAccessor接口失败，错误码：0x%08X\n"),hr);
	hr = pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,lColumns,pBindings,0,&hAccessor,NULL);
	GRS_COM_CHECK(hr,_T("创建行访问器失败，错误码：0x%08X\n"),hr);

	//获取数据
	//分配cRows行数据的缓冲，然后反复读取cRows行到这里，然后逐行处理之
	pData = GRS_CALLOC(dwOffset * cRows);
	lAllRows = 0;

	while( S_OK == hr )    
	{//循环读取数据，每次循环默认读取cRows行，实际读取到ulRowsObtained行
		hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER,0,cRows,&ulRowsObtained, &phRows);
		//每次新显示一批数据时都先清0一下宽度数组
		ZeroMemory(plDisplayLen,lColumns * sizeof(ULONG));

		for( iRow = 0; iRow < ulRowsObtained; iRow++ )
		{
			//计算每列最大的显示宽度
			pCurData    = (BYTE*)pData + (dwOffset * iRow);
			pIRowset->GetData(phRows[iRow],hAccessor,pCurData);
			UpdateDisplaySize(lColumns,pBindings,pCurData,plDisplayLen);
		}

		if(!bShowColumnName)
		{//显示字段信息
			DisplayColumnNames(lColumns,pColumnInfo,plDisplayLen);
			bShowColumnName = TRUE;
		}

		for( iRow = 0; iRow < ulRowsObtained; iRow++ )
		{
			//显示数据
			pCurData    = (BYTE*)pData + (dwOffset * iRow);
			DisplayRow( ++ lAllRows,lColumns,pBindings,pCurData,plDisplayLen);
		}

		if( ulRowsObtained )
		{//释放行句柄数组
			pIRowset->ReleaseRows(ulRowsObtained,phRows,NULL,NULL,NULL);
		}
		bShowColumnName = FALSE;
		CoTaskMemFree(phRows);
		phRows = NULL;
	}

CLEAR_UP:
	if(pIAccessor && hAccessor)
	{
		pIAccessor->ReleaseAccessor(hAccessor,NULL);
		hAccessor = NULL;
	}

	GRS_SAFEFREE(pData);
	GRS_SAFEFREE(pBindings);
	GRS_SAFEFREE(plDisplayLen);
	CoTaskMemFree(pColumnInfo);
	CoTaskMemFree(pStringBuffer);
	GRS_SAFERELEASE(pIAccessor);
	GRS_SAFERELEASE(pIColumnsInfo);
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
