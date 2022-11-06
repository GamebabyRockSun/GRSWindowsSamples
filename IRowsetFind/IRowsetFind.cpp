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
    ICommandProperties* pICommandProperties = NULL;
    GRS_DEF_INTERFACE(IAccessor);
    GRS_DEF_INTERFACE(IColumnsInfo);
    GRS_DEF_INTERFACE(IRowset);
    //==============================================================================
    IRowsetFind*        pIRowsetFind        = NULL;
    //==============================================================================

    TCHAR* pSQL = _T("Select top 100 K_StateCode,F_StateName From T_State");
    DBPROPSET ps[1] = {};
    DBPROP prop[2]	= {};

    ULONG               cColumns;
    DBCOLUMNINFO *      rgColumnInfo        = NULL;
    LPWSTR              pStringBuffer       = NULL;
    ULONG               iCol				= 0;
    ULONG               dwOffset            = 0;
    ULONG               ulFindDataLen       = 0;
    DBBINDING *         rgBindings          = NULL;
    HACCESSOR			hAccessor			= NULL;
    DBBINDING           pFindBinding[1]     = {};
    HACCESSOR           hFindAccessor       = NULL;
    void *              pData               = NULL;
    BYTE*               pFindData           = NULL;
    ULONG               cRowsObtained		= 0;
    HROW *              rghRows             = NULL;
    ULONG               iRow				= 0;
    ULONG				lAllRows			= 0;
    LONG                cRows               = 10;//一次读取10行
    void *              pCurData			= NULL;
    ULONG*				plDisplayLen		= NULL;
    BOOL				bShowColumnName		= FALSE;

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

    //设置属性
    //申请打开IRowsetFind接口
    prop[0].dwPropertyID    = DBPROP_IRowsetFind;
    prop[0].vValue.vt       = VT_BOOL;
    prop[0].vValue.boolVal  = VARIANT_TRUE;
    prop[0].dwOptions       = DBPROPOPTIONS_REQUIRED;
    prop[0].colid           = DB_NULLID;

    ps[0].guidPropertySet = DBPROPSET_ROWSET;       //注意属性集合的名称
    ps[0].cProperties = 1;
    ps[0].rgProperties = prop;

    //设置属性 执行语句 获得行集
    hr = pICommandText->QueryInterface(IID_ICommandProperties,
        (void**)& pICommandProperties);
    GRS_COM_CHECK(hr,_T("获取ICommandProperties接口失败，错误码：0x%08X\n"),hr);
    hr = pICommandProperties->SetProperties(1,ps);//注意必须在Execute前设定属性
    GRS_COM_CHECK(hr,_T("设置Command对象属性失败，错误码：0x%08X\n"),hr);
    hr = pICommandText->Execute(NULL,IID_IRowset,NULL,NULL,(IUnknown**)&pIRowset);
    GRS_COM_CHECK(hr,_T("执行SQL语句失败，错误码：0x%08X\n"),hr);

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
        rgBindings[iCol].obValue    = dwOffset + sizeof(DBSTATUS) + sizeof(ULONG);
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


    //==============================================================================
    hr = pIRowset->QueryInterface(IID_IRowsetFind,(void**)&pIRowsetFind);
    GRS_COM_CHECK(hr,_T("获取IRowsetFind接口失败,错误码:0x%08X\n"),hr);

    if(0 == rgColumnInfo[0].iOrdinal)
    {//跳过第0列 
        iCol = 1;
    }
    else
    {
        iCol = 0;
    }
    pFindBinding[0].iOrdinal   = rgColumnInfo[iCol].iOrdinal;
    pFindBinding[0].dwPart     = DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS;
    pFindBinding[0].obStatus   = 0;
    pFindBinding[0].obLength   = sizeof(DBSTATUS);
    pFindBinding[0].obValue    = sizeof(DBSTATUS) + sizeof(ULONG);
    pFindBinding[0].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
    pFindBinding[0].eParamIO   = DBPARAMIO_NOTPARAM;
    pFindBinding[0].bPrecision = rgColumnInfo[iCol].bPrecision;
    pFindBinding[0].bScale     = rgColumnInfo[iCol].bScale;
    pFindBinding[0].wType      = DBTYPE_WSTR;
    pFindBinding[0].cbMaxLen   = 7 * sizeof(WCHAR);

    ulFindDataLen = pFindBinding[0].cbMaxLen + pFindBinding[0].obValue;
    ulFindDataLen = GRS_ROUNDUP(ulFindDataLen);

    hr = pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,1,pFindBinding,0,&hFindAccessor,NULL);
    GRS_COM_CHECK(hr,_T("创建查找访问器失败，错误码：0x%08X\n"),hr);

    pFindData = (BYTE*)GRS_CALLOC(ulFindDataLen);

    *(DBSTATUS *)(pFindData + pFindBinding[0].obStatus) = DBSTATUS_S_OK;
    *(DBLENGTH *)(pFindData + pFindBinding[0].obLength) = 6 * sizeof(WCHAR);
    StringCchCopy((LPWSTR)(pFindData + pFindBinding[0].obValue),pFindBinding[0].cbMaxLen/sizeof(WCHAR),_T("120109")); 
    //hr = pIRowsetFind->FindNextRow(DB_NULL_HCHAPTER,hFindAccessor,pFindData,DBCOMPAREOPS_EQ,NULL,NULL,0,1,&cRowsObtained, &rghRows);
    //分配cRows行数据的缓冲，然后反复读取cRows行到这里，然后逐行处理之
    pData = GRS_CALLOC(dwOffset * cRows);
    lAllRows = 0;
    //查询数据
    while(S_OK == pIRowsetFind->FindNextRow(DB_NULL_HCHAPTER,hFindAccessor,pFindData
        ,DBCOMPAREOPS_LE,NULL,NULL,0,cRows,&cRowsObtained, &rghRows))
    {//循环读取数据，每次循环默认读取cRows行，实际读取到cRowsObtained行
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

    //==============================================================================

CLEAR_UP:
    if(NULL != pIAccessor)
    {
        if(NULL != hAccessor)
        {
            pIAccessor->ReleaseAccessor(hAccessor,NULL);
        }
        if(NULL != hFindAccessor )
        {
            pIAccessor->ReleaseAccessor(hFindAccessor,NULL);
        }
    }
    GRS_SAFEFREE(pFindData);
    GRS_SAFEFREE(pData);
    GRS_SAFEFREE(rgBindings);
    GRS_SAFEFREE(plDisplayLen);
    CoTaskMemFree(rgColumnInfo);
    CoTaskMemFree(pStringBuffer);
    GRS_SAFERELEASE(pIRowset);
    GRS_SAFERELEASE(pIAccessor);
    GRS_SAFERELEASE(pIColumnsInfo);
    GRS_SAFERELEASE(pICommandProperties);
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