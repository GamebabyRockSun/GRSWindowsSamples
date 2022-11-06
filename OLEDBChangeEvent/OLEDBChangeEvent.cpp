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

//���ߺ� ���ڽ�֪ͨ�¼���ԭ��Ͳ�������ֵתΪ�ַ���
#define GRS_VAL2WSTR_LEN    40
#define GRS_VAL2WSTR(val,cst,psz) if((val) == (cst)){StringCchCopy(psz,GRS_VAL2WSTR_LEN,_T(#cst));}

BOOL CreateDBSession(IOpenRowset*&pIOpenRowset);
VOID DisplayColumnNames(DBORDINAL        ulRowsetCols
                        ,DBCOLUMNINFO*   pColInfo
                        ,ULONG*          rgDispSize );
HRESULT DisplayRow(DBCOUNTITEM iRow,
                   DBCOUNTITEM cBindings,
                   DBBINDING * pChangeBindings,
                   void * pChangeData,
                   ULONG * rgDispSize );

VOID UpdateDisplaySize (DBCOUNTITEM   cBindings,
                        DBBINDING *   pChangeBindings,
                        void *        pChangeData,
                        ULONG *       rgDispSize );

VOID GetDBEventPhaseName(DBEVENTPHASE dbPhase,LPWSTR pszPhaseName);
VOID GetDBReasonName(DBREASON dbPhase,LPWSTR pszReasonName);

class CGRSRowsetNotify:
    public IRowsetNotify,		//Rowset Notifications
    public IDBAsynchNotify,		//Asynch Notifications
    public IRowPositionChange	//RowPosition Notifications
{
public:
    CGRSRowsetNotify()
    {
    }
    virtual ~CGRSRowsetNotify()
    {
    }
public:
    virtual HRESULT			Advise(IUnknown* pIUnknown,REFIID riid);
    virtual HRESULT			Unadvise(IUnknown* pIUnknown,REFIID riid);
protected:
    virtual HRESULT			Advise(IConnectionPoint* pIConnectionPoint,REFIID riid);
    virtual HRESULT			Unadvise(IConnectionPoint* pIConnectionPoint);
public:
    STDMETHODIMP_(ULONG)	AddRef(void);
    STDMETHODIMP_(ULONG)	Release(void);
    STDMETHODIMP			QueryInterface(REFIID riid, LPVOID *ppv);
public://�м����֪ͨ�ص��ӿ�
    //IRowsetNotify
    STDMETHODIMP OnFieldChange
        ( 
        IRowset* pIRowset,
        HROW hRow,
        DBORDINAL cColumns,
        DBORDINAL rgColumns[  ],
        DBREASON eReason,
        DBEVENTPHASE ePhase,
        BOOL fCantDeny
        );

    STDMETHODIMP OnRowChange
        ( 
        IRowset* pIRowset,
        DBCOUNTITEM cRows,
        const HROW rghRows[  ],
        DBREASON eReason,
        DBEVENTPHASE ePhase,
        BOOL fCantDeny
        );

    STDMETHODIMP OnRowsetChange
        ( 
        IRowset* pIRowset,
        DBREASON eReason,
        DBEVENTPHASE ePhase,
        BOOL fCantDeny
        );

public://���½ӿ������첽���ݿ������ʽ�Ļص��ӿ�ʵ��
    //IDBAsynchNotify
    STDMETHODIMP OnLowResource
        ( 
        DB_DWRESERVE dwReserved
        );

    STDMETHODIMP OnProgress
        ( 
        HCHAPTER		hChapter,
        DBASYNCHOP		eOperation,
        DBCOUNTITEM		ulProgress,
        DBCOUNTITEM		ulProgressMax,
        DBASYNCHPHASE	eAsynchPhase,
        LPOLESTR pwszStatusText
        );

    STDMETHODIMP OnStop
        ( 
        HCHAPTER		hChapter,
        DBASYNCHOP		eOperation,
        HRESULT			hrStatus,
        LPOLESTR		pwszStatusText
        );
public://���½ӿ����ڸı���λ��֮��Ļص��ӿ�
    //IRowPositionChange
    STDMETHODIMP OnRowPositionChange
        (
        DBREASON			eReason,
        DBEVENTPHASE		ePhase,
        BOOL				fCantDeny
        );
protected:
    //Data
    ULONG			m_cRef;			// reference count
    DWORD           m_dwCookie;     // Cookie
};

HRESULT CGRSRowsetNotify::Advise(IUnknown* pIUnknown,REFIID riid)
{
    GRS_USEPRINTF();
    IConnectionPointContainer* pICpc = NULL;
    IConnectionPoint* pICPoint = NULL;
    HRESULT hr = pIUnknown->QueryInterface(IID_IConnectionPointContainer,(void**)&pICpc);
    if(FAILED(hr))
    {
        GRS_PRINTF(_T("ͨ��IRowset�ӿڻ�ȡIConnectionPointContainer�ӿ�ʧ��,������:0x%08X\n"),hr);
        return hr;
    }
    hr = pICpc->FindConnectionPoint(riid,&pICPoint);
    if(FAILED(hr))
    {
        GRS_PRINTF(_T("��ȡIConnectionPoint�ӿ�ʧ��,������:0x%08X\n"),hr);
        GRS_SAFERELEASE(pICpc);
        return hr;
    }
    GRS_SAFERELEASE(pICpc);

    hr = Advise(pICPoint,riid);
    GRS_SAFERELEASE(pICPoint);
    return hr;
}   
HRESULT CGRSRowsetNotify::Advise(IConnectionPoint* pIConnectionPoint,REFIID riid)
{
    GRS_USEPRINTF();
    if(NULL == pIConnectionPoint)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_FALSE;
    
    if(IID_IRowsetNotify == riid)
    {
        hr = pIConnectionPoint->Advise(dynamic_cast<IRowsetNotify*>(this), &m_dwCookie);
    }
    if(IID_IDBAsynchNotify == riid)
    {
        hr = pIConnectionPoint->Advise(dynamic_cast<IDBAsynchNotify*>(this), &m_dwCookie);
    }
    if(IID_IRowPositionChange == riid)
    {
        hr = pIConnectionPoint->Advise(dynamic_cast<IRowPositionChange*>(this), &m_dwCookie);
    }

    if(FAILED(hr))
    {
        GRS_PRINTF(_T("IConnectionPoint::Advise��������ʧ��,������:0x%08X\n"),hr);
        return hr;
    }

    return S_OK;
}

HRESULT CGRSRowsetNotify::Unadvise(IUnknown* pIUnknown,REFIID riid)
{
    GRS_USEPRINTF();
    IConnectionPointContainer* pICpc = NULL;
    IConnectionPoint* pICPoint = NULL;
    HRESULT hr = pIUnknown->QueryInterface(IID_IConnectionPointContainer,(void**)&pICpc);
    if(FAILED(hr))
    {
        GRS_PRINTF(_T("ͨ��IRowset�ӿڻ�ȡIConnectionPointContainer�ӿ�ʧ��,������:0x%08X\n"),hr);
        return hr;
    }
    hr = pICpc->FindConnectionPoint(riid,&pICPoint);
    if(FAILED(hr))
    {
        GRS_PRINTF(_T("��ȡIConnectionPoint�ӿ�ʧ��,������:0x%08X\n"),hr);
        GRS_SAFERELEASE(pICpc);
        return hr;
    }
    GRS_SAFERELEASE(pICpc);

    hr = Unadvise(pICPoint);
    GRS_SAFERELEASE(pICPoint);
    return hr;
}
HRESULT CGRSRowsetNotify::Unadvise(IConnectionPoint* pIConnectionPoint)
{
    GRS_USEPRINTF();
    if(NULL == pIConnectionPoint)
    {
        return E_INVALIDARG;
    }
    HRESULT hr = pIConnectionPoint->Unadvise(m_dwCookie);
    if(FAILED(hr))
    {
        GRS_PRINTF(_T("IConnectionPoint::Unadvise��������ʧ��,������:0x%08X\n"),hr);
        return hr;
    }
    return S_OK;
}

STDMETHODIMP_(ULONG)	CGRSRowsetNotify::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)	CGRSRowsetNotify::Release()
{
    if(--m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

STDMETHODIMP CGRSRowsetNotify::QueryInterface(REFIID riid, LPVOID* ppv)
{
    if(!ppv)
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if(riid == IID_IUnknown)
    {
        *ppv = (IUnknown*)(IRowsetNotify*)this;
    }
    else if(riid == IID_IRowsetNotify)
    {
        *ppv = (IRowsetNotify*)this;
    }
    else if(riid == IID_IDBAsynchNotify)
    {
        *ppv = (IDBAsynchNotify*)this;
    }
    else if(riid == IID_IRowPositionChange)
    {
        *ppv = (IRowPositionChange*)this;
    }

    if(*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }	
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP CGRSRowsetNotify::OnFieldChange
( 
 /* [in] */ IRowset __RPC_FAR* pIRowset,
 /* [in] */ HROW hRow,
 /* [in] */ DBORDINAL cColumns,
 /* [size_is][in] */ DBORDINAL __RPC_FAR rgColumns[  ],
 /* [in] */ DBREASON eReason,
 /* [in] */ DBEVENTPHASE ePhase,
 /* [in] */ BOOL fCantDeny
 )
{
    GRS_USEPRINTF();
    WCHAR pszReason[GRS_VAL2WSTR_LEN] = {};
    WCHAR pszPhase[GRS_VAL2WSTR_LEN] = {};

    GetDBReasonName(eReason,(LPWSTR)pszReason);
    GetDBEventPhaseName(ePhase,(LPWSTR)pszPhase);

    GRS_PRINTF(_T("CGRSRowsetNotify::OnFieldChange[%lu Cols] Reason:%-40s Phase:%-40s\n")
        ,cColumns,pszReason,pszPhase);

    //if(DBREASON_COLUMN_SET == eReason && DBEVENTPHASE_OKTODO == ePhase)
    //{
    //    return S_FALSE;
    //}
    return S_OK;
}

STDMETHODIMP CGRSRowsetNotify::OnRowChange
( 
 /* [in] */ IRowset __RPC_FAR* pIRowset,
 /* [in] */ DBCOUNTITEM cRows,
 /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
 /* [in] */ DBREASON eReason,
 /* [in] */ DBEVENTPHASE ePhase,
 /* [in] */ BOOL fCantDeny
 )
{
    GRS_USEPRINTF();
    WCHAR pszReason[GRS_VAL2WSTR_LEN] = {};
    WCHAR pszPhase[GRS_VAL2WSTR_LEN] = {};

    GetDBReasonName(eReason,(LPWSTR)pszReason);
    GetDBEventPhaseName(ePhase,(LPWSTR)pszPhase);

    GRS_PRINTF(_T("CGRSRowsetNotify::OnRowChange  [%lu Rows] Reason:%-40s Phase:%-40s\n")
        ,cRows,pszReason,pszPhase);
    return S_OK;
}

STDMETHODIMP CGRSRowsetNotify::OnRowsetChange
( 
 /* [in] */ IRowset __RPC_FAR* pIRowset,
 /* [in] */ DBREASON eReason,
 /* [in] */ DBEVENTPHASE ePhase,
 /* [in] */ BOOL fCantDeny)
{
    GRS_USEPRINTF();
    WCHAR pszReason[GRS_VAL2WSTR_LEN] = {};
    WCHAR pszPhase[GRS_VAL2WSTR_LEN] = {};

    GetDBReasonName(eReason,(LPWSTR)pszReason);
    GetDBEventPhaseName(ePhase,(LPWSTR)pszPhase);

    GRS_PRINTF(_T("CGRSRowsetNotify::OnRowsetChange,       Reason:%-40s Phase:%-40s\n")
        ,pszReason,pszPhase);
    return S_OK;
}

STDMETHODIMP CGRSRowsetNotify::OnLowResource(DB_DWRESERVE dwReserved)
{
    return S_OK;
}

STDMETHODIMP CGRSRowsetNotify::OnProgress
( 
 HCHAPTER		hChapter,
 DBASYNCHOP		eOperation,
 DBCOUNTITEM		ulProgress,
 DBCOUNTITEM		ulProgressMax,
 DBASYNCHPHASE	eAsynchPhase,
 LPOLESTR		pwszStatusText
 )
{
    //Output Notification
    //return TRACE_NOTIFICATION(NOTIFY_IDBASYNCHNOTIFY, m_hrReturn, L"IDBAsynchNotify", L"OnProgress", L"(0x%p, %s, %lu, %lu, %s, %s)", hChapter, GetAsynchReason(eOperation), ulProgress, ulProgressMax, GetAsynchPhase(eAsynchPhase), pwszStatusText);
    return S_OK;
}

STDMETHODIMP CGRSRowsetNotify::OnStop
( 
 HCHAPTER hChapter,
 ULONG    ulOperation,
 HRESULT  hrStatus,
 LPOLESTR pwszStatusText
 )
{
    //Output Notification
    //return TRACE_NOTIFICATION(NOTIFY_IDBASYNCHNOTIFY, m_hrReturn, L"IDBAsynchNotify", L"OnStop", L"(0x%p, %s, %S, %s)", hChapter, GetAsynchReason(ulOperation), GetErrorName(hrStatus), pwszStatusText);
    return S_OK;
}

STDMETHODIMP CGRSRowsetNotify::OnRowPositionChange(
    DBREASON		eReason,
    DBEVENTPHASE	ePhase,
    BOOL			fCantDeny)
{
    //Output Notification
    //return TRACE_NOTIFICATION(NOTIFY_IROWPOSITIONCHANGE, m_hrReturn, L"IRowPositionChange", L"OnRowPositionChange", L"(%s, %s, %s)", GetReasonName(eReason), GetPhaseName(ePhase), fCantDeny ? L"TRUE" : L"FALSE");
    return S_OK;
}

int _tmain()
{
    GRS_USEPRINTF();
    //CoInitializeEx(NULL,COINIT_MULTITHREADED);
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
    GRS_DEF_INTERFACE(IRowsetChange);
    GRS_DEF_INTERFACE(IRowsetUpdate);

    TCHAR* pSQL = _T("Select top 10 * From T_State");
    DBPROPSET ps[1] = {};
    DBPROP prop[20]	= {};

    ULONG               ulRowsetCols		= 0;
    ULONG				ulRealCols			= 0;
    DBCOLUMNINFO*		pColInfo			= NULL;
    LPWSTR              pStringBuffer       = NULL;
    ULONG               iCol				= 0;
    ULONG               ulChangeOffset      = 0;
    ULONG				ulShowOffset		= 0;
    DBBINDING*			pChangeBindings     = NULL;// �������ݸ��µİ�
    DBBINDING*			pShowBindings		= NULL;// ����������ʾ�İ�
    DBROWSTATUS			parRowStatus[1]		= {};
    HACCESSOR			hChangeAccessor		= NULL;
    HACCESSOR			hShowAccessor		= NULL;
    void*				pChangeData         = NULL;
    void*				pCurChangeData		= NULL;
    void*				pShowData			= NULL;
    void*				pCurShowData		= NULL;
    ULONG               pRowsObtained		= 0;
    HROW *              phRows				= NULL;
    HROW				hNewRow				= NULL;
    ULONG               iRow				= 0;
    ULONG				ulAllRows			= 0;
    LONG                lReadRows           = 2;//һ�ζ�ȡ2��
    DBLENGTH            ulLength			= 0;
    WCHAR*				pwsValue			= NULL;
    BYTE*				pbNewData			= NULL;
    ULONG*				plDisplayLen		= NULL;
    BOOL				bShowColumnName		= FALSE;
    DBROWSTATUS*        pUpdateStatus       = NULL;
    CGRSRowsetNotify    RowsetNotify;

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

    //��������
    prop[0].dwPropertyID    = DBPROP_UPDATABILITY;
    prop[0].vValue.vt       = VT_I4;
    //����ִ�к�Ľ�������Խ�����ɾ�Ĳ���
    prop[0].vValue.lVal     = DBPROPVAL_UP_CHANGE     //��Update����
        |DBPROPVAL_UP_DELETE					//��Delete����
        |DBPROPVAL_UP_INSERT;					//��Insert����
    prop[0].dwOptions       = DBPROPOPTIONS_REQUIRED;
    prop[0].colid = DB_NULLID;

    //��IRowsetUpdate�ӿ�Ҳ����֧���ӳٵĸ�������
    prop[1].dwPropertyID    = DBPROP_IRowsetUpdate;
    prop[1].vValue.vt       = VT_BOOL;
    prop[1].vValue.boolVal  = VARIANT_TRUE;         
    prop[1].dwOptions       = DBPROPOPTIONS_REQUIRED;
    prop[1].colid           = DB_NULLID;

    //���������޸����������������Ҫ����Ϊ150��
    prop[2].dwPropertyID    = DBPROP_MAXPENDINGROWS;
    prop[2].vValue.vt       = VT_I4;
    prop[2].vValue.intVal   = 150;         
    prop[2].dwOptions       = DBPROPOPTIONS_REQUIRED;
    prop[2].colid           = DB_NULLID;

    //�����������Ҳ��Ҫ���䣬��������ô�Ͳ��ܼ�����ɾ����ͬʱ��������
    prop[3].dwPropertyID    = DBPROP_CANHOLDROWS;
    prop[3].vValue.vt       = VT_BOOL;
    prop[3].vValue.boolVal  = VARIANT_TRUE;         
    prop[3].dwOptions       = DBPROPOPTIONS_REQUIRED;
    prop[3].colid           = DB_NULLID;

  
    prop[4].dwPropertyID    = DBPROP_IConnectionPointContainer;
    prop[4].vValue.vt       = VT_BOOL;
    prop[4].vValue.boolVal  = VARIANT_TRUE;         
    prop[4].dwOptions       = DBPROPOPTIONS_REQUIRED;
    prop[4].colid           = DB_NULLID;

    ps[0].guidPropertySet   = DBPROPSET_ROWSET;       //ע�����Լ��ϵ�����
    ps[0].cProperties       = 5;							//���Լ������Ը���
    ps[0].rgProperties      = prop;

    //�������� ִ����� ����м�
    hr = pICommandText->QueryInterface(IID_ICommandProperties,(void**)& pICommandProperties);
    GRS_COM_CHECK(hr,_T("��ȡICommandProperties�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    hr = pICommandProperties->SetProperties(1,ps);//ע�������Executeǰ�趨����
    GRS_COM_CHECK(hr,_T("����Command��������ʧ�ܣ������룺0x%08X\n"),hr);
    
    hr = pICommandText->Execute(NULL,IID_IRowsetChange,NULL,NULL,(IUnknown**)&pIRowsetChange);
    GRS_COM_CHECK(hr,_T("ִ��SQL���ʧ�ܣ������룺0x%08X\n"),hr);

    //���IRowsetUpdate�ӿ�
    hr = pIRowsetChange->QueryInterface(IID_IRowsetUpdate,(void**)&pIRowsetUpdate);
    GRS_COM_CHECK(hr,_T("��ȡIRowsetUpdate�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    //׼������Ϣ
    hr = pIRowsetChange->QueryInterface(IID_IColumnsInfo,(void**)&pIColumnsInfo);
    GRS_COM_CHECK(hr,_T("���IColumnsInfo�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);
    
    hr = pIColumnsInfo->GetColumnInfo(&ulRowsetCols, &pColInfo,&pStringBuffer);
    GRS_COM_CHECK(hr,_T("��ȡ����Ϣʧ�ܣ������룺0x%08X\n"),hr);

    //����󶨽ṹ������ڴ�	
    pChangeBindings = (DBBINDING*)GRS_CALLOC(ulRowsetCols * sizeof(DBBINDING));
    pShowBindings	= (DBBINDING*)GRS_CALLOC(ulRowsetCols * sizeof(DBBINDING));

    if( 0 == pColInfo[0].iOrdinal )
    {//��0�� �Ͳ�Ҫ���� ��ֹ����Ĳ��벻�ɹ�
        ulRealCols = ulRowsetCols - 1;
        for( iCol = 1; iCol < ulRowsetCols; iCol++ )
        {
            pChangeBindings[iCol - 1].iOrdinal   = pColInfo[iCol].iOrdinal;
            pChangeBindings[iCol - 1].dwPart     = DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS;
            pChangeBindings[iCol - 1].obStatus   = ulChangeOffset;
            pChangeBindings[iCol - 1].obLength   = ulChangeOffset + sizeof(DBSTATUS);
            pChangeBindings[iCol - 1].obValue    = ulChangeOffset + sizeof(DBSTATUS) + sizeof(ULONG);
            pChangeBindings[iCol - 1].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
            pChangeBindings[iCol - 1].eParamIO   = DBPARAMIO_NOTPARAM;
            pChangeBindings[iCol - 1].bPrecision = pColInfo[iCol].bPrecision;
            pChangeBindings[iCol - 1].bScale     = pColInfo[iCol].bScale;
            pChangeBindings[iCol - 1].wType      = DBTYPE_WSTR;//pColInfo[iCol].wType;	//ǿ��תΪWSTR
            pChangeBindings[iCol - 1].cbMaxLen   = pColInfo[iCol].ulColumnSize * sizeof(WCHAR);
            ulChangeOffset = pChangeBindings[iCol - 1].cbMaxLen + pChangeBindings[iCol - 1].obValue;
            ulChangeOffset = GRS_ROUNDUP(ulChangeOffset);
        }
    }
    else
    {
        ulRealCols = ulRowsetCols;
        //��������Ϣ ���󶨽ṹ��Ϣ���� ע���д�С����8�ֽڱ߽��������
        for( iCol = 0; iCol < ulRowsetCols; iCol++ )
        {
            pChangeBindings[iCol].iOrdinal   = pColInfo[iCol].iOrdinal;
            pChangeBindings[iCol].dwPart     = DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS;
            pChangeBindings[iCol].obStatus   = ulChangeOffset;
            pChangeBindings[iCol].obLength   = ulChangeOffset + sizeof(DBSTATUS);
            pChangeBindings[iCol].obValue    = ulChangeOffset + sizeof(DBSTATUS) + sizeof(ULONG);
            pChangeBindings[iCol].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
            pChangeBindings[iCol].eParamIO   = DBPARAMIO_NOTPARAM;
            pChangeBindings[iCol].bPrecision = pColInfo[iCol].bPrecision;
            pChangeBindings[iCol].bScale     = pColInfo[iCol].bScale;
            pChangeBindings[iCol].wType      = DBTYPE_WSTR;//pColInfo[iCol].wType;	//ǿ��תΪWSTR
            pChangeBindings[iCol].cbMaxLen   = pColInfo[iCol].ulColumnSize * sizeof(WCHAR);
            ulChangeOffset = pChangeBindings[iCol].cbMaxLen + pChangeBindings[iCol].obValue;
            ulChangeOffset = GRS_ROUNDUP(ulChangeOffset);
        }

    }

    for( iCol = 0; iCol < ulRowsetCols; iCol++ )
    {
        pShowBindings[iCol].iOrdinal   = pColInfo[iCol].iOrdinal;
        pShowBindings[iCol].dwPart     = DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;
        pShowBindings[iCol].obStatus   = ulShowOffset;
        pShowBindings[iCol].obLength   = ulShowOffset + sizeof(DBSTATUS);
        pShowBindings[iCol].obValue    = ulShowOffset + sizeof(DBSTATUS) + sizeof(ULONG);
        pShowBindings[iCol].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
        pShowBindings[iCol].eParamIO   = DBPARAMIO_NOTPARAM;
        pShowBindings[iCol].bPrecision = pColInfo[iCol].bPrecision;
        pShowBindings[iCol].bScale     = pColInfo[iCol].bScale;
        pShowBindings[iCol].wType      = DBTYPE_WSTR;//pColInfo[iCol].wType;	//ǿ��תΪWSTR
        pShowBindings[iCol].cbMaxLen   = pColInfo[iCol].ulColumnSize * sizeof(WCHAR);
        ulShowOffset = pShowBindings[iCol].cbMaxLen + pShowBindings[iCol].obValue;
        ulShowOffset = GRS_ROUNDUP(ulShowOffset);
    }

    //������ʾ�������
    plDisplayLen = (ULONG*)GRS_CALLOC(ulRowsetCols * sizeof(ULONG));

    //�����з�����
    hr = pIRowsetChange->QueryInterface(IID_IAccessor,(void**)&pIAccessor);
    GRS_COM_CHECK(hr,_T("���IAccessor�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);
    hr = pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,ulRowsetCols,pShowBindings,0,&hShowAccessor,NULL);
    GRS_COM_CHECK(hr,_T("��������ʾ������ʧ�ܣ������룺0x%08X\n"),hr);
    hr = pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,ulRealCols,pChangeBindings,0,&hChangeAccessor,NULL);
    GRS_COM_CHECK(hr,_T("�����и��·�����ʧ�ܣ������룺0x%08X\n"),hr);

    //��ȡ����
    hr = pIRowsetChange->QueryInterface(IID_IRowset,(void**)&pIRowset);
    GRS_COM_CHECK(hr,_T("���IRowset�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    hr = RowsetNotify.Advise(pIRowset,IID_IRowsetNotify);
    GRS_COM_CHECK(hr,_T("���IRowsetNotify�ص��ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

    //����lReadRows�����ݵĻ��壬Ȼ�󷴸���ȡlReadRows�е����Ȼ�����д���֮
    pChangeData = GRS_CALLOC(ulChangeOffset * lReadRows);
    pShowData	= GRS_CALLOC(ulShowOffset * lReadRows);
    ulAllRows = 0;

    while( S_OK == pIRowset->GetNextRows(DB_NULL_HCHAPTER,0,lReadRows,&pRowsObtained, &phRows) )    
    {//ѭ����ȡ���ݣ�ÿ��ѭ��Ĭ�϶�ȡlReadRows�У�ʵ�ʶ�ȡ��pRowsObtained��
        //ÿ������ʾһ������ʱ������0һ�¿������
        ZeroMemory(plDisplayLen,ulRowsetCols * sizeof(ULONG));
        for( iRow = 0; iRow < pRowsObtained; iRow++ )
        {
            //����ÿ��������ʾ���
            pCurChangeData    = (BYTE*)pChangeData + (ulChangeOffset * iRow);
            hr = pIRowset->GetData(phRows[iRow],hChangeAccessor,pCurChangeData);

            pCurShowData    = (BYTE*)pShowData + (ulShowOffset * iRow);
            hr = pIRowset->GetData(phRows[iRow],hShowAccessor,pCurShowData);

            UpdateDisplaySize(ulRowsetCols,pShowBindings,pCurShowData,plDisplayLen);
            //UpdateDisplaySize(ulRealCols,pChangeBindings,pCurChangeData,plDisplayLen);
        }

        if(!bShowColumnName)
        {//��ʾ�ֶ���Ϣ
            DisplayColumnNames(ulRowsetCols,pColInfo,plDisplayLen);
            //DisplayColumnNames(ulRealCols,&pColInfo[1],plDisplayLen);
            bShowColumnName = TRUE;
        }

        for( iRow = 0; iRow < pRowsObtained; iRow++ )
        {
            //����ƫ��ȡ��������
            pCurChangeData    = (BYTE*)pChangeData + (ulChangeOffset * iRow);
            pCurShowData    = (BYTE*)pShowData + (ulShowOffset * iRow);
            //��ʾ����
            DisplayRow(++ulAllRows,ulRowsetCols,pShowBindings,pCurShowData,plDisplayLen);
            //DisplayRow(++ulAllRows,ulRealCols,pChangeBindings,pCurChangeData,plDisplayLen);

            //ȡ������ָ�� ע�� ȡ���ǵڶ����ֶΣ�Ҳ���������ֶν����޸�
            //ע��ʵ�ʵ����ݳ���ҲҪ�޸�Ϊ��ȷ��ֵ����λ���ֽ�
            *(DBLENGTH *)((BYTE *)pCurChangeData + pChangeBindings[1].obLength) = 4;
            pwsValue    = (WCHAR*)((BYTE *)pCurChangeData + pChangeBindings[1].obValue);
            StringCchCopy(pwsValue,pChangeBindings[1].cbMaxLen/sizeof(WCHAR),_T("����"));

            hr = pIRowsetChange->SetData(phRows[iRow],hChangeAccessor,pCurChangeData);
            if(FAILED(hr))
            {
                GRS_PRINTF(_T("����SetData��������ʧ�ܣ������룺0x%08X\n"),hr);
            }
            //GRS_COM_CHECK(hr,_T("����SetData��������ʧ�ܣ������룺0x%08X\n"),hr);
        }

        //ɾ��һ������
        hr = pIRowsetChange->DeleteRows(DB_NULL_HCHAPTER,1,&phRows[0],parRowStatus);
        GRS_COM_CHECK(hr,_T("����DeleteRowsɾ����ʧ�ܣ������룺0x%08X\n"),hr);


        if( pRowsObtained )
        {//�ͷ��о������
            pIRowset->ReleaseRows(pRowsObtained,phRows,NULL,NULL,NULL);
        }
        bShowColumnName = FALSE;
        CoTaskMemFree(phRows);
        phRows = NULL;
    }

    //δ����DBPROP_CANHOLDROWS����ΪTRUEʱ,֮ǰ���޸�Ҫ�ύ��֮����ܲ�������
    //hr = pIRowsetUpdate->Update(NULL,0,NULL,NULL,NULL,&pUpdateStatus);
    //GRS_COM_CHECK(hr,_T("����Update�ύ����ʧ�ܣ������룺0x%08X\n"),hr);

    //����һ���������� �������ݺ� ����
    pbNewData = (BYTE*)GRS_CALLOC(ulChangeOffset);

    //���õ�һ���ֶ�
    *(DBLENGTH *)((BYTE *)pbNewData + pChangeBindings[0].obLength) = 12;
    pwsValue = (WCHAR*) (pbNewData + pChangeBindings[0].obValue);
    StringCchCopy(pwsValue,pChangeBindings[1].cbMaxLen/sizeof(WCHAR),_T("000000"));

    //���õڶ����ֶ�
    *(DBLENGTH *)((BYTE *)pbNewData + pChangeBindings[1].obLength) = 4;
    pwsValue = (WCHAR*) (pbNewData + pChangeBindings[1].obValue);
    StringCchCopy(pwsValue,pChangeBindings[1].cbMaxLen/sizeof(WCHAR),_T("�й�"));

    //����������
    hr = pIRowsetChange->InsertRow(NULL,hChangeAccessor,pbNewData,&hNewRow);
    GRS_COM_CHECK(hr,_T("����InsertRow��������ʧ�ܣ������룺0x%08X\n"),hr);

    //�����ύ���еĸ��µ����ݿ���
    hr = pIRowsetUpdate->Update(NULL,0,NULL,NULL,NULL,&pUpdateStatus);
    GRS_COM_CHECK(hr,_T("����Update�ύ����ʧ�ܣ������룺0x%08X\n"),hr);

CLEAR_UP:
    if(NULL != pIAccessor)
    {
        if(hChangeAccessor)
        {
            pIAccessor->ReleaseAccessor(hChangeAccessor,NULL);
        }
        if(hShowAccessor)
        {
            pIAccessor->ReleaseAccessor(hShowAccessor,NULL);
        }
    }
    GRS_SAFEFREE(pChangeData);
    GRS_SAFEFREE(pChangeBindings);
    GRS_SAFEFREE(pShowData);
    GRS_SAFEFREE(pShowBindings);
    GRS_SAFEFREE(plDisplayLen);
    GRS_SAFEFREE(pbNewData);
    CoTaskMemFree(pColInfo);
    CoTaskMemFree(pStringBuffer);
    CoTaskMemFree(pUpdateStatus);
    GRS_SAFERELEASE(pIRowset);
    GRS_SAFERELEASE(pIAccessor);
    GRS_SAFERELEASE(pIColumnsInfo);
    GRS_SAFERELEASE(pIRowsetChange);
    GRS_SAFERELEASE(pIRowsetUpdate);
    GRS_SAFERELEASE(pICommandProperties);
    GRS_SAFERELEASE(pICommand);
    GRS_SAFERELEASE(pICommandText);
    GRS_SAFERELEASE(pIOpenRowset);
    GRS_SAFERELEASE(pIDBCreateCommand);

    _tsystem(_T("PAUSE"));
    CoUninitialize();
    return 0;
}

VOID GetDBEventPhaseName(DBEVENTPHASE dbPhase,LPWSTR pszPhaseName)
{
    GRS_VAL2WSTR(dbPhase,DBEVENTPHASE_OKTODO,pszPhaseName);
    GRS_VAL2WSTR(dbPhase,DBEVENTPHASE_ABOUTTODO,pszPhaseName);
    GRS_VAL2WSTR(dbPhase,DBEVENTPHASE_SYNCHAFTER,pszPhaseName);
    GRS_VAL2WSTR(dbPhase,DBEVENTPHASE_FAILEDTODO,pszPhaseName);
    GRS_VAL2WSTR(dbPhase,DBEVENTPHASE_DIDEVENT,pszPhaseName);
}
VOID GetDBReasonName(DBREASON dbReason,LPWSTR pszReasonName)
{
    GRS_VAL2WSTR(dbReason,DBREASON_ROW_ASYNCHINSERT,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROWSET_FETCHPOSITIONCHANGE,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROWSET_RELEASE,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROWSET_CHANGED,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_COLUMN_SET,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_COLUMN_RECALCULATED,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROW_ACTIVATE,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROW_RELEASE,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROW_DELETE,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROW_FIRSTCHANGE,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROW_INSERT,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROW_RESYNCH,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROW_UNDOCHANGE,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROW_UNDOINSERT,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROW_UNDODELETE,pszReasonName);
    GRS_VAL2WSTR(dbReason,DBREASON_ROW_UPDATE,pszReasonName);
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


VOID DisplayColumnNames(DBORDINAL        ulRowsetCols
                        ,DBCOLUMNINFO*   pColInfo
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

    for( iCol = 0; iCol < ulRowsetCols; iCol++ ) 
    {
        pwszColName = pColInfo[iCol].pwszName;
        if( !pwszColName ) 
        {
            if( !pColInfo[iCol].iOrdinal )
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
        if( iCol < ulRowsetCols - 1 )
        {
            GRS_PRINTF(_T(" | "));
        }
    }
    GRS_PRINTF(_T("\n"));
}

HRESULT DisplayRow(DBCOUNTITEM iRow,
                   DBCOUNTITEM cBindings,
                   DBBINDING * pChangeBindings,
                   void * pChangeData,
                   ULONG * rgDispSize )
{
    GRS_USEPRINTF();
    HRESULT               hr = S_OK;
    WCHAR                 wszColumn[MAX_DISPLAY_SIZE + 1];
    DBSTATUS              dwStatus;
    DBLENGTH              ulLength;
    void *                pwsValue;
    DBCOUNTITEM           iCol;
    ULONG                 cbRead;
    ISequentialStream *   pISeqStream = NULL;
    ULONG                 cSpaces;
    ULONG                 iSpace;
    size_t				  szLen;

    GRS_PRINTF(_T("[%8d]| "), iRow);
    for( iCol = 0; iCol < cBindings; iCol++ ) 
    {
        dwStatus   = *(DBSTATUS *)((BYTE *)pChangeData + pChangeBindings[iCol].obStatus);
        ulLength   = *(DBLENGTH *)((BYTE *)pChangeData + pChangeBindings[iCol].obLength);
        pwsValue    = (BYTE *)pChangeData + pChangeBindings[iCol].obValue;
        switch( dwStatus ) 
        {
        case DBSTATUS_S_ISNULL:
            StringCchCopy(wszColumn,MAX_DISPLAY_SIZE + 1, L"(null)");
            break;
        case DBSTATUS_S_TRUNCATED:
        case DBSTATUS_S_OK:
        case DBSTATUS_S_DEFAULT:
            {
                switch( pChangeBindings[iCol].wType )
                {
                case DBTYPE_WSTR:
                    {   
                        StringCchCopy(wszColumn,MAX_DISPLAY_SIZE + 1,(WCHAR*)pwsValue);
                        break;
                    }
                case DBTYPE_IUNKNOWN:
                    {
                        pISeqStream = *(ISequentialStream**)pwsValue;
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
                        DBBINDING *   pChangeBindings,
                        void *        pChangeData,
                        ULONG *       rgDispSize )
{
    DBSTATUS      dwStatus;
    ULONG         cchLength;
    DBCOUNTITEM   iCol;

    for( iCol = 0; iCol < cBindings; iCol++ )
    {
        dwStatus = *(DBSTATUS *)((BYTE *)pChangeData + pChangeBindings[iCol].obStatus);
        cchLength = ((*(ULONG *)((BYTE *)pChangeData + pChangeBindings[iCol].obLength))
            / sizeof(WCHAR));
        switch( dwStatus )
        {
        case DBSTATUS_S_ISNULL:
            cchLength = 6;                              // "(null)"
            break;

        case DBSTATUS_S_TRUNCATED:
        case DBSTATUS_S_OK:
        case DBSTATUS_S_DEFAULT:
            if( pChangeBindings[iCol].wType == DBTYPE_IUNKNOWN )
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