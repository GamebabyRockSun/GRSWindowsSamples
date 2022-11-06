#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define COM_NO_WINDOWS_H    //����Ѿ�������Windows.h��ʹ������Windows�⺯��ʱ
#define DBINITCONSTANTS
#define INITGUID
#define OLEDBVER 0x0260		//Ϊ��ICommandStream�ӿڶ���Ϊ2.6��
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
    GRS_PRINTF(_T("��֧��'%s'�ӿ�\n"),_T(#iid));\
    }\
    else\
    {\
    GRS_PRINTF(_T("֧��'%s'�ӿ�\n"),_T(#iid));\
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
    LONG                cRows               = 10;//һ�ζ�ȡ10��
    BYTE *              pCurData			= NULL;

    ULONG*				plDisplayLen		= NULL;
    BOOL				bShowColumnName		= FALSE;

    DBSTATUS            dbstsDst            = DBSTATUS_S_OK;
    TCHAR               pwsDest[50]         = {};
    size_t              szDestLen           = 50 * sizeof(TCHAR);
    size_t              szRDstLen           = szDestLen;

    hr = CoCreateInstance(CLSID_OLEDB_CONVERSIONLIBRARY,NULL,CLSCTX_INPROC_SERVER
        ,IID_IDataConvert,(void**)&pIDataConvert);
    GRS_COM_CHECK(hr,_T("����IDataConvert�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    if(!CreateDBSession(pIOpenRowset))
    {
        _tsystem(_T("PAUSE"));
        return -1;
    }

    hr = pIOpenRowset->QueryInterface(IID_IDBCreateCommand,(void**)&pIDBCreateCommand);
    GRS_COM_CHECK(hr,_T("��ȡIDBCreateCommand�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pIDBCreateCommand->CreateCommand(NULL,IID_ICommand,(IUnknown**)&pICommand);
    GRS_COM_CHECK(hr,_T("����ICommand�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    //ִ��һ������
    hr = pICommand->QueryInterface(IID_ICommandText,(void**)&pICommandText);
    GRS_COM_CHECK(hr,_T("��ȡICommandText�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);
    hr = pICommandText->SetCommandText(DBGUID_DEFAULT,pSQL);
    GRS_COM_CHECK(hr,_T("����SQL���ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pICommandText->Execute(NULL,IID_IRowsetChange,NULL,NULL,(IUnknown**)&pIRowsetChange);
    GRS_COM_CHECK(hr,_T("ִ��SQL���ʧ�ܣ������룺0x%08X\n"),hr);

    //׼������Ϣ
    hr = pIRowsetChange->QueryInterface(IID_IColumnsInfo,(void**)&pIColumnsInfo);
    GRS_COM_CHECK(hr,_T("���IColumnsInfo�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);
    hr = pIColumnsInfo->GetColumnInfo(&cColumns, &rgColumnInfo,&pStringBuffer);
    GRS_COM_CHECK(hr,_T("��ȡ����Ϣʧ�ܣ������룺0x%08X\n"),hr);


    //����󶨽ṹ������ڴ�	
    rgBindings = (DBBINDING*)GRS_CALLOC(cColumns * sizeof(DBBINDING));
    //��������Ϣ ���󶨽ṹ��Ϣ���� ע���д�С����8�ֽڱ߽��������
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
        rgBindings[iCol].wType      = rgColumnInfo[iCol].wType;	//����ԭ������IDataConvertת��
        rgBindings[iCol].cbMaxLen   = rgColumnInfo[iCol].ulColumnSize * sizeof(WCHAR);
        dwOffset = rgBindings[iCol].cbMaxLen + rgBindings[iCol].obValue;
        dwOffset = GRS_ROUNDUP(dwOffset);
    }

    plDisplayLen = (ULONG*)GRS_CALLOC(cColumns * sizeof(ULONG));

    //�����з�����
    hr = pIRowsetChange->QueryInterface(IID_IAccessor,(void**)&pIAccessor);
    GRS_COM_CHECK(hr,_T("���IAccessor�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);
    hr = pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,cColumns,rgBindings,0,&hAccessor,NULL);
    GRS_COM_CHECK(hr,_T("�����з�����ʧ�ܣ������룺0x%08X\n"),hr);

    //��ȡ����
    hr = pIRowsetChange->QueryInterface(IID_IRowset,(void**)&pIRowset);
    GRS_COM_CHECK(hr,_T("���IRowset�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);
    //����cRows�����ݵĻ��壬Ȼ�󷴸���ȡcRows�е����Ȼ�����д���֮
    pData = (BYTE*)GRS_CALLOC(dwOffset * cRows);
    lAllRows = 0;
    do 
    {
        hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER,0,cRows,&cRowsObtained, &rghRows);
        //ѭ����ȡ���ݣ�ÿ��ѭ��Ĭ�϶�ȡcRows�У�ʵ�ʶ�ȡ��cRowsObtained��
        //ÿ������ʾһ������ʱ������0һ�¿������
        ZeroMemory(plDisplayLen,cColumns * sizeof(ULONG));
        for( iRow = 0; iRow < cRowsObtained; iRow++ )
        {
            //����ÿ��������ʾ���
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
        {//�ͷ��о������
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

    //1��ʹ��OLEDB���ݿ����ӶԻ������ӵ�����Դ
    HRESULT hr = CoCreateInstance(CLSID_DataLinks, NULL, CLSCTX_INPROC_SERVER,
        IID_IDBPromptInitialize, (void **)&pIDBPromptInitialize);
    if(FAILED(hr))
    {
        GRS_PRINTF(_T("�޷�����IDBPromptInitialize�ӿڣ������룺0x%08x\n"),hr);
        GRS_SAFERELEASE(pDBConnect);
        GRS_SAFERELEASE(pIDBPromptInitialize);
        return FALSE;
    }

    //������佫�������ݿ����ӶԻ���
    hr = pIDBPromptInitialize->PromptDataSource(NULL, hWndParent,
        DBPROMPTOPTIONS_PROPERTYSHEET, 0, NULL, NULL, IID_IDBInitialize,
        (IUnknown **)&pDBConnect);
    if( FAILED(hr) )
    {
        GRS_PRINTF(_T("�������ݿ�Դ�Ի�����󣬴����룺0x%08x\n"),hr);
        GRS_SAFERELEASE(pDBConnect);
        GRS_SAFERELEASE(pIDBPromptInitialize);
        return FALSE;
    }
    GRS_SAFERELEASE(pIDBPromptInitialize);

    hr = pDBConnect->Initialize();//���ݶԻ���ɼ��Ĳ������ӵ�ָ�������ݿ�
    if( FAILED(hr) )
    {
        GRS_PRINTF(_T("��ʼ������Դ���Ӵ��󣬴����룺0x%08x\n"),hr);
        GRS_SAFERELEASE(pDBConnect);
        return FALSE;
    }

    if(FAILED(hr = pDBConnect->QueryInterface(IID_IDBCreateSession,(void**)& pIDBCreateSession)))
    {
        GRS_PRINTF(_T("��ȡIDBCreateSession�ӿ�ʧ�ܣ������룺0x%08x\n"),hr);
        GRS_SAFERELEASE(pIDBCreateSession);
        GRS_SAFERELEASE(pDBConnect);
        return FALSE;
    }
    GRS_SAFERELEASE(pDBConnect);

    hr = pIDBCreateSession->CreateSession(NULL,IID_IOpenRowset,(IUnknown**)&pIOpenRowset);
    if(FAILED(hr))
    {
        GRS_PRINTF(_T("����Session�����ȡIOpenRowset�ӿ�ʧ�ܣ������룺0x%08x\n"),hr);
        GRS_SAFERELEASE(pDBConnect);
        GRS_SAFERELEASE(pIDBCreateSession);
        GRS_SAFERELEASE(pIOpenRowset);
        return FALSE;
    }
    //��������Դ���ӵĽӿڿ��Բ�Ҫ�ˣ���Ҫʱ����ͨ��Session�����IGetDataSource�ӿڻ�ȡ
    GRS_SAFERELEASE(pDBConnect);
    GRS_SAFERELEASE(pIDBCreateSession);
    return TRUE;
}
