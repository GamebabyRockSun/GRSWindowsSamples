#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <stdlib.h>

#include <msxml6.h>
#pragma comment (lib,"msxml6.lib")

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_SAFERELEASE(I) if(NULL != I){I->Release();I=NULL;}
#define GRS_COM_CHECK2(hr,...)  if(FAILED(hr)){GRS_PRINTF(__VA_ARGS__);goto CLEAR_UP;}

#define GRS_QUIT		0
#define GRS_NEW_ATTR	1
#define GRS_NEW_NODE	2
#define GRS_PARENT_NODE 3

#define GRS_MAX_INPUT	512

int GetCommand(IXMLDOMElement*pIElement);

int _tmain()
{
	GRS_USEPRINTF();
	CoInitialize(NULL);
	HRESULT						hr					= S_OK;
	IXMLDOMDocument3*			pIXMLDoc			= NULL;
	IXMLDOMElement*				pIXMLDocElement		= NULL;
	IXMLDOMElement*				pIXMLNode			= NULL;
	IXMLDOMElement*				pINewNode			= NULL;
	IXMLDOMNamedNodeMap*		pAttributeSet		= NULL;
	IXMLDOMAttribute*			pAttribute			= NULL;	//����

	VARIANT						vtbStr				= {};
	VARIANT						vtFile				= {};
	TCHAR						pInput[GRS_MAX_INPUT] = {};
	DWORD						dwLen				= 0;
	size_t						szLen				= 0;

	hr = CoCreateInstance(CLSID_DOMDocument60,NULL,
		CLSCTX_INPROC_SERVER,IID_IXMLDOMDocument3,(void**)&pIXMLDoc);
	GRS_COM_CHECK2(hr,_T("����IXMLDOMDocument3�ӿ�ʧ�ܣ������룺0x%08X\n"),hr);

	hr = pIXMLDoc->createElement(_T("Root"),&pIXMLDocElement);
	GRS_COM_CHECK2(hr,_T("�������ڵ�ʧ�ܣ������룺0x%08X\n"),hr);
	
	hr = pIXMLDoc->appendChild(pIXMLDocElement,NULL);
	GRS_COM_CHECK2(hr,_T("��Ӹ��ڵ㵽XML�ĵ���ʧ�ܣ������룺0x%08X\n"),hr);

	pIXMLNode = pIXMLDocElement;
	pIXMLDocElement = NULL;
	int iCommand = 0;
	do 
	{
		iCommand = GetCommand(pIXMLNode);
	
		if(GRS_NEW_ATTR == iCommand || GRS_NEW_NODE == iCommand )
		{
			GRS_PRINTF(_T("\t���������ƣ�"));
			while(S_OK != StringCchGets(pInput,GRS_MAX_INPUT));	
		}

		switch(iCommand)
		{
		case GRS_NEW_ATTR:
			hr = pIXMLDoc->createAttribute(pInput,&pAttribute);
			GRS_COM_CHECK2(hr,_T("����\"\"����ʧ�ܣ������룺0x%08X\n"),pInput,hr);

			GRS_PRINTF(_T("\t������ֵ��"));
			while(S_OK != StringCchGets(pInput,GRS_MAX_INPUT));
			
			StringCchLength(pInput,GRS_MAX_INPUT,&szLen);
			if(NULL != pInput[0] && szLen > 0)
			{
				hr = pAttribute->put_text(pInput);
				GRS_COM_CHECK2(hr,_T("��������ֵʧ�ܣ������룺0x%08X\n"),hr);
			}
			
			hr = pIXMLNode->get_attributes(&pAttributeSet);

			GRS_COM_CHECK2(hr,_T("��ȡ�ڵ������б�ʧ�ܣ������룺0x%08X\n"),hr);
			hr = pAttributeSet->setNamedItem(pAttribute,NULL);
			GRS_COM_CHECK2(hr,_T("�������ʧ�ܣ������룺0x%08X\n"),hr);
			GRS_SAFERELEASE(pAttributeSet);
			GRS_SAFERELEASE(pAttribute);
			break;
		case GRS_NEW_NODE:
			hr = pIXMLDoc->createElement(pInput,&pINewNode);
			GRS_COM_CHECK2(hr,_T("��ǩ\"\"����ʧ�ܣ������룺0x%08X\n"),pInput,hr);
			GRS_PRINTF(_T("\t������ֵ��"));
			while(S_OK != StringCchGets(pInput,GRS_MAX_INPUT));
			StringCchLength(pInput,GRS_MAX_INPUT,&szLen);
			if(NULL != pInput[0] && szLen > 0)
			{
				hr = pINewNode->put_text(pInput);
				GRS_COM_CHECK2(hr,_T("����Ԫ��ֵʧ�ܣ������룺0x%08X\n"),hr);
			}

			hr = pIXMLNode->appendChild(pINewNode,NULL);
			GRS_COM_CHECK2(hr,_T("��ӽڵ�ʧ�ܣ������룺0x%08X\n"),hr);

			GRS_SAFERELEASE(pIXMLNode);
			pIXMLNode = pINewNode;
			pINewNode = NULL;
			break;
		case GRS_PARENT_NODE:
			if(NULL != pIXMLNode)
			{
				IXMLDOMNode* pIParent = NULL;
				hr = pIXMLNode->get_parentNode(&pIParent);
				GRS_COM_CHECK2(hr,_T("��ȡ��ǰ�ڵ�ĸ��ڵ�ʧ�ܣ������룺0x%08X\n"),hr);
				hr = pIParent->QueryInterface(IID_IXMLDOMElement,(void**)&pINewNode);

				GRS_SAFERELEASE(pIXMLNode);
				pIXMLNode = pINewNode;
				pINewNode = NULL;
			}
			break;
		default:
			break;
		}
	} while (GRS_QUIT != iCommand);
	
	GRS_PRINTF(_T("\t������Ҫ�����XML�ļ����ƣ�"));
	while(S_OK != StringCchGets(pInput,GRS_MAX_INPUT));	
	StringCchLength(pInput,GRS_MAX_INPUT,&szLen);

	if(NULL == pInput[0] || szLen < 1)
	{//ʹ��Ĭ���ļ�����
		GRS_PRINTF(_T("ʹ��Ĭ���ļ����Ʊ���XML�ĵ�.\n"));
		StringCchCopy(pInput,GRS_MAX_INPUT,_T("WorkWithDOM.xml"));
	}

	vtFile.vt = VT_BSTR;
	vtFile.bstrVal = SysAllocString(pInput);
	hr = pIXMLDoc->save(vtFile);
	SysFreeString(vtFile.bstrVal);
	GRS_COM_CHECK2(hr,_T("����XML�ĵ�\"\"ʧ�ܣ������룺0x%08X\n"),pInput,hr);

	//���ļ���htmlʹ��Ĭ���������
	ShellExecute(NULL,_T("open"),pInput,NULL,NULL,SW_SHOWNORMAL);

CLEAR_UP:
	GRS_SAFERELEASE(pIXMLDoc);
	GRS_SAFERELEASE(pIXMLDocElement);
	GRS_SAFERELEASE(pIXMLNode);
	GRS_SAFERELEASE(pINewNode);
	GRS_SAFERELEASE(pAttributeSet);	//�����б�
	GRS_SAFERELEASE(pAttribute);	//����
	_tsystem(_T("PAUSE"));
	CoUninitialize();
	return 0;
}


int GetCommand(IXMLDOMElement*pIElement)
{
	int iCommand = 0;
	BSTR pstrName = NULL;

	GRS_USEPRINTF();
	if(NULL != pIElement && SUCCEEDED(pIElement->get_nodeName(&pstrName)))
	{
		GRS_PRINTF(_T("�������������ִ��(NODE��%s)��\n"),pstrName);
		SysFreeString(pstrName);
	}
	else
	{
		GRS_PRINTF(_T("�������������ִ�У�\n"));
	}

	GRS_PRINTF(_T("\t1-Ϊ��ǰ�Ľڵ����һ�����ԣ�\n"));
	GRS_PRINTF(_T("\t2-���һ���µĽڵ㣻\n"));
	GRS_PRINTF(_T("\t3-���ص��ϼ��ڵ㣻\n"));
	GRS_PRINTF(_T("\t0-�������룻\n"));
	GRS_PRINTF(_T("\t�����룺\t"));

	TCHAR pInput[GRS_MAX_INPUT] = {};
	while(S_OK != StringCchGets(pInput,GRS_MAX_INPUT));	
	iCommand = _ttoi(pInput);

	return iCommand;
}
