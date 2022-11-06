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
#include <msdadc.h>   // for IDataConvert
#include <msdaguid.h>   // for IDataConvert

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

int _tmain()
{
    GRS_USEPRINTF();
    CoInitialize(NULL);
    HRESULT				hr					= S_OK;
    IDataConvert*       pIDataConvert       = NULL;
    IOpenRowset*		pIOpenRowset		= NULL;
    IDBCreateCommand*	pIDBCreateCommand	= NULL;
    ICommand*			pICommand			= NULL;
    ICommandText*		pICommandText		= NULL;
    ICommandProperties* pICommandProperties = NULL;
    GRS_DEF_INTERFACE(IAccessor);
    GRS_DEF_INTERFACE(IColumnsInfo);
    GRS_DEF_INTERFACE(IRowset);
    GRS_DEF_INTERFACE(IRowsetChange);

    TCHAR* pSQL = _T("Select * From T_Decimal");

    ULONG               cColumns;
    DBCOLUMNINFO *      rgColumnInfo        = NULL;
    LPWSTR              pStringBuffer       = NULL;
    ULONG               iCol				= 0;
    ULONG               dwOffset            = 0;
    DBBINDING *         rgBindings          = NULL;
    HACCESSOR			hAccessor			= NULL;
    BYTE *              pData               = NULL;
    ULONG               cRowsObtained		= 0;
    HROW *              rghRows             = NULL;
    ULONG               iRow				= 0;
    ULONG				lAllRows			= 0;
    LONG                cRows               = 10;//一次读取10行
    BYTE *              pCurData			= NULL;

    ULONG*				plDisplayLen		= NULL;
    BOOL				bShowColumnName		= FALSE;

    DBSTATUS            dbstsDst            = DBSTATUS_S_OK;
    TCHAR               pwsDest[50]         = {};
    size_t              szDestLen           = 50 * sizeof(TCHAR);
    size_t              szRDstLen           = szDestLen;

    hr = CoCreateInstance(CLSID_OLEDB_CONVERSIONLIBRARY,NULL,CLSCTX_INPROC_SERVER
        ,IID_IDataConvert,(void**)&pIDataConvert);
    GRS_COM_CHECK(hr,_T("创建IDataConvert接口失败，错误码：0x%08X\n"),hr);

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

    hr = pICommandText->Execute(NULL,IID_IRowsetChange,NULL,NULL,(IUnknown**)&pIRowsetChange);
    GRS_COM_CHECK(hr,_T("执行SQL语句失败，错误码：0x%08X\n"),hr);

    //准备绑定信息
    hr = pIRowsetChange->QueryInterface(IID_IColumnsInfo,(void**)&pIColumnsInfo);
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
        rgBindings[iCol].wType      = rgColumnInfo[iCol].wType;	//保持原类型用IDataConvert转换
        rgBindings[iCol].cbMaxLen   = rgColumnInfo[iCol].ulColumnSize * sizeof(WCHAR);
        dwOffset = rgBindings[iCol].cbMaxLen + rgBindings[iCol].obValue;
        dwOffset = GRS_ROUNDUP(dwOffset);
    }

    plDisplayLen = (ULONG*)GRS_CALLOC(cColumns * sizeof(ULONG));

    //创建行访问器
    hr = pIRowsetChange->QueryInterface(IID_IAccessor,(void**)&pIAccessor);
    GRS_COM_CHECK(hr,_T("获得IAccessor接口失败，错误码：0x%08X\n"),hr);
    hr = pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,cColumns,rgBindings,0,&hAccessor,NULL);
    GRS_COM_CHECK(hr,_T("创建行访问器失败，错误码：0x%08X\n"),hr);

    //获取数据
    hr = pIRowsetChange->QueryInterface(IID_IRowset,(void**)&pIRowset);
    GRS_COM_CHECK(hr,_T("获得IRowset接口失败，错误码：0x%08X\n"),hr);
    //分配cRows行数据的缓冲，然后反复读取cRows行到这里，然后逐行处理之
    pData = (BYTE*)GRS_CALLOC(dwOffset * cRows);
    lAllRows = 0;
    do 
    {
        hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER,0,cRows,&cRowsObtained, &rghRows);
        //循环读取数据，每次循环默认读取cRows行，实际读取到cRowsObtained行
        //每次新显示一批数据时都先清0一下宽度数组
        ZeroMemory(plDisplayLen,cColumns * sizeof(ULONG));
        for( iRow = 0; iRow < cRowsObtained; iRow++ )
        {
            //计算每列最大的显示宽度
            pCurData    = pData + (dwOffset * iRow);
            pIRowset->GetData(rghRows[iRow],hAccessor,pCurData);
            GRS_PRINTF(_T("[Row:%09d] |"),++lAllRows);

            for(iCol = 0; iCol < cColumns; iCol++)
            {
                ZeroMemory(pwsDest,szDestLen);

                hr = pIDataConvert->DataConvert(rgBindings[iCol].wType,     // wSrcType
                    DBTYPE_WSTR,                                            // wDstType
                    *(ULONG*)(pCurData + rgBindings[iCol].obLength),        // cbSrcLength 
                    (DBLENGTH*)&szRDstLen,                                  // pcbDstLength
                    pCurData + rgBindings[iCol].obValue,                   // pSrc
                    pwsDest,                                                // pDst
                    szDestLen,                                              //cbDstMaxLength
                    *(DBSTATUS*)(pCurData + rgBindings[iCol].obStatus),     // dbsSrcStatus
                    &dbstsDst,                                              // pdbsStatus
                    0,                                                      // bPrecision (used for DBTYPE_NUMERIC only)
                    0,                                                      // bScale (used for DBTYPE_NUMERIC only)
                    DBDATACONVERT_DEFAULT);                                 // dwFlags

                if(SUCCEEDED(hr))
                {
                    GRS_PRINTF(_T(" %18s |"),pwsDest);
                }
                else
                {
                    GRS_PRINTF(_T(" [Can't Convert] |"))
                }
            }

            GRS_PRINTF(_T("\n"));
        }

        if( cRowsObtained )
        {//释放行句柄数组
            pIRowset->ReleaseRows(cRowsObtained,rghRows,NULL,NULL,NULL);
        }
        bShowColumnName = FALSE;
        CoTaskMemFree(rghRows);
        rghRows = NULL;
    } while (S_OK == hr);
CLEAR_UP:
    if(NULL != pIAccessor)
    {
        pIAccessor->ReleaseAccessor(hAccessor,NULL);
    }
    GRS_SAFEFREE(pData);
    GRS_SAFEFREE(rgBindings);
    GRS_SAFEFREE(plDisplayLen);
    CoTaskMemFree(rgColumnInfo);
    CoTaskMemFree(pStringBuffer);
    GRS_SAFERELEASE(pIRowset);
    GRS_SAFERELEASE(pIAccessor);
    GRS_SAFERELEASE(pIColumnsInfo);
    GRS_SAFERELEASE(pIRowsetChange);
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
