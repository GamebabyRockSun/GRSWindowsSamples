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
	HRESULT				hr					= S_OK;
	IOpenRowset*		pIOpenRowset		= NULL;
	IDBCreateCommand*	pIDBCreateCommand	= NULL;

	ICommand*			pICommand			= NULL;
	ICommandText*		pICommandText		= NULL;
	IMultipleResults*	pIMultipleResults	= NULL;

	GRS_DEF_INTERFACE(IAccessor);
	GRS_DEF_INTERFACE(IColumnsInfo);
	GRS_DEF_INTERFACE(IRowset);

	TCHAR* pSQL = _T("Select * From T_State Where Left(K_StateCode,2) = '11';\
					 Select * From T_State Where Left(K_StateCode,2) = '32';\
                     Select * From T_State Where Left(K_StateCode,2) = '21';");

	ULONG               cColumns;
	DBCOLUMNINFO *      rgColumnInfo        = NULL;
	LPWSTR              pStringBuffer       = NULL;
	ULONG               iCol				= 0;
	ULONG               dwOffset            = 0;
	DBBINDING *         rgBindings          = NULL;
	HACCESSOR			hAccessor			= NULL;
	void *              pData               = NULL;
	ULONG               cRowsObtained		= 0;
	HROW *              rghRows             = NULL;
	ULONG               iRow				= 0;
	ULONG				lAllRows			= 0;
	LONG                cRows               = 100;//一次读取100行
	void *              pCurData			= NULL;

	LONG				lRowsetNum			= 0;

	ULONG*				plDisplayLen		= NULL;
	BOOL				bShowColumnName		= FALSE;


	if(!CreateDBSession(pIOpenRowset))
	{
		_tsystem(_T("PAUSE"));
		return -1;
	}
    //======================================================================================================
    //判定数据源提供者是否支持IMultipleResults接口及对象
    IGetDataSource* pIGetDataSource = NULL;
    IDBProperties*  pIDBProperties = NULL;

    //属性ID集合 和 属性ID
    DBPROPID                PropertyIDs[4]				= {};
    DBPROPIDSET				PropertyIDSets[1]			= {};
    //返回的属性集个数和对应的属性集结构数组指针
    ULONG					ulPropSets					= 0;
    DBPROPSET*				pGetPropSets				= NULL;
    //多结果集属性ID
    PropertyIDs[0]                      = DBPROP_MULTIPLERESULTS;

    PropertyIDSets[0].rgPropertyIDs     = PropertyIDs;
    PropertyIDSets[0].cPropertyIDs      = 1;
    PropertyIDSets[0].guidPropertySet   = DBPROPSET_DATASOURCEINFO;

    hr = pIOpenRowset->QueryInterface(IID_IGetDataSource,(void**)&pIGetDataSource);
    GRS_COM_CHECK(hr,_T("获取IGetDataSource接口失败，错误码：0x%08X\n"),hr);
    hr = pIGetDataSource->GetDataSource(IID_IDBProperties,(IUnknown**)&pIDBProperties);
    GRS_COM_CHECK(hr,_T("获取IDBProperties接口失败，错误码：0x%08X\n"),hr);

    hr = pIDBProperties->GetProperties(1,PropertyIDSets,&ulPropSets,&pGetPropSets);
    GRS_COM_CHECK(hr,_T("获取DBPROP_MULTIPLERESULTS数据源属性失败，错误码：0x%08X\n"),hr);

    if(0 < ulPropSets && VT_I4 == pGetPropSets[0].rgProperties[0].vValue.vt)
    {
        if(DBPROPVAL_MR_SUPPORTED & pGetPropSets[0].rgProperties[0].vValue.intVal)
        {
            GRS_PRINTF(_T("当前数据源对象支持IMultipleResults接口及对象.\n"));
        }
        
        if(DBPROPVAL_MR_NOTSUPPORTED & pGetPropSets[0].rgProperties[0].vValue.intVal)
        {
            GRS_PRINTF(_T("当前数据源对象不支持IMultipleResults接口及对象.\n"));
            goto CLEAR_UP;
        }

        if(DBPROPVAL_MR_CONCURRENT & pGetPropSets[0].rgProperties[0].vValue.intVal)
        {
            GRS_PRINTF(_T("当前数据源对象允许同时打开和操作多个由IMultipleResults::GetResult返回的结果集对象.\n"));
        }
    }
    CoTaskMemFree(pGetPropSets[0].rgProperties);
    CoTaskMemFree(pGetPropSets);
    //======================================================================================================

	hr = pIOpenRowset->QueryInterface(IID_IDBCreateCommand,(void**)&pIDBCreateCommand);
	GRS_COM_CHECK(hr,_T("获取IDBCreateCommand接口失败，错误码：0x%08X\n"),hr);

	hr = pIDBCreateCommand->CreateCommand(NULL,IID_ICommand,(IUnknown**)&pICommand);
	GRS_COM_CHECK(hr,_T("创建ICommand接口失败，错误码：0x%08X\n"),hr);

	//执行命令
	hr = pICommand->QueryInterface(IID_ICommandText,(void**)&pICommandText);
	GRS_COM_CHECK(hr,_T("获取ICommandText接口失败，错误码：0x%08X\n"),hr);
	hr = pICommandText->SetCommandText(DBGUID_DEFAULT,pSQL);
	GRS_COM_CHECK(hr,_T("设置SQL语句失败，错误码：0x%08X\n"),hr);

	hr = pICommandText->Execute(NULL,IID_IMultipleResults,NULL,NULL,(IUnknown**)&pIMultipleResults);
	GRS_COM_CHECK(hr,_T("执行SQL语句失败，错误码：0x%08X\n"),hr);

	
	while(S_OK == pIMultipleResults->GetResult(NULL,DBRESULTFLAG_DEFAULT,IID_IRowset,NULL,(IUnknown**)&pIRowset))
	{
		GRS_PRINTF(_T("显示第%ld个结果集：\n"),++lRowsetNum);

		//准备绑定信息
		hr = pIRowset->QueryInterface(IID_IColumnsInfo,(void**)&pIColumnsInfo);
		GRS_COM_CHECK(hr,_T("获得IColumnsInfo接口失败，错误码：0x%08X\n"),hr);
		hr = pIColumnsInfo->GetColumnInfo(&cColumns, &rgColumnInfo,&pStringBuffer);
		GRS_COM_CHECK(hr,_T("获取列信息失败，错误码：0x%08X\n"),hr);

		//分配绑定结构数组的内存	
		rgBindings = (DBBINDING*)GRS_CALLOC(cColumns * sizeof(DBBINDING));
		//根据列信息 填充绑定结构信息数组 注意行大小进行8字节边界对齐排列
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
			rgBindings[iCol].wType      = DBTYPE_WSTR;//rgColumnInfo[iCol].wType;	//强制转为WSTR
			rgBindings[iCol].cbMaxLen   = rgColumnInfo[iCol].ulColumnSize * sizeof(WCHAR);
			dwOffset = rgBindings[iCol].cbMaxLen + rgBindings[iCol].obValue;
			dwOffset = GRS_ROUNDUP(dwOffset);
		}
		plDisplayLen = (ULONG*)GRS_CALLOC(cColumns * sizeof(ULONG));

		//创建行访问器
		hr = pIRowset->QueryInterface(IID_IAccessor,(void**)&pIAccessor);
		GRS_COM_CHECK(hr,_T("获得IAccessor接口失败，错误码：0x%08X\n"),hr);
		hr = pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,cColumns,rgBindings,0,&hAccessor,NULL);
		GRS_COM_CHECK(hr,_T("创建行访问器失败，错误码：0x%08X\n"),hr);

		//获取数据
		//分配cRows行数据的缓冲，然后反复读取cRows行到这里，然后逐行处理之
		pData = GRS_CALLOC(dwOffset * cRows);
		lAllRows = 0;

		while( S_OK == hr )    
		{//循环读取数据，每次循环默认读取cRows行，实际读取到cRowsObtained行
			hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER,0,cRows,&cRowsObtained, &rghRows);
			
			//每次新显示一批数据时都先清0一下宽度数组
			ZeroMemory(plDisplayLen,cColumns * sizeof(ULONG));
			for( iRow = 0; iRow < cRowsObtained; iRow++ )
			{
				//计算每列最大的显示宽度
				pCurData    = (BYTE*)pData + (dwOffset * iRow);
				pIRowset->GetData(rghRows[iRow],hAccessor,pCurData);
				UpdateDisplaySize(cColumns,rgBindings,pCurData,plDisplayLen);
			}

			if(!bShowColumnName)
			{//显示字段信息
				DisplayColumnNames(cColumns,rgColumnInfo,plDisplayLen);
				bShowColumnName = TRUE;
			}

			for( iRow = 0; iRow < cRowsObtained; iRow++ )
			{
				//显示数据
				pCurData    = (BYTE*)pData + (dwOffset * iRow);
				DisplayRow(++lAllRows,cColumns,rgBindings,pCurData,plDisplayLen);
			}

			if( cRowsObtained )
			{//释放行句柄数组
				pIRowset->ReleaseRows(cRowsObtained,rghRows,NULL,NULL,NULL);
			}
			bShowColumnName = FALSE;
			CoTaskMemFree(rghRows);
			rghRows = NULL;
		}

		pIAccessor->ReleaseAccessor(hAccessor,NULL);
		hAccessor = NULL;

		GRS_SAFEFREE(pData);
		GRS_SAFEFREE(rgBindings);
		GRS_SAFEFREE(plDisplayLen);
		CoTaskMemFree(rgColumnInfo);
		CoTaskMemFree(pStringBuffer);
		GRS_SAFERELEASE(pIRowset);
		GRS_SAFERELEASE(pIAccessor);
		GRS_SAFERELEASE(pIColumnsInfo);
		_tsystem(_T("PAUSE"));
	}

    
CLEAR_UP:
    CoTaskMemFree(pGetPropSets[0].rgProperties);
    CoTaskMemFree(pGetPropSets);
    GRS_SAFERELEASE(pIDBProperties);
    GRS_SAFERELEASE(pIGetDataSource);

	GRS_SAFEFREE(pData);
	GRS_SAFEFREE(rgBindings);
	GRS_SAFEFREE(plDisplayLen);
	CoTaskMemFree(rgColumnInfo);
	CoTaskMemFree(pStringBuffer);
	GRS_SAFERELEASE(pIRowset);
	GRS_SAFERELEASE(pIAccessor);
	GRS_SAFERELEASE(pIColumnsInfo);

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