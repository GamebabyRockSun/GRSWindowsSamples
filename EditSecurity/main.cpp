#include <tchar.h>
#include <windows.h>
#include <Aclapi.h>
#include <Aclui.h>
#pragma comment( lib, "Aclui.lib" )

class CSecurityInformation:public ISecurityInformation 
{
protected:
	LONG m_lRefCnt;
	HANDLE m_hObject;
public:
	CSecurityInformation(HANDLE hObject)
		:m_lRefCnt(1)
		,m_hObject(hObject)
	{
	}
	virtual ~CSecurityInformation()
	{

	}
public:
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj)
	{
		if ( IID_ISecurityInformation == riid  ||  IID_IUnknown == riid )
		{
			*ppvObj = this;
		}
		else
		{
			*ppvObj = NULL;
		}
		return (NULL == *ppvObj?E_NOINTERFACE:S_OK);
	}
	STDMETHODIMP_(ULONG) AddRef() 
	{
		return ++m_lRefCnt;
	}
	STDMETHODIMP_(ULONG) Release()
	{
		return --m_lRefCnt;
	}

	STDMETHODIMP GetObjectInformation(PSI_OBJECT_INFO pOi )
	{
		return S_OK;
	}

	STDMETHODIMP GetSecurity(SECURITY_INFORMATION Rsi
		,PSECURITY_DESCRIPTOR *ppSD
		,BOOL fDefault ) 
	{
		if(NULL != m_hObject )
		{
			GetSecurityInfo(m_hObject,SE_KERNEL_OBJECT,Rsi,
				NULL,NULL,NULL,NULL,ppSD);
		}
		return S_OK;
	}
	STDMETHODIMP SetSecurity(SECURITY_INFORMATION si
		,PSECURITY_DESCRIPTOR pSD )
	{
		if( NULL != pSD )
		{
			SECURITY_DESCRIPTOR* pTmpSD = (SECURITY_DESCRIPTOR*)pSD;
			SetSecurityInfo(m_hObject,SE_KERNEL_OBJECT,si
				,pTmpSD->Owner,pTmpSD->Group,pTmpSD->Dacl,pTmpSD->Sacl);
		}

		return S_OK;
	}
	STDMETHODIMP GetAccessRights(const GUID* pguidObjectType,
		DWORD dwFlags, // SI_EDIT_AUDITS, SI_EDIT_PROPERTIES
		PSI_ACCESS *ppAccess,
		ULONG *pcAccesses,
		ULONG *piDefaultAccess )
	{
		return S_OK;
	}
	STDMETHODIMP MapGeneric(const GUID *pguidObjectType,
		UCHAR *pAceFlags,
		ACCESS_MASK *pMask)
	{
		return S_OK;
	}
	STDMETHODIMP GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,ULONG *pcInheritTypes )
	{

		return S_OK;
	}
	STDMETHODIMP PropertySheetPageCallback(HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage )
	{
		return S_OK;
	}
};


int _tmain()
{
	HWND hWndParent = GetDesktopWindow();
	HANDLE hObject = GetCurrentProcess();//CreateEvent(NULL,FALSE,FALSE,NULL);
	CSecurityInformation si(hObject);

	EditSecurity( hWndParent,&si );

	EditSecurity(hWndParent,&si);

	CloseHandle(hObject);
	return 0;
}