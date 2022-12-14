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
#define MAX_INPUT_LENGTH    10

BOOL CreateDBSession(IOpenRowset*&pIOpenRowset);

VOID DisplayRowSet(IRowset* pIRowset);
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
    HRESULT					hr						= S_OK;
    IOpenRowset*			pIOpenRowset			= NULL;
    IDBCreateCommand*		pIDBCreateCommand		= NULL;

    ICommand*				pICommand				= NULL;
    ICommandText*			pICommandText			= NULL;
    ICommandPrepare*        pICommandPrepare        = NULL;
    ICommandWithParameters* pICommandWithParameters = NULL;
    IAccessor*				pICommandAccessor		= NULL;

    GRS_DEF_INTERFACE(IRowset);
    //注意SQL语句里的？参数
    TCHAR* pSQL = _T("{? = call Usp_InputOutput(?,?)}");

    DB_UPARAMS			iParamCnt			= 0;
    DBPARAMINFO*		pParamInfo			= 0;
    LPWSTR				pStringBufferParam	= NULL;
    ULONG               dwOffset            = 0;
    DBBINDING *         rgParamBindings     = NULL;
    DBBINDSTATUS*		pBindingStatus		= NULL;
    HACCESSOR			hParamAccessor		= NULL;
    DBPARAMS			dbParam				= {};
    WCHAR               pInputBuf[MAX_INPUT_LENGTH] = {};
    DBROWCOUNT          cNumRows            = 0;

    if(!CreateDBSession(pIOpenRowset))
    {
        _tsystem(_T("PAUSE"));
        return -1;
    }

    hr = pIOpenRowset->QueryInterface(IID_IDBCreateCommand,(void**)&pIDBCreateCommand);
    GRS_COM_CHECK(hr,_T("获取IDBCreateCommand接口失败，错误码：0x%08X\n"),hr);

    hr = pIDBCreateCommand->CreateCommand(NULL,IID_ICommand,(IUnknown**)&pICommand);
    GRS_COM_CHECK(hr,_T("创建ICommand接口失败，错误码：0x%08X\n"),hr);

    //执行命令
    hr = pICommand->QueryInterface(IID_ICommandText,(void**)&pICommandText);
    GRS_COM_CHECK(hr,_T("获取ICommandText接口失败，错误码：0x%08X\n"),hr);
    hr = pICommandText->SetCommandText(DBGUID_DBSQL,pSQL);
    GRS_COM_CHECK(hr,_T("设置SQL语句失败，错误码：0x%08X\n"),hr);

    hr = pICommandText->QueryInterface(IID_ICommandPrepare,(void**)&pICommandPrepare);
    GRS_COM_CHECK(hr,_T("获取ICommandPrepare接口失败，错误码：0x%08X\n"),hr);
    //预处理命令，解析出参数
    hr = pICommandPrepare->Prepare(0);
    GRS_COM_CHECK(hr,_T("调用Prepare方法失败，错误码：0x%08X\n"),hr);

    //ICommandPrepare接口可以释放了
    GRS_SAFERELEASE(pICommandPrepare);

    hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParameters);
    GRS_COM_CHECK(hr,_T("获取ICommandWithParameters接口失败，错误码：0x%08X\n"),hr);
    //获得参数个数和参数信息结构数组
    hr = pICommandWithParameters->GetParameterInfo(&iParamCnt,&pParamInfo,&pStringBufferParam);
    GRS_COM_CHECK(hr,_T("获取查询的参数信息失败，错误码：0x%08X\n"),hr);

    if( iParamCnt > 0 )
    {
        //从Command对象获得IAccessor接口这与从Rowset对象获得IAccessor接口所属对象和用途不同
        hr = pICommandText->QueryInterface(IID_IAccessor,(void**)&pICommandAccessor);
        GRS_COM_CHECK(hr,_T("获取Command对象的IAccessor接口失败，错误码：0x%08X\n"),hr);

        rgParamBindings = (DBBINDING*)GRS_CALLOC(iParamCnt * sizeof(DBBINDING));

        for(ULONG i = 0;i < iParamCnt; i++)
        {
            rgParamBindings[i].iOrdinal   = pParamInfo[i].iOrdinal;
            rgParamBindings[i].dwPart	  = DBPART_VALUE ;//参数绑定值
            rgParamBindings[i].obStatus   = 0;
            rgParamBindings[i].obLength   = 0;
            rgParamBindings[i].obValue    = dwOffset;
            rgParamBindings[i].dwMemOwner = DBMEMOWNER_CLIENTOWNED;

            if( pParamInfo[i].dwFlags & DBPARAMFLAGS_ISINPUT )
            {
                rgParamBindings[i].eParamIO |= DBPARAMIO_INPUT;				//输入型参数
            }

            if( pParamInfo[i].dwFlags & DBPARAMFLAGS_ISOUTPUT )
            {
                rgParamBindings[i].eParamIO |= DBPARAMIO_OUTPUT;	
            }
            
            rgParamBindings[i].bPrecision = 11;//pParamInfo[i].bPrecision;
            rgParamBindings[i].bScale     = pParamInfo[i].bScale;
            rgParamBindings[i].wType      = pParamInfo[i].wType;

            //需要注意的是pPatamInfo结构中ulParamSize的长度实际是？占位符的长度
            //并不是实际条件字段的长度，因此需要自己指定比较合适的长度
            rgParamBindings[i].cbMaxLen   = pParamInfo[i].ulParamSize;

            dwOffset = rgParamBindings[i].cbMaxLen + rgParamBindings[i].obValue;
            //dwOffset = GRS_ROUNDUP(dwOffset);
        } 

        hr = pICommandAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,
            iParamCnt,rgParamBindings,dwOffset,&hParamAccessor, NULL);
        
        GRS_COM_CHECK(hr,_T("创建参数访问器失败，错误码：0x%08X\n"),hr);

        //准备参数
        dbParam.cParamSets = 1;
        dbParam.hAccessor = hParamAccessor;
        dbParam.pData = GRS_CALLOC(dwOffset);

        for(ULONG i = 0; i < iParamCnt; i++)
        {
            if( !(pParamInfo[i].dwFlags & DBPARAMFLAGS_ISINPUT) )
            {
                continue;
            }
            GRS_PRINTF(_T("请输入第[%ld]个参数[%s]:")
                ,i+1
                ,pParamInfo[i].pwszName);

            while(S_OK != StringCchGets(pInputBuf,MAX_INPUT_LENGTH));

            //转换成整形
            *(int*)((BYTE*)dbParam.pData + rgParamBindings[i].obValue) = _ttoi(pInputBuf);
            //*((ULONG*)((BYTE*)dbParam.pData + rgParamBindings[i].obLength)) = rgParamBindings[i].cbMaxLen;
        }
    }

    hr = pICommandText->Execute(NULL,IID_IRowset,&dbParam,&cNumRows,(IUnknown**)&pIRowset);
    GRS_COM_CHECK(hr,_T("执行SQL语句失败，错误码：0x%08X\n"),hr);
    
    GRS_PRINTF(_T("\n存储过程返回结果集:\n"));
    //显示结果集
    DisplayRowSet(pIRowset);

    //释放结果集接口,此时输出参数就被写到了缓冲中
    GRS_SAFERELEASE(pIRowset);

    GRS_PRINTF(_T("\n"));
    for(ULONG i = 0; i < iParamCnt; i++)
    {
        if( !(pParamInfo[i].dwFlags & DBPARAMFLAGS_ISOUTPUT) )
        {
            continue;
        }

        GRS_PRINTF(_T("第[%ld]个参数输出值[%d]\n")
            ,i+1
            ,*(int*)((BYTE*)dbParam.pData + rgParamBindings[i].obValue) );
    }
    

CLEAR_UP:
    GRS_SAFERELEASE(pIRowset);
    
    if(hParamAccessor && pICommandAccessor)
    {
        pICommandAccessor->ReleaseAccessor(hParamAccessor,NULL);
    }
    
    CoTaskMemFree(pParamInfo);
    CoTaskMemFree(pStringBufferParam);

    GRS_SAFERELEASE(pICommandWithParameters);
    GRS_SAFERELEASE(pICommandPrepare);
    GRS_SAFERELEASE(pICommand);
    GRS_SAFERELEASE(pICommandText);
    GRS_SAFERELEASE(pIOpenRowset);
    GRS_SAFERELEASE(pIDBCreateCommand);

    //这个缓冲要在pIRowset等资源释放后释放
    GRS_SAFEFREE(dbParam.pData);

    _tsystem(_T("PAUSE"));
    CoUninitialize();
    return 0;
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