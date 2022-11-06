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
#include <stddef.h>

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

#define K_STATECODE_LEN 11
#define F_STATENAME_LEN 51
#pragma pack(push)
#pragma pack(8)
//按照数据库字段的大小定义出一个结构
//并且使该结构自动8字节边界对齐 
struct ST_T_STATE
{
	DBSTATUS m_stsKStateCode;
	ULONG    m_ulKStateCode;
	WCHAR	 m_pszKStateCode[K_STATECODE_LEN];

	DBSTATUS m_stsFStateName;
	ULONG    m_ulFStateName;
	WCHAR	 m_pszFStateName[F_STATENAME_LEN];
};

#pragma pack(pop)



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
	GRS_DEF_INTERFACE(IAccessor);
	GRS_DEF_INTERFACE(IColumnsInfo);
	GRS_DEF_INTERFACE(IRowset);


	TCHAR* pSQL = _T("Select K_StateCode,F_StateName From T_State");
	DBPROPSET ps[1] = {};
	DBPROP prop[2]	= {};

	ULONG               cColumns;
	DBCOLUMNINFO *      rgColumnInfo        = NULL;
	LPWSTR              pStringBuffer       = NULL;
	ULONG               iCol				= 0;
	DBBINDING	        rgBindings[2]       = {};
	HACCESSOR			hAccessor			= NULL;
	ST_T_STATE *        pData               = NULL;
	ST_T_STATE *        pCurData			= NULL;
	ULONG               cRowsObtained		= 0;
	HROW *              rghRows             = NULL;
	ULONG               iRow				= 0;
	ULONG				lAllRows			= 0;
	LONG                cRows               = 100;//一次读取10行
	ULONG*				plDisplayLen		= NULL;
	
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

	GRS_COM_CHECK(hr,_T("设置Command对象属性失败，错误码：0x%08X\n"),hr);
	hr = pICommandText->Execute(NULL,IID_IRowset,NULL,NULL,(IUnknown**)&pIRowset);
	GRS_COM_CHECK(hr,_T("执行SQL语句失败，错误码：0x%08X\n"),hr);

	//准备绑定信息
	hr = pIRowset->QueryInterface(IID_IColumnsInfo,(void**)&pIColumnsInfo);
	GRS_COM_CHECK(hr,_T("获得IColumnsInfo接口失败，错误码：0x%08X\n"),hr);
	hr = pIColumnsInfo->GetColumnInfo(&cColumns, &rgColumnInfo,&pStringBuffer);
	GRS_COM_CHECK(hr,_T("获取列信息失败，错误码：0x%08X\n"),hr);

	//根据列信息 填充绑定结构信息数组 注意行大小进行8字节边界对齐排列
	iCol = 0;
	rgBindings[iCol].iOrdinal   = rgColumnInfo[iCol].iOrdinal;
	rgBindings[iCol].dwPart     = DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS;
	rgBindings[iCol].obStatus   = offsetof(ST_T_STATE,m_stsKStateCode);
	rgBindings[iCol].obLength   = offsetof(ST_T_STATE,m_ulKStateCode);
	rgBindings[iCol].obValue    = offsetof(ST_T_STATE,m_pszKStateCode);
	rgBindings[iCol].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
	rgBindings[iCol].eParamIO   = DBPARAMIO_NOTPARAM;
	rgBindings[iCol].bPrecision = 0;
	rgBindings[iCol].bScale     = 0;
	rgBindings[iCol].wType      = DBTYPE_WSTR;
	rgBindings[iCol].cbMaxLen   = K_STATECODE_LEN * sizeof(WCHAR);
	++iCol;
	rgBindings[iCol].iOrdinal   = rgColumnInfo[iCol].iOrdinal;
	rgBindings[iCol].dwPart     = DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS;
	rgBindings[iCol].obStatus   = offsetof(ST_T_STATE,m_stsFStateName);
	rgBindings[iCol].obLength   = offsetof(ST_T_STATE,m_ulFStateName);
	rgBindings[iCol].obValue    = offsetof(ST_T_STATE,m_pszFStateName);
	rgBindings[iCol].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
	rgBindings[iCol].eParamIO   = DBPARAMIO_NOTPARAM;
	rgBindings[iCol].bPrecision = 0;
	rgBindings[iCol].bScale     = 0;
	rgBindings[iCol].wType      = DBTYPE_WSTR;
	rgBindings[iCol].cbMaxLen   = F_STATENAME_LEN * sizeof(WCHAR);

	plDisplayLen = (ULONG*)GRS_CALLOC(cColumns * sizeof(ULONG));

	//创建行访问器
	hr = pIRowset->QueryInterface(IID_IAccessor,(void**)&pIAccessor);
	GRS_COM_CHECK(hr,_T("获得IAccessor接口失败，错误码：0x%08X\n"),hr);
	hr = pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,cColumns,rgBindings,0,&hAccessor,NULL);
	GRS_COM_CHECK(hr,_T("创建行访问器失败，错误码：0x%08X\n"),hr);

	//分配cRows行数据的缓冲，然后反复读取cRows行到这里，然后逐行处理之
	pData = (ST_T_STATE*)GRS_CALLOC(cRows * sizeof(ST_T_STATE));

	lAllRows = 0;
    GRS_PRINTF(_T("|%-10s|%-50s|\n"),rgColumnInfo[0].pwszName,rgColumnInfo[1].pwszName);
	while( S_OK == pIRowset->GetNextRows(DB_NULL_HCHAPTER,0,cRows,&cRowsObtained, &rghRows) )    
	{//循环读取数据，每次循环默认读取cRows行，实际读取到cRowsObtained行
		//每次新显示一批数据时都先清0一下宽度数组
		ZeroMemory(plDisplayLen,cColumns * sizeof(ULONG));
		for( iRow = 0; iRow < cRowsObtained; iRow++ )
		{
			//计算每列最大的显示宽度
			//pCurData    = pData + iRow;
			pIRowset->GetData(rghRows[iRow],hAccessor,&pData[iRow]);
			//在此处以可以通过访问结构成员的方法来访问字段了
			//pCurData->m_pszKStateCode
			//pCurData->m_pszFStateName

			//UpdateDisplaySize(cColumns,rgBindings,pCurData,plDisplayLen);

            GRS_PRINTF(_T("|%-10s|%-50s|\n"),pData[iRow].m_pszKStateCode,pData[iRow].m_pszFStateName);
		}

		//DisplayColumnNames(cColumns,rgColumnInfo,plDisplayLen);


		//for( iRow = 0; iRow < cRowsObtained; iRow++ )
		//{
		//	//显示数据
		//	pCurData    = pData + iRow;
		//	DisplayRow(++lAllRows,cColumns,rgBindings,pCurData,plDisplayLen);
		//}

		if( cRowsObtained )
		{//释放行句柄数组
			pIRowset->ReleaseRows(cRowsObtained,rghRows,NULL,NULL,NULL);
		}
		CoTaskMemFree(rghRows);
		rghRows = NULL;
	}

CLEAR_UP:
	if(NULL != pIAccessor)
	{
		pIAccessor->ReleaseAccessor(hAccessor,NULL);
	}
	GRS_SAFEFREE(pData);
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