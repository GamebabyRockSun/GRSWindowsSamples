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
	IUnknown*				pIUnknown					= NULL;
	ISourcesRowset*			pISourcesRowset				= NULL;
	ISourcesRowset*			pISubSource					= NULL;
	IRowset*				pIRowset					= NULL;
	IParseDisplayName*		pIParseDisplayName			= NULL;

	LPMONIKER				pMoniker					= NULL;
	ULONG					ulEaten						= 0;
	TCHAR*					pOleName					= _T("SQLOLEDB Enumerator");

    DBBINDING               dbBinding[1]                = {};
    HACCESSOR               hAccessor                   = NULL;
    HROW*                   phRow                       = NULL;
    ULONG                   ulRows                      = 0;
    IAccessor*              pIAccessor                  = NULL;
    IDBInitialize*          pIDBInitialize              = NULL;
    IDBProperties*          pIDBProperties              = NULL;
    WCHAR                   pParseName[129]             = {};
    DBPROP                  InitProperties[5]           = {};
    DBPROPSET               rgInitPropSet[1]            = {};

	hr = CoCreateInstance(CLSID_OLEDB_ENUMERATOR,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IUnknown,
		(void**)&pIUnknown
		);
	GRS_COM_CHECK(hr,_T("创建OLEDB数据源枚举器对象接口失败，错误码：0x%08X\n"),hr);

	hr = pIUnknown->QueryInterface(IID_ISourcesRowset,(void**)&pISourcesRowset);
	GRS_COM_CHECK(hr,_T("获取ISourceRowset接口失败，错误码：0x%08X\n"),hr);


	//获取数据源信息结果集
	hr = pISourcesRowset->GetSourcesRowset(NULL,IID_IRowset,0,NULL,(IUnknown**)&pIRowset);
	GRS_COM_CHECK(hr,_T("获取数据源结果集失败，错误码：0x%08X\n"),hr);
	
	DisplayRowSet(pIRowset);
	GRS_SAFERELEASE(pIRowset);
  
	hr = pISourcesRowset->QueryInterface(IID_IParseDisplayName,(void**)&pIParseDisplayName);
	GRS_COM_CHECK(hr,_T("获取IParseDisplayName接口失败，错误码：0x%08X\n"),hr);


	hr = pIParseDisplayName->ParseDisplayName(NULL,pOleName,&ulEaten,&pMoniker);
	GRS_COM_CHECK(hr,_T("解析OLEDB提供者名称对象失败，错误码：0x%08X\n"),hr);

	hr = BindMoniker(pMoniker, 0, __uuidof(ISourcesRowset),(void**)&pISubSource);
	GRS_COM_CHECK(hr,_T("绑定到别名对象失败，错误码：0x%08X\n"),hr);

	//获取数据源信息结果集
	hr = pISubSource->GetSourcesRowset(NULL,IID_IRowset,0,NULL,(IUnknown**)&pIRowset);
	GRS_COM_CHECK(hr,_T("获取数据源结果集失败，错误码：0x%08X\n"),hr);
	//显示SQL OLEDB数据源实例名称
	DisplayRowSet(pIRowset);

    GRS_SAFERELEASE(pIParseDisplayName);
    GRS_SAFERELEASE(pMoniker);

    //利用枚举出的SQL Server实例名创建数据源对象
    dbBinding[0].iOrdinal   = 2;
    dbBinding[0].dwPart	    = DBPART_VALUE ;
    dbBinding[0].obStatus   = 0;
    dbBinding[0].obLength   = 0;
    dbBinding[0].obValue    = 0;
    dbBinding[0].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
    dbBinding[0].bPrecision = 0;
    dbBinding[0].bScale     = 0;
    dbBinding[0].wType      = DBTYPE_WSTR;
    dbBinding[0].cbMaxLen   = 128 * sizeof(WCHAR);

    hr = pIRowset->QueryInterface(IID_IAccessor,(void**)&pIAccessor);
    GRS_COM_CHECK(hr,_T("获取IAccessor接口失败，错误码：0x%08X\n"),hr);

    hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA,1,dbBinding,128*sizeof(WCHAR),&hAccessor,NULL);
    GRS_COM_CHECK(hr,_T("创建Accessor失败，错误码：0x%08X\n"),hr);

    hr = pIRowset->RestartPosition(DB_NULL_HCHAPTER);
    GRS_COM_CHECK(hr,_T("回到结果集起始位置失败，错误码：0x%08X\n"),hr);

    do 
    {
        hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER,0,1,&ulRows,&phRow);
        GRS_COM_CHECK(hr,_T("得到第一行数据失败，错误码：0x%08X\n"),hr);
        hr = pIRowset->GetData(phRow[0],hAccessor,pParseName);
        hr = pIRowset->ReleaseRows(ulRows,phRow,NULL,NULL,NULL);

        if(0 == lstrcmpi(pParseName,_T("ASUS-PC\\SQL2008")))
        {
            break;
        }
    } while (S_OK == hr);

    hr = pISubSource->QueryInterface(IID_IParseDisplayName,(void**)&pIParseDisplayName);
    GRS_COM_CHECK(hr,_T("获取IParseDisplayName接口失败，错误码：0x%08X\n"),hr);

    hr = pIParseDisplayName->ParseDisplayName(NULL,pParseName,&ulEaten,&pMoniker);
    GRS_COM_CHECK(hr,_T("解析SQL实例名称对象失败，错误码：0x%08X\n"),hr);

    hr = BindMoniker(pMoniker, 0, __uuidof(IDBProperties),(void**)&pIDBProperties);
    GRS_COM_CHECK(hr,_T("绑定到别名对象失败，错误码：0x%08X\n"),hr);

    //指定数据库名
    InitProperties[0].dwPropertyID      = DBPROP_INIT_CATALOG;
    InitProperties[0].vValue.vt         = VT_BSTR;
    InitProperties[0].vValue.bstrVal    = SysAllocString(L"Study");
    InitProperties[0].dwOptions         = DBPROPOPTIONS_REQUIRED;
    InitProperties[0].colid             = DB_NULLID;

    // User ID
    InitProperties[1].dwPropertyID      = DBPROP_AUTH_USERID;
    InitProperties[1].vValue.vt         = VT_BSTR;
    InitProperties[1].vValue.bstrVal    = SysAllocString(OLESTR("sa"));

    // Password
    InitProperties[2].dwPropertyID      = DBPROP_AUTH_PASSWORD;
    InitProperties[2].vValue.vt         = VT_BSTR;
    InitProperties[2].vValue.bstrVal    = SysAllocString(OLESTR("999999"));


    //创建一个GUID为DBPROPSET_DBINIT的属性集合，这也是初始化连接时需要的唯一一个属性集合
    rgInitPropSet[0].guidPropertySet    = DBPROPSET_DBINIT;
    rgInitPropSet[0].cProperties        = 3;
    rgInitPropSet[0].rgProperties       = InitProperties;

    //得到数据库初始化的属性接口
    hr = pIDBProperties->SetProperties(1, rgInitPropSet); 
    GRS_COM_CHECK(hr,_T("设置连接属性失败，错误码：0x%08x\n"),hr);
    hr = pIDBProperties->QueryInterface(IID_IDBInitialize, (void **)&pIDBInitialize);
    GRS_COM_CHECK(hr,_T("获取IDBInitialize接口失败，错误码：0x%08x\n"),hr);
    //属性一但设置完成，相应的接口就可以释放了
    GRS_SAFERELEASE(pIDBProperties);

    hr = pIDBInitialize->Initialize();
    GRS_COM_CHECK(hr,_T("连接到数据源实例[%s]失败，错误码：0x%08x\n"),pParseName,hr);

    GRS_PRINTF(_T("连接到数据源实例[%s]成功......\n"),pParseName);

    pIDBInitialize->Uninitialize();

CLEAR_UP:
    if(NULL != hAccessor && NULL != pIAccessor)
    {
        pIAccessor->ReleaseAccessor(hAccessor,NULL);
    }
    GRS_SAFERELEASE(pIAccessor);
    GRS_SAFERELEASE(pMoniker);
    GRS_SAFERELEASE(pIParseDisplayName);
    GRS_SAFERELEASE(pIDBInitialize);
    GRS_SAFERELEASE(pIDBProperties);
	GRS_SAFERELEASE(pIRowset);
	GRS_SAFERELEASE(pISubSource);
	GRS_SAFERELEASE(pISourcesRowset);

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