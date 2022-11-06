#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#define OLEDBVER 0x0260
#include <oledb.h>
#include <oledberr.h>
#include <msdasc.h>

#import "C:\Program Files\Common Files\System\ado\msado15.dll" \
    no_namespace rename("EOF", "EndOfFile")

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

BOOL CreateDBSession(IOpenRowset*&pIOpenRowset);

int _tmain()
{   
    GRS_USEPRINTF();
    CoInitialize(NULL);
    try
    {        
        _bstr_t bsCnct(_T("Provider=sqloledb;Data Source=ASUS-PC\\SQL2008;\
                          Initial Catalog=Study;User Id=sa;Password=999999;")); 
        _RecordsetPtr  Rowset(__uuidof(Recordset));
        
        Rowset->Open(_T("SELECT top 200 * FROM T_State")
            ,bsCnct,adOpenStatic,adLockOptimistic,adCmdText);
        
        //获取IRowset接口指针
        ADORecordsetConstruction* padoRecordsetConstruct = NULL;
        Rowset->QueryInterface(__uuidof(ADORecordsetConstruction),
            (void **) &padoRecordsetConstruct);
        IRowset * pIRowset = NULL;
        padoRecordsetConstruct->get_Rowset((IUnknown **)&pIRowset);
        padoRecordsetConstruct->Release();
        
        DisplayRowSet(pIRowset);
        
        pIRowset->Release();


        GRS_PRINTF(_T("\n\n显示第二个结果集:\n"));

        //使用OLEDB方法打开一个结果集
        IOpenRowset*			pIOpenRowset				= NULL;
        TCHAR*					pszTableName				= _T("T_State");
        DBID					TableID						= {};

        CreateDBSession(pIOpenRowset);

        TableID.eKind           = DBKIND_NAME;
        TableID.uName.pwszName  = (LPOLESTR)pszTableName;

        HRESULT hr = pIOpenRowset->OpenRowset(NULL,&TableID,NULL,IID_IRowset,0,NULL,(IUnknown**)&pIRowset);
        if(FAILED(hr))
        {
            _com_raise_error(hr);
        }
        //创建一个新的ADO记录集对象
        _RecordsetPtr  Rowset2(__uuidof(Recordset));
        Rowset2->QueryInterface(__uuidof(ADORecordsetConstruction),
            (void **) &padoRecordsetConstruct);
        //将OLEDB的结果集放置到ADO记录集对象中
        padoRecordsetConstruct->put_Rowset(pIRowset);
        ULONG ulRow = 0;
        
        while(!Rowset2->EndOfFile)
        {
            GRS_PRINTF(_T("|%10u|%10s|%-40s|\n")
                ,++ulRow
                ,Rowset2->Fields->GetItem("K_StateCode")->Value.bstrVal
                ,Rowset2->Fields->GetItem("F_StateName")->Value.bstrVal);

            Rowset2->MoveNext();
        }
    }
    catch(_com_error & e)
    {
        GRS_PRINTF(_T("发生错误:\n Source : %s \n Description : %s \n")
            ,(LPCTSTR)e.Source(),(LPCTSTR)e.Description());
    }

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
