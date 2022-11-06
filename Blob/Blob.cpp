#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define COM_NO_WINDOWS_H    //����Ѿ�������Windows.h��ʹ������Windows�⺯��ʱ
#define DBINITCONSTANTS
#define INITGUID
#define OLEDBVER 0x0260		//OLEDB2.6����
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
BOOL GetFileData(LPCTSTR pszFileName,BYTE*&pFileData,DWORD& dwDataLen);

//�Զ���һ��
class CSeqStream : public ISequentialStream 
{
public:
	// Constructors
	CSeqStream();
	virtual ~CSeqStream();
public:
	virtual BOOL Seek(ULONG iPos);
	virtual BOOL Clear();
	virtual BOOL CompareData(void* pBuffer);
	virtual ULONG Length() 
	{ 
		return m_cBufSize; 
	};
	virtual operator void* const() 
	{ 
		return m_pBuffer; 
	};
public:
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);

	STDMETHODIMP Read( 
		/* [out] */ void __RPC_FAR *pv,
		/* [in]  */ ULONG cb,
		/* [out] */ ULONG __RPC_FAR *pcbRead);

	STDMETHODIMP Write( 
		/* [in] */ const void __RPC_FAR *pv,
		/* [in] */ ULONG cb,
		/* [out]*/ ULONG __RPC_FAR *pcbWritten);
private:
	ULONG m_cRef;       // reference count
	void* m_pBuffer;    // buffer
	ULONG m_cBufSize;   // buffer size
	ULONG m_iPos;       // current index position in the buffer
};

// class implementation
CSeqStream::CSeqStream() 
{
	m_iPos = 0;
	m_cRef = 0;
	m_pBuffer = NULL;
	m_cBufSize = 0;

	// The constructor AddRef's
	AddRef();
}

CSeqStream::~CSeqStream() 
{
	// Shouldn't have any references left ASSERT(m_cRef == 0);
	CoTaskMemFree(m_pBuffer);
}

ULONG CSeqStream::AddRef() 
{
	return ++m_cRef;
}

ULONG CSeqStream::Release()
{
	// ASSERT(m_cRef);
	if (--m_cRef)
		return m_cRef;

	delete this;
	return 0;
}

HRESULT CSeqStream::QueryInterface(REFIID riid, void** ppv)
{
	// ASSERT(ppv);
	*ppv = NULL;

	if (riid == IID_IUnknown)
		*ppv = this;

	if (riid == IID_ISequentialStream)
		*ppv = this;

	if(*ppv) {
		((IUnknown*)*ppv)->AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

BOOL CSeqStream::Seek(ULONG iPos) 
{
	// Make sure the desired position is within the buffer
	if( !(iPos == 0 || iPos < m_cBufSize) )
    {
        return FALSE;
    }
	// Reset the current buffer position
	m_iPos = iPos;
	return TRUE;
}

BOOL CSeqStream::Clear()
{
	// Frees the buffer
	m_iPos = 0;
	m_cBufSize = 0;

	CoTaskMemFree(m_pBuffer);
	m_pBuffer = NULL;

	return TRUE;
}

BOOL CSeqStream::CompareData(void* pBuffer) 
{
	// ASSERT(pBuffer);

	// Quick and easy way to compare user buffer with the stream
	return memcmp(pBuffer, m_pBuffer, m_cBufSize) == 0;
}

HRESULT CSeqStream::Read(void *pv, ULONG cb, ULONG* pcbRead) 
{
	// Parameter checking
	if (pcbRead)
		*pcbRead = 0;

	if (!pv)
		return STG_E_INVALIDPOINTER;

	if (cb == 0)
		return S_OK;

	// Actual code
	ULONG cBytesLeft = m_cBufSize - m_iPos;
	ULONG cBytesRead = cb > cBytesLeft ? cBytesLeft : cb;

	// if no more bytes to retrieve return 
	if (cBytesLeft == 0)
		return S_FALSE; 

	// Copy to users buffer the number of bytes requested or remaining
	memcpy(pv, (void*)((BYTE*)m_pBuffer + m_iPos), cBytesRead);
	m_iPos += cBytesRead;

	if (pcbRead)
		*pcbRead = cBytesRead;

	if (cb != cBytesRead)
		return S_FALSE; 

	return S_OK;
}

HRESULT CSeqStream::Write(const void *pv, ULONG cb, ULONG* pcbWritten)
{
	// Parameter checking
	if (!pv)
		return STG_E_INVALIDPOINTER;

	if (pcbWritten)
		*pcbWritten = 0;

	if (cb == 0)
		return S_OK;

	// Enlarge the current buffer
	m_cBufSize += cb;

	// Need to append to the end of the stream
	m_pBuffer = CoTaskMemRealloc(m_pBuffer, m_cBufSize);
	memcpy((void*)((BYTE*)m_pBuffer + m_iPos), pv, cb);
	m_iPos += cb;

	if (pcbWritten)
		*pcbWritten = cb;

	return S_OK;
}

int _tmain()
{
	GRS_USEPRINTF();
	CoInitialize(NULL);

	HRESULT					hr							= S_OK;
	IGetDataSource*			pIGetDataSource				= NULL;
	IDBProperties*			pIDBProperties				= NULL;
	IOpenRowset*			pIOpenRowset				= NULL;
	IRowset*				pIRowset					= NULL;
	IRowsetChange*			pIRowsetChange				= NULL;
	IRowsetUpdate*			pIRowsetUpdate				= NULL;

	IColumnsInfo*			pIColumnsInfo				= NULL;
	IAccessor*				pIAccessor					= NULL;
	IRowsetInfo*			pIRowsetInfo				= NULL;

	LPWSTR					pStringBuffer				= NULL;
	ULONG					lColumns					= 0;
	DBCOLUMNINFO*			pColumnInfo					= NULL;
	
	BOOL					bMultiObjs					= FALSE;//�Ƿ�֧�ֶ��BLOB���ֶ�
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
	//���Լ��� �� ����
	DBPROPSET				PropSet[1]					= {};
	DBPROP					Prop[6]						= {};
	//����ID���� �� ����ID
	DBPROPID                PropertyIDs[4]				= {};
	DBPROPIDSET				PropertyIDSets[1]			= {};
	ULONG					ulPropSets					= 0;
	DBPROPSET*				pGetPropSets				= NULL;
	IID						iid_blob					= {};
	BYTE*					pNewData1					= NULL;
	BYTE*					pNewData2					= NULL;
	BYTE*					pNewData3					= NULL;
	LPWSTR					pwsValue					= NULL;
	BYTE*					pFileData					= NULL;
	DWORD					dwFileLen					= 0;
	ULONG					ulWrited					= 0;
	BYTE*					pTxtData					= NULL;
	HROW					hNewRow						= NULL;
	DBROWSTATUS*			pUpdateStatus				= NULL;
	CSeqStream*				pSeqStream					= NULL;

	if(!CreateDBSession(pIOpenRowset))
	{
		goto CLEAR_UP;
	}

	//�ж�����Դ֧�ֵ�BLOBs�����ݵĽӿ�����
	PropertyIDs[0] = DBPROP_STRUCTUREDSTORAGE;
	PropertyIDs[1] = DBPROP_OLEOBJECTS;

	PropertyIDSets[0].rgPropertyIDs     = PropertyIDs;
	PropertyIDSets[0].cPropertyIDs      = 2;
	PropertyIDSets[0].guidPropertySet   = DBPROPSET_DATASOURCEINFO;

	hr = pIOpenRowset->QueryInterface(IID_IGetDataSource,(void**)&pIGetDataSource);
	GRS_COM_CHECK(hr,_T("��ȡIGetDataSource�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

	hr = pIGetDataSource->GetDataSource(IID_IDBProperties,(IUnknown**)&pIDBProperties);
	GRS_COM_CHECK(hr,_T("��ȡIDBProperties�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

	hr = pIDBProperties->GetProperties(1,PropertyIDSets,&ulPropSets,&pGetPropSets);
	GRS_COM_CHECK(hr,_T("��ȡ����Դ����ʧ�ܣ������룺0x%08X\n"),hr);	
	if( VT_I4 == pGetPropSets[0].rgProperties[0].vValue.vt )	
	{
		if( DBPROPVAL_SS_ISTREAM & pGetPropSets[0].rgProperties[0].vValue.intVal )
		{
			CopyMemory(&iid_blob,&IID_IStream,sizeof(IID));
		}
		else if( DBPROPVAL_SS_ISEQUENTIALSTREAM & pGetPropSets[0].rgProperties[0].vValue.intVal  )
		{
			CopyMemory(&iid_blob,&IID_ISequentialStream,sizeof(IID));
		}
		else if( DBPROPVAL_SS_ISTORAGE & pGetPropSets[0].rgProperties[0].vValue.intVal  )
		{
			CopyMemory(&iid_blob,&IID_IStorage,sizeof(IID));
		}
		else if( DBPROPVAL_SS_ILOCKBYTES & pGetPropSets[0].rgProperties[0].vValue.intVal  )
		{
			CopyMemory(&iid_blob,&IID_ILockBytes,sizeof(IID));
		}
		else
		{
			CopyMemory(&iid_blob,&IID_IUnknown,sizeof(IID));
		}
	}

	if(DBPROPVAL_OO_BLOB & pGetPropSets[0].rgProperties[1].vValue.intVal)
	{
	
		GRS_PRINTF(_T("\tThe provider supports access to BLOBs as structured \
storage objects.A consumer determines what interfaces are \
supported through DBPROP_STRUCTUREDSTORAGE.\n"));
	}

	if(DBPROPVAL_OO_DIRECTBIND & pGetPropSets[0].rgProperties[1].vValue.intVal)
	{
		GRS_PRINTF(_T("\tThe provider supports direct binding. If this bit is set,\
the IBindResource and ICreateRow interfaces are supported on the \
session object and the provider implements a provider binder object.\n"));
	}

	if(DBPROPVAL_OO_IPERSIST & pGetPropSets[0].rgProperties[1].vValue.intVal)
	{
		GRS_PRINTF(_T("\tThe provider supports access to COM objects \
through IPersistStream, IPersistStreamInit, or IPersistStorage.\n"));
	}
	if(DBPROPVAL_OO_ROWOBJECT & pGetPropSets[0].rgProperties[1].vValue.intVal)
	{
		GRS_PRINTF(_T("\tThe provider supports row objects. IGetRow is supported on rowsets.\
Row objects support the mandatory interfaces IRow, IGetSession,\
IColumnsInfo, and IConvertType. If the provider supports direct URL binding,\
it must support binding to row objects by passing DBGUID_ROW in \
IBindResource::Bind and, if supported, ICreateRow::CreateRow.\n"));
	}
	if(DBPROPVAL_OO_SCOPED & pGetPropSets[0].rgProperties[1].vValue.intVal)
	{
		GRS_PRINTF(_T("\tIndicates that row objects implement IScopedOperations.\n")); 
	}
	
	if( DBPROPVAL_OO_SINGLETON & pGetPropSets[0].rgProperties[1].vValue.intVal )
	{
		GRS_PRINTF(_T("\tThe provider supports singleton selects. The provider \
supports the return of row objects on ICommand::Execute and \
IOpenRowset::OpenRowset.\n"));
	}
	
	//��������
	//�����������Ե������
	Prop[0].dwPropertyID    = DBPROP_CANFETCHBACKWARDS;
	Prop[0].vValue.vt       = VT_BOOL;
	Prop[0].vValue.boolVal  = VARIANT_TRUE;
	Prop[0].dwOptions       = DBPROPOPTIONS_REQUIRED;
	Prop[0].colid           = DB_NULLID;

	//�����IRowsetLocate�ӿ�
	Prop[1].dwPropertyID    = DBPROP_IRowsetLocate;
	Prop[1].vValue.vt       = VT_BOOL;
	Prop[1].vValue.boolVal  = VARIANT_TRUE;         
	Prop[1].dwOptions       = DBPROPOPTIONS_REQUIRED;
	Prop[1].colid           = DB_NULLID;

	Prop[2].dwPropertyID    = DBPROP_UPDATABILITY;
	Prop[2].vValue.vt       = VT_I4;
	//����ִ�к�Ľ�������Խ�����ɾ�Ĳ���
	Prop[2].vValue.lVal     = DBPROPVAL_UP_CHANGE     //��Update����
		|DBPROPVAL_UP_DELETE					//��Delete����
		|DBPROPVAL_UP_INSERT;					//��Insert����
	Prop[2].dwOptions       = DBPROPOPTIONS_REQUIRED;
	Prop[2].colid           = DB_NULLID;

	//��IRowsetUpdate�ӿ�Ҳ����֧���ӳٵĸ�������
	Prop[3].dwPropertyID    = DBPROP_IRowsetUpdate;
	Prop[3].vValue.vt       = VT_BOOL;
	Prop[3].vValue.boolVal  = VARIANT_TRUE;         
	Prop[3].dwOptions       = DBPROPOPTIONS_REQUIRED;
	Prop[3].colid           = DB_NULLID;

	//�����������Ҳ��Ҫ���䣬��������ô�Ͳ��ܼ�����ɾ����ͬʱ��������
	Prop[4].dwPropertyID    = DBPROP_CANHOLDROWS;
	Prop[4].vValue.vt       = VT_BOOL;
	Prop[4].vValue.boolVal  = VARIANT_TRUE;         
	Prop[4].dwOptions       = DBPROPOPTIONS_REQUIRED;
	Prop[4].colid           = DB_NULLID;

	PropSet[0].guidPropertySet  = DBPROPSET_ROWSET;      //ע�����Լ��ϵ�����
	PropSet[0].cProperties      = 5;							//���Լ�����ʵ���Ը���
	PropSet[0].rgProperties     = Prop;

	TableID.eKind           = DBKIND_NAME;
	TableID.uName.pwszName  = (LPOLESTR)pszTableName;

	hr = pIOpenRowset->OpenRowset(NULL,&TableID,NULL,IID_IRowsetChange,1,PropSet,(IUnknown**)&pIRowsetChange);
	GRS_COM_CHECK(hr,_T("�򿪱����'%s'ʧ�ܣ������룺0x%08X\n"),pszTableName,hr);
	
	hr = pIRowsetChange->QueryInterface(IID_IRowset,(void**)&pIRowset);
	GRS_COM_CHECK(hr,_T("��ȡIRowset�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

	//���IRowsetUpdate�ӿ�
	hr = pIRowsetChange->QueryInterface(IID_IRowsetUpdate,(void**)&pIRowsetUpdate);
	GRS_COM_CHECK(hr,_T("��ȡIRowsetUpdate�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

	hr = pIRowset->QueryInterface(IID_IRowsetInfo,(void**)&pIRowsetInfo);
	GRS_COM_CHECK(hr,_T("��ȡIColumnsInfo�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

	//�Ƿ�֧�ֶ��BLOB���ֶ���һ�������а󶨵�����
	PropertyIDs[0] = DBPROP_MULTIPLESTORAGEOBJECTS;
	
	PropertyIDSets[0].rgPropertyIDs     = PropertyIDs;
	PropertyIDSets[0].cPropertyIDs      = 1;
	PropertyIDSets[0].guidPropertySet   = DBPROPSET_ROWSET;

	hr = pIRowsetInfo->GetProperties(1,PropertyIDSets,&ulPropSets,&pGetPropSets);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("��ȡRowset����DBPROP_MULTIPLESTORAGEOBJECTS��Ϣʧ�ܣ������룺0x%08X\n"),hr);
	}

	if( V_VT(&pGetPropSets[0].rgProperties[0].vValue) == VT_BOOL )
	{
		bMultiObjs = V_BOOL(&pGetPropSets[0].rgProperties[0].vValue);
	}

	if( DBPROPSTATUS_NOTSUPPORTED == pGetPropSets[0].rgProperties[0].dwStatus )
	{
		bMultiObjs = FALSE;
	}

	hr = pIRowset->QueryInterface(IID_IColumnsInfo,(void**)&pIColumnsInfo);
	GRS_COM_CHECK(hr,_T("��ȡIColumnsInfo�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

	hr = pIColumnsInfo->GetColumnInfo(&lColumns,&pColumnInfo,&pStringBuffer);
	GRS_COM_CHECK(hr,_T("��ȡ����Ϣʧ�ܣ������룺0x%08X\n"),hr);

	
	pBindings = (DBBINDING*)GRS_CALLOC(lColumns * sizeof(DBBINDING));
	pulColCnts = (ULONG*)GRS_CALLOC(sizeof(ULONG));
	pulDataLen = (ULONG*)GRS_CALLOC(sizeof(ULONG));
	ppBindings = (DBBINDING**)GRS_CALLOC(sizeof(DBBINDING*));	//Ĭ����һ����
	
	for(iCol = 0; iCol < lColumns;iCol ++)
	{
		if(NULL == ppBindings[ulBindCnt])
		{
			ppBindings[ulBindCnt] = &pBindings[iCol];
		}
		//�ۼ�����
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

		if( ( pColumnInfo[iCol].wType == DBTYPE_IUNKNOWN ||
			( pColumnInfo[iCol].dwFlags & DBCOLUMNFLAGS_ISLONG) )  )
		{
			pBindings[iCol].pBindExt			= NULL;
			pBindings[iCol].dwFlags				= 0;
			pBindings[iCol].bPrecision			= 0;
			pBindings[iCol].bScale				= 0;
			pBindings[iCol].wType				= DBTYPE_IUNKNOWN;
			pBindings[iCol].cbMaxLen			= 0;			
			pBindings[iCol].pObject				= (DBOBJECT*)GRS_CALLOC(sizeof(DBOBJECT));
			pBindings[iCol].pObject->iid		= iid_blob;
			pBindings[iCol].pObject->dwFlags	= STGM_READ;
			++ ulBlobs;
		}	

		pBindings[iCol].cbMaxLen = min(pBindings[iCol].cbMaxLen, MAX_COL_SIZE);
		pulDataLen[ulBindCnt] = pBindings[iCol].cbMaxLen + pBindings[iCol].obValue;
		if(DBTYPE_IUNKNOWN == pBindings[iCol].wType)
		{//������0������Ҫ�ѽӿ�ָ���λ��������
			pulDataLen[ulBindCnt]				+= sizeof(ISequentialStream*);
		}
		pulDataLen[ulBindCnt] = GRS_ROUNDUP(pulDataLen[ulBindCnt]);

		if( ( (!bMultiObjs && ulBlobs) || 0 == pColumnInfo[iCol].iOrdinal )
			&& (iCol != (lColumns - 1)) )
		{//���һ������Ȩ�����ж��,��0������ �������ڸ���
			//����һ����
			++ ulBindCnt;
			pulColCnts = (ULONG*)GRS_CREALLOC(pulColCnts,(ulBindCnt + 1) * sizeof(ULONG));
			pulDataLen = (ULONG*)GRS_CREALLOC(pulDataLen,(ulBindCnt + 1) * sizeof(ULONG));
			ppBindings = (DBBINDING**)GRS_CREALLOC(ppBindings,(ulBindCnt + 1) * sizeof(DBBINDING*));
            //ulBlobs = 0;
		}
	}

	hr = pIRowset->QueryInterface(IID_IAccessor,(void**)&pIAccessor);
	GRS_COM_CHECK(hr,_T("��ȡIAccessor�ӿ�ʧ�ܣ�������:0x%08X\n"),hr);

	phAccessor = (HACCESSOR*)GRS_CALLOC( (ulBindCnt + 1) * sizeof(HACCESSOR));

	for(iCol = 0;iCol <= ulBindCnt;iCol++)
	{
		pBindStatus = (DBBINDSTATUS*)GRS_CALLOC(pulColCnts[iCol] * sizeof(DBBINDSTATUS));
		hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA,pulColCnts[iCol]
					,ppBindings[iCol],pulDataLen[iCol],&phAccessor[iCol],pBindStatus);
		GRS_COM_CHECK(hr,_T("������[%ul]��������ʧ�ܣ�������:0x%08x\n"),iCol,hr)
		GRS_SAFEFREE(pBindStatus);
	}

	if(0 == pColumnInfo[0].iOrdinal)
	{
		iCol = 1;
	}
	else
	{
		iCol = 0;
	}
	//�Ȳ����һ���������е��� ע�� ������0����������� ���в��ܲ���
	pNewData1 = (BYTE*)GRS_CALLOC(pulDataLen[iCol]);
	for(ULONG i = 0; i < pulColCnts[iCol]; i++)
	{
		if(DBTYPE_IUNKNOWN == ppBindings[iCol][i].wType)
		{//BLOB�ֶ�
			*((DBSTATUS*)(pNewData1 + ppBindings[iCol][i].obStatus)) = DBSTATUS_S_OK;
			pSeqStream = new CSeqStream();
			*(ISequentialStream**)(pNewData1 + ppBindings[iCol][i].obValue) = pSeqStream;
			GetFileData(_T("1.jpg"),pFileData,dwFileLen);
			*(ULONG*)(pNewData1 + ppBindings[iCol][i].obLength) = dwFileLen;
			hr = pSeqStream->Write(pFileData,dwFileLen,&ulWrited);
			GRS_SAFEFREE(pFileData);
            pSeqStream->Seek(0);
			//ע�ⲻ��delete pSeqStream ����Դ�ڲ������Release ������delete
		}
		else
		{//����������ID 
			*(DBLENGTH *)(pNewData1 + ppBindings[iCol][i].obLength) = 2;
			if(DBTYPE_I4 == ppBindings[iCol][i].wType)
			{
				*(INT*)(pNewData1 + ppBindings[iCol][i].obValue) = 2;
			}
		}
	}
	//�������л���о��
	hr = pIRowsetChange->InsertRow(NULL,phAccessor[iCol],pNewData1,&hNewRow);
	GRS_COM_CHECK(hr,_T("����InsertRow��������ʧ�ܣ������룺0x%08X\n"),hr);
	

	++ iCol; //��һ��������
	pNewData2 = (BYTE*)GRS_CALLOC(pulDataLen[iCol]);
	if(DBTYPE_IUNKNOWN == ppBindings[iCol][0].wType)
	{//BLOB�ֶ�

		*((DBSTATUS*)(pNewData2 + ppBindings[iCol][0].obStatus)) = DBSTATUS_S_OK;
		*(ULONG*)(pNewData2 + ppBindings[iCol][0].obLength) = 0;
		CSeqStream* pSeqStream = new CSeqStream();
		*(ISequentialStream**)(pNewData2 + ppBindings[iCol][0].obValue) = pSeqStream;
		GetFileData(_T("2.jpg"),pFileData,dwFileLen);
		*(ULONG*)(pNewData2 + ppBindings[iCol][0].obLength) = dwFileLen;
		hr = pSeqStream->Write(pFileData,dwFileLen,&ulWrited);
		GRS_SAFEFREE(pFileData);
        pSeqStream->Seek(0);
	}
	hr = pIRowsetChange->SetData(hNewRow,phAccessor[iCol],pNewData2);
	GRS_COM_CHECK(hr,_T("����SetData���µڶ���ʧ�ܣ������룺0x%08X\n"),hr);

	++ iCol; //��һ��������
	pNewData3 = (BYTE*)GRS_CALLOC(pulDataLen[iCol]);
	if(DBTYPE_IUNKNOWN == ppBindings[iCol][0].wType)
	{//BLOB�ֶ�
		*((DBSTATUS*)(pNewData3 + ppBindings[iCol][0].obStatus)) = DBSTATUS_S_OK;
		*(ULONG*)(pNewData3 + ppBindings[iCol][0].obLength) = 0;
		CSeqStream* pSeqStream = new CSeqStream();
		*(ISequentialStream**)(pNewData3 + ppBindings[iCol][0].obValue) = pSeqStream;
		GetFileData(_T("3.txt"),pFileData,dwFileLen);
		*(ULONG*)(pNewData3 + ppBindings[iCol][0].obLength) = dwFileLen;
		hr = pSeqStream->Write(pFileData,dwFileLen,&ulWrited);
		GRS_SAFEFREE(pFileData);
        pSeqStream->Seek(0);
	}
	hr = pIRowsetChange->SetData(hNewRow,phAccessor[iCol],pNewData3);
	GRS_COM_CHECK(hr,_T("����SetData���µڶ���ʧ�ܣ������룺0x%08X\n"),hr);

	hr = pIRowsetUpdate->Update(NULL,0,NULL,NULL,NULL,&pUpdateStatus);
	GRS_COM_CHECK(hr,_T("����Update�ύ����ʧ�ܣ������룺0x%08X\n"),hr);
	
	hr = pIRowset->ReleaseRows(1,&hNewRow,NULL,NULL,NULL);
	GRS_COM_CHECK(hr,_T("����ReleaseRows�ͷ����о��ʧ�ܣ������룺0x%08X\n"),hr);

	GRS_PRINTF(_T("���ݲ���ɹ���������3��blob�����ݣ���鿴���ݿ�������\n"));
	
CLEAR_UP:
	if(phAccessor && pIAccessor)
	{
		for(iCol = 0; iCol <= ulBindCnt;iCol++)
		{
			pIAccessor->ReleaseAccessor(phAccessor[iCol],NULL);
		}
	}
	GRS_SAFEFREE(pNewData1);
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
	GRS_SAFERELEASE(pIRowsetChange);
	GRS_SAFERELEASE(pIRowsetUpdate);
	GRS_SAFERELEASE(pIRowset);
	GRS_SAFERELEASE(pIOpenRowset);
	GRS_SAFERELEASE(pIGetDataSource);
	GRS_SAFERELEASE(pIDBProperties);

	_tsystem(_T("PAUSE"));
	CoUninitialize();
	return 0;
}


BOOL GetFileData(LPCTSTR pszFileName,BYTE*&pFileData,DWORD& dwDataLen)
{//��ȡ�ļ�����
	GRS_USEPRINTF();
	pFileData = NULL;
	dwDataLen = 0;
	HANDLE hFile = CreateFile(pszFileName,GENERIC_READ
		,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(INVALID_HANDLE_VALUE == hFile)
	{
		GRS_PRINTF(_T("�޷����ļ�\"%s\""),pszFileName);
		return FALSE;
	}

	dwDataLen = GetFileSize(hFile,NULL);
	pFileData = (BYTE*)GRS_CALLOC(dwDataLen);
	BOOL bRet = ReadFile(hFile,pFileData,dwDataLen,&dwDataLen,NULL);
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

	//2��ʹ�ô����뷽ʽ���ӵ�ָ��������Դ
	HRESULT hr = CoCreateInstance(CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER,
		IID_IDataInitialize,(void**)&pIDataInitialize);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("�޷�����IDataInitialize�ӿڣ������룺0x%08x\n"),hr);
		return FALSE;
	}

	hr = CLSIDFromProgID(_T("SQLOLEDB"), &clsid);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("��ȡ\"SQLOLEDB\"��CLSID���󣬴����룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		return FALSE;
	}

	//��������Դ���Ӷ���ͳ�ʼ���ӿ�
	hr = pIDataInitialize->CreateDBInstance(clsid, NULL,
		CLSCTX_INPROC_SERVER, NULL, IID_IDBInitialize,
		(IUnknown**)&pDBConnect);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("����IDataInitialize->CreateDBInstance����IDBInitialize�ӿڴ��󣬴����룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}

	//׼����������
	//ע����Ȼ����ֻʹ����3�����ԣ�������ȻҪ����ͳ�ʼ��3+1������
	//Ŀ���������������������һ���յ�������Ϊ�����β
	DBPROP InitProperties[5] = {};
	DBPROPSET rgInitPropSet[1] = {};
	//ָ�����ݿ�ʵ����������ʹ���˱���local��ָ������Ĭ��ʵ��
	InitProperties[0].dwPropertyID = DBPROP_INIT_DATASOURCE;
	InitProperties[0].vValue.vt = VT_BSTR;
	InitProperties[0].vValue.bstrVal= SysAllocString(L"ASUS-PC\\SQL2008");
	InitProperties[0].dwOptions = DBPROPOPTIONS_REQUIRED;
	InitProperties[0].colid = DB_NULLID;
	//ָ�����ݿ���
	InitProperties[1].dwPropertyID = DBPROP_INIT_CATALOG;
	InitProperties[1].vValue.vt = VT_BSTR;
	InitProperties[1].vValue.bstrVal = SysAllocString(L"Study");
	InitProperties[1].dwOptions = DBPROPOPTIONS_REQUIRED;
	InitProperties[1].colid = DB_NULLID;

	////ָ�������֤��ʽΪ���ɰ�ȫģʽ��SSPI��
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

	//����һ��GUIDΪDBPROPSET_DBINIT�����Լ��ϣ���Ҳ�ǳ�ʼ������ʱ��Ҫ��Ψһһ//�����Լ���
	rgInitPropSet[0].guidPropertySet = DBPROPSET_DBINIT;
	rgInitPropSet[0].cProperties = 4;
	rgInitPropSet[0].rgProperties = InitProperties;

	//�õ����ݿ��ʼ�������Խӿ�
	hr = pDBConnect->QueryInterface(IID_IDBProperties, (void **)&pIDBProperties);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("��ȡIDBProperties�ӿ�ʧ�ܣ������룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}

	hr = pIDBProperties->SetProperties(1, rgInitPropSet); 
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("������������ʧ�ܣ������룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		GRS_SAFERELEASE(pDBConnect);
		GRS_SAFERELEASE(pIDBProperties);
		return FALSE;
	}
	//����һ��������ɣ���Ӧ�ĽӿھͿ����ͷ���
	GRS_SAFERELEASE(pIDBProperties);


	//����ָ�����������ӵ����ݿ�
	hr = pDBConnect->Initialize();
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("ʹ���������ӵ�����Դʧ�ܣ������룺0x%08x\n"),hr);
		GRS_SAFERELEASE(pIDataInitialize);
		GRS_SAFERELEASE(pDBConnect);
		return FALSE;
	}

	GRS_PRINTF(_T("ʹ�����Լ���ʽ���ӵ�����Դ�ɹ�!\n"));

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
	GRS_SAFERELEASE(pIDataInitialize);

	return TRUE;

	//1��ʹ��OLEDB���ݿ����ӶԻ������ӵ�����Դ
	hr = CoCreateInstance(CLSID_DataLinks, NULL, CLSCTX_INPROC_SERVER,
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
