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
#define GRS_CREALLOC(p,sz)	HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_SAFERELEASE(I) if(NULL != (I)){(I)->Release();(I)=NULL;}
#define GRS_COM_CHECK(hr,...) if(FAILED(hr)){GRS_PRINTF(__VA_ARGS__);goto CLEAR_UP;}

#define GRS_ROUNDUP_AMOUNT 8 
#define GRS_ROUNDUP_(size,amount) (((ULONG)(size)+((amount)-1))&~((amount)-1)) 
#define GRS_ROUNDUP(size) GRS_ROUNDUP_(size, GRS_ROUNDUP_AMOUNT) 

#define MAX_DISPLAY_SIZE	40
#define MIN_DISPLAY_SIZE    3
#define MAX_COL_SIZE		1024

BOOL CreateDBSession(IOpenRowset*&pIOpenRowset);
BOOL WriteFileData(LPCTSTR pszFileName,BYTE*pFileData,DWORD dwDataLen);

int _tmain()
{
	GRS_USEPRINTF();
	CoInitialize(NULL);

	HRESULT					hr							= S_OK;
	IOpenRowset*			pIOpenRowset				= NULL;
	IRowset*				pIRowset					= NULL;
	IColumnsInfo*			pIColumnsInfo				= NULL;
	IAccessor*				pIAccessor					= NULL;
	IRowsetInfo*			pIRowsetInfo				= NULL;
	ISequentialStream*		pISeqStream					= NULL;

	LPWSTR					pStringBuffer				= NULL;
	ULONG					lColumns					= 0;
	DBCOLUMNINFO*			pColumnInfo					= NULL;

	BOOL					bMultiObjs					= FALSE;//是否支持多个BLOB型字段
	ULONG					iCol						= 0;
	ULONG					iRow						= 0;
	ULONG*					pulColCnts					= 0;
	ULONG*					pulDataLen					= 0;
	ULONG					ulBlobs						= 0;
	DBBINDING*				pBindings					= NULL;
	DBBINDING**				ppBindings					= NULL;
	ULONG					ulBindCnt					= 0;
	HACCESSOR*				phAccessor					= 0;
	DBBINDSTATUS*			pBindStatus					= 0;
	TCHAR*					pszTableName				= _T("T_News");
	DBID					TableID						= {};
	//属性ID集合 和 属性ID
	DBPROPID                PropertyIDs[4]				= {};
	DBPROPIDSET				PropertyIDSets[1]			= {};

	ULONG					ulPropSets					= 0;
	DBPROPSET*				pGetPropSets				= NULL;
	BYTE*					pNewData1					= NULL;
	BYTE*					pNewData2					= NULL;
	BYTE*					pNewData3					= NULL;
	BYTE*					pFileData					= NULL;
	DWORD					dwFileLen					= 0;
	ULONG					ulWrited					= 0;
	HROW*					phGetRows					= NULL;
	ULONG					ulGetRows					= 0;
	DBROWSTATUS*			pUpdateStatus				= NULL;
	
	if(!CreateDBSession(pIOpenRowset))
	{
		goto CLEAR_UP;
	}

	TableID.eKind           = DBKIND_NAME;
	TableID.uName.pwszName  = (LPOLESTR)pszTableName;

	hr = pIOpenRowset->OpenRowset(NULL,&TableID,NULL,IID_IRowset,0,NULL,(IUnknown**)&pIRowset);
	GRS_COM_CHECK(hr,_T("打开表对象'%s'失败，错误码：0x%08X\n"),pszTableName,hr);

	hr = pIRowset->QueryInterface(IID_IRowsetInfo,(void**)&pIRowsetInfo);
	GRS_COM_CHECK(hr,_T("获取IColumnsInfo接口失败，错误码：0x%08X\n"),hr);

	//------------------------------------------------------------------------------
	//是否支持多个BLOB型字段在一个绑定器中绑定的属性
	PropertyIDs[0] = DBPROP_MULTIPLESTORAGEOBJECTS;

	PropertyIDSets[0].rgPropertyIDs     = PropertyIDs;
	PropertyIDSets[0].cPropertyIDs      = 1;
	PropertyIDSets[0].guidPropertySet   = DBPROPSET_ROWSET;

	hr = pIRowsetInfo->GetProperties(1,PropertyIDSets,&ulPropSets,&pGetPropSets);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("获取Rowset属性DBPROP_MULTIPLESTORAGEOBJECTS信息失败，错误码：0x%08X\n"),hr);
	}

	if( V_VT(&pGetPropSets[0].rgProperties[0].vValue) == VT_BOOL )
	{
		bMultiObjs = V_BOOL(&pGetPropSets[0].rgProperties[0].vValue);
	}

	if( DBPROPSTATUS_NOTSUPPORTED == pGetPropSets[0].rgProperties[0].dwStatus )
	{
		bMultiObjs = FALSE;
	}
	//------------------------------------------------------------------------------

	hr = pIRowset->QueryInterface(IID_IColumnsInfo,(void**)&pIColumnsInfo);
	GRS_COM_CHECK(hr,_T("获取IColumnsInfo接口失败，错误码：0x%08X\n"),hr);

	hr = pIColumnsInfo->GetColumnInfo(&lColumns,&pColumnInfo,&pStringBuffer);
	GRS_COM_CHECK(hr,_T("获取列信息失败，错误码：0x%08X\n"),hr);

	pBindings = (DBBINDING*)GRS_CALLOC(lColumns * sizeof(DBBINDING));
	pulColCnts = (ULONG*)GRS_CALLOC(sizeof(ULONG));
	pulDataLen = (ULONG*)GRS_CALLOC(sizeof(ULONG));
	ppBindings = (DBBINDING**)GRS_CALLOC(sizeof(DBBINDING*));	//默认是一个绑定

	for(iCol = 0; iCol < lColumns;iCol ++)
	{
		if(NULL == ppBindings[ulBindCnt])
		{
			ppBindings[ulBindCnt] = &pBindings[iCol];
		}
		//累加列数
		++ pulColCnts[ulBindCnt];

		pBindings[iCol].iOrdinal   = pColumnInfo[iCol].iOrdinal;
		pBindings[iCol].dwPart     = DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS;
		pBindings[iCol].obStatus   = pulDataLen[ulBindCnt];
		pBindings[iCol].obLength   = pulDataLen[ulBindCnt] + sizeof(DBSTATUS);
		pBindings[iCol].obValue    = pulDataLen[ulBindCnt] + sizeof(DBSTATUS) + sizeof(ULONG);
		pBindings[iCol].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
		pBindings[iCol].eParamIO   = DBPARAMIO_NOTPARAM;
		pBindings[iCol].bPrecision = pColumnInfo[iCol].bPrecision;
		pBindings[iCol].bScale     = pColumnInfo[iCol].bScale;
		pBindings[iCol].wType      = pColumnInfo[iCol].wType;	
		pBindings[iCol].cbMaxLen   = pColumnInfo[iCol].ulColumnSize;

		if( ( DBTYPE_IUNKNOWN == pColumnInfo[iCol].wType ||
			( pColumnInfo[iCol].dwFlags & DBCOLUMNFLAGS_ISLONG ) )  )
		{
			pBindings[iCol].pBindExt			= NULL;
			pBindings[iCol].dwFlags				= 0;
			pBindings[iCol].bPrecision			= 0;
			pBindings[iCol].bScale				= 0;
			pBindings[iCol].wType				= DBTYPE_IUNKNOWN;
			pBindings[iCol].cbMaxLen			= 0;			
			pBindings[iCol].pObject				= (DBOBJECT*)GRS_CALLOC(sizeof(DBOBJECT));
			pBindings[iCol].pObject->iid		= IID_ISequentialStream;
			pBindings[iCol].pObject->dwFlags	= STGM_READ;
			++ ulBlobs;
		}	

		pBindings[iCol].cbMaxLen = min(pBindings[iCol].cbMaxLen, MAX_COL_SIZE);
		pulDataLen[ulBindCnt] += pBindings[iCol].cbMaxLen + pBindings[iCol].obValue;
		if(DBTYPE_IUNKNOWN == pBindings[iCol].wType)
		{//长度是0，但是要把接口指针的位置留出来
			pulDataLen[ulBindCnt]				+= sizeof(ISequentialStream*);
		}
		pulDataLen[ulBindCnt] = GRS_ROUNDUP(pulDataLen[ulBindCnt]);

		if( ( (!bMultiObjs && ulBlobs) || 0 == pColumnInfo[iCol].iOrdinal )
			&& (iCol != (lColumns - 1)) )
		{//如果一个访问器不能有多个BLOB,第0列跳过 不能用于更新
			//增加一个绑定
			++ ulBindCnt;
			pulColCnts = (ULONG*)GRS_CREALLOC(pulColCnts,(ulBindCnt + 1) * sizeof(ULONG));
			pulDataLen = (ULONG*)GRS_CREALLOC(pulDataLen,(ulBindCnt + 1) * sizeof(ULONG));
			ppBindings = (DBBINDING**)GRS_CREALLOC(ppBindings,(ulBindCnt + 1) * sizeof(DBBINDING*));
            ulBlobs = 0;
		}
	}

	hr = pIRowset->QueryInterface(IID_IAccessor,(void**)&pIAccessor);
	GRS_COM_CHECK(hr,_T("获取IAccessor接口失败，错误码:0x%08X\n"),hr);

	phAccessor = (HACCESSOR*)GRS_CALLOC( (ulBindCnt + 1) * sizeof(HACCESSOR));

	for(iCol = 0;iCol <= ulBindCnt;iCol++)
	{
		pBindStatus = (DBBINDSTATUS*)GRS_CALLOC(pulColCnts[iCol] * sizeof(DBBINDSTATUS));
		hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA,pulColCnts[iCol]
		,ppBindings[iCol],pulDataLen[iCol],&phAccessor[iCol],pBindStatus);
		GRS_COM_CHECK(hr,_T("创建第[%ul]个访问器失败，错误码:0x%08x\n"),iCol,hr);
		GRS_SAFEFREE(pBindStatus);
	}

	hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER,0,1,&ulGetRows,&phGetRows);
	GRS_COM_CHECK(hr,_T("调用GetNextRows获取行数据失败，错误码:0x%08x\n"),hr);
	if(0 == ulGetRows)
	{
		GRS_PRINTF(_T("数据表为空，未能获得任何数据！\n"));
		goto CLEAR_UP;
	}

	if(0 == pColumnInfo[0].iOrdinal)
	{
		iCol = 1;
	}
	else
	{
		iCol = 0;
	}
	
	//先读取第一个访问器中的列 注意 跳过第0列这个特殊列
	pNewData1 = (BYTE*)GRS_CALLOC(pulDataLen[iCol]);
	hr = pIRowset->GetData(phGetRows[0],phAccessor[iCol],pNewData1);
	GRS_COM_CHECK(hr,_T("调用GetData获取第一列失败，错误码：0x%08X\n"),hr);

	for(ULONG i = 0; i < pulColCnts[iCol]; i++)
	{
		if(DBTYPE_IUNKNOWN == ppBindings[iCol][i].wType)
		{//BLOB字段
			if( DBSTATUS_S_OK == *((DBSTATUS*)(pNewData1 + ppBindings[iCol][i].obStatus)))
			{
				dwFileLen = *(ULONG*)(pNewData1 + ppBindings[iCol][i].obLength);
				if( dwFileLen > 0 )
				{
					pFileData = (BYTE*)GRS_CALLOC(dwFileLen);
					pISeqStream = *(ISequentialStream**)(pNewData1 + ppBindings[iCol][i].obValue);
					hr = pISeqStream->Read(pFileData,dwFileLen,&ulWrited);
					WriteFileData(_T("DB1.jpg"),pFileData,dwFileLen);
					GRS_SAFEFREE(pFileData);
					GRS_SAFERELEASE(pISeqStream);
				}			
			}

		}
	}

	++ iCol; //下一个访问器
	pNewData2 = (BYTE*)GRS_CALLOC(pulDataLen[iCol]);
	hr = pIRowset->GetData(phGetRows[0],phAccessor[iCol],pNewData2);
	GRS_COM_CHECK(hr,_T("调用GetData获取第二列失败，错误码：0x%08X\n"),hr);

	if(DBTYPE_IUNKNOWN == ppBindings[iCol][0].wType)
	{//BLOB字段
		if( DBSTATUS_S_OK == *((DBSTATUS*)(pNewData2 + ppBindings[iCol][0].obStatus)))
		{
			dwFileLen = *(ULONG*)(pNewData2 + ppBindings[iCol][0].obLength);
			if( dwFileLen > 0 )
			{
				pFileData = (BYTE*)GRS_CALLOC(dwFileLen);
				pISeqStream = *(ISequentialStream**)(pNewData2 + ppBindings[iCol][0].obValue);
				hr = pISeqStream->Read(pFileData,dwFileLen,&ulWrited);
				WriteFileData(_T("DB2.jpg"),pFileData,dwFileLen);
				GRS_SAFEFREE(pFileData);
				GRS_SAFERELEASE(pISeqStream);
			}			
		}
	}
	++ iCol; //下一个访问器
	pNewData3 = (BYTE*)GRS_CALLOC(pulDataLen[iCol]);
	hr = pIRowset->GetData(phGetRows[0],phAccessor[iCol],pNewData3);
	GRS_COM_CHECK(hr,_T("调用GetData获取第三列失败，错误码：0x%08X\n"),hr);

	if(DBTYPE_IUNKNOWN == ppBindings[iCol][0].wType)
	{//BLOB字段
		if( DBSTATUS_S_OK == *((DBSTATUS*)(pNewData3 + ppBindings[iCol][0].obStatus)))
		{
			dwFileLen = *(ULONG*)(pNewData3 + ppBindings[iCol][0].obLength);
			if( dwFileLen > 0 )
			{
				pFileData = (BYTE*)GRS_CALLOC(dwFileLen);
				pISeqStream = *(ISequentialStream**)(pNewData3 + ppBindings[iCol][0].obValue);
				hr = pISeqStream->Read(pFileData,dwFileLen,&ulWrited);
				WriteFileData(_T("DB3.txt"),pFileData,dwFileLen);
				GRS_SAFEFREE(pFileData);
				GRS_SAFERELEASE(pISeqStream);
			}			
		}
	}
	hr = pIRowset->ReleaseRows(1,phGetRows,NULL,NULL,NULL);
	GRS_COM_CHECK(hr,_T("调用ReleaseRows释放新行句柄失败，错误码：0x%08X\n"),hr);

	GRS_PRINTF(_T("数据数据成功，共读取了3个blob型数据，请查看保存的文件结果！\n"));

CLEAR_UP:
    for(iCol = 0; iCol < lColumns;iCol ++)
    {
        GRS_SAFEFREE(pBindings[iCol].pObject);
    }

	if(phAccessor && pIAccessor)
	{
		for(iCol = 0; iCol <= ulBindCnt;iCol++)
		{
			pIAccessor->ReleaseAccessor(phAccessor[iCol],NULL);
		}
	}
	GRS_SAFEFREE(pFileData);
	GRS_SAFEFREE(pNewData1);
	GRS_SAFEFREE(pNewData2);
	GRS_SAFEFREE(pNewData3);
	GRS_SAFEFREE(pBindStatus);
	GRS_SAFEFREE(phAccessor);
	GRS_SAFEFREE(pulDataLen);
	GRS_SAFEFREE(pulColCnts);
	GRS_SAFEFREE(ppBindings);
	GRS_SAFEFREE(pBindings);
	CoTaskMemFree(pUpdateStatus);
	CoTaskMemFree(pColumnInfo);
	CoTaskMemFree(pStringBuffer);
	if(NULL != pGetPropSets)
	{
		CoTaskMemFree(pGetPropSets[0].rgProperties);
	}
	CoTaskMemFree(pGetPropSets);
	GRS_SAFERELEASE(pIRowsetInfo);
	GRS_SAFERELEASE(pIColumnsInfo);
	GRS_SAFERELEASE(pIAccessor);
	GRS_SAFERELEASE(pIRowset);
	GRS_SAFERELEASE(pIOpenRowset);

	_tsystem(_T("PAUSE"));
	CoUninitialize();
	return 0;
}

BOOL WriteFileData(LPCTSTR pszFileName,BYTE*pFileData,DWORD dwDataLen)
{//将数据写入指定的文件中
	GRS_USEPRINTF();
	HANDLE hFile = CreateFile(pszFileName,GENERIC_WRITE
		,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(INVALID_HANDLE_VALUE == hFile)
	{
		GRS_PRINTF(_T("无法创建文件\"%s\""),pszFileName);
		return FALSE;
	}

	BOOL bRet = WriteFile(hFile,pFileData,dwDataLen,&dwDataLen,NULL);
	CloseHandle(hFile);

	return bRet;
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
	
 //   //指定身份验证方式为集成安全模式“SSPI”
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
