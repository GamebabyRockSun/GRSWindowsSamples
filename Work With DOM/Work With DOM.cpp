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
	IXMLDOMAttribute*			pAttribute			= NULL;	//属性

	VARIANT						vtbStr				= {};
	VARIANT						vtFile				= {};
	TCHAR						pInput[GRS_MAX_INPUT] = {};
	DWORD						dwLen				= 0;
	size_t						szLen				= 0;

	hr = CoCreateInstance(CLSID_DOMDocument60,NULL,
		CLSCTX_INPROC_SERVER,IID_IXMLDOMDocument3,(void**)&pIXMLDoc);
	GRS_COM_CHECK2(hr,_T("创建IXMLDOMDocument3接口失败，错误码：0x%08X\n"),hr);

	hr = pIXMLDoc->createElement(_T("Root"),&pIXMLDocElement);
	GRS_COM_CHECK2(hr,_T("创建根节点失败，错误码：0x%08X\n"),hr);
	
	hr = pIXMLDoc->appendChild(pIXMLDocElement,NULL);
	GRS_COM_CHECK2(hr,_T("添加根节点到XML文档中失败，错误码：0x%08X\n"),hr);

	pIXMLNode = pIXMLDocElement;
	pIXMLDocElement = NULL;
	int iCommand = 0;
	do 
	{
		iCommand = GetCommand(pIXMLNode);
	
		if(GRS_NEW_ATTR == iCommand || GRS_NEW_NODE == iCommand )
		{
			GRS_PRINTF(_T("\t请输入名称："));
			while(S_OK != StringCchGets(pInput,GRS_MAX_INPUT));	
		}

		switch(iCommand)
		{
		case GRS_NEW_ATTR:
			hr = pIXMLDoc->createAttribute(pInput,&pAttribute);
			GRS_COM_CHECK2(hr,_T("属性\"\"创建失败，错误码：0x%08X\n"),pInput,hr);

			GRS_PRINTF(_T("\t请输入值："));
			while(S_OK != StringCchGets(pInput,GRS_MAX_INPUT));
			
			StringCchLength(pInput,GRS_MAX_INPUT,&szLen);
			if(NULL != pInput[0] && szLen > 0)
			{
				hr = pAttribute->put_text(pInput);
				GRS_COM_CHECK2(hr,_T("设置属性值失败，错误码：0x%08X\n"),hr);
			}
			
			hr = pIXMLNode->get_attributes(&pAttributeSet);

			GRS_COM_CHECK2(hr,_T("获取节点属性列表失败，错误码：0x%08X\n"),hr);
			hr = pAttributeSet->setNamedItem(pAttribute,NULL);
			GRS_COM_CHECK2(hr,_T("添加属性失败，错误码：0x%08X\n"),hr);
			GRS_SAFERELEASE(pAttributeSet);
			GRS_SAFERELEASE(pAttribute);
			break;
		case GRS_NEW_NODE:
			hr = pIXMLDoc->createElement(pInput,&pINewNode);
			GRS_COM_CHECK2(hr,_T("标签\"\"创建失败，错误码：0x%08X\n"),pInput,hr);
			GRS_PRINTF(_T("\t请输入值："));
			while(S_OK != StringCchGets(pInput,GRS_MAX_INPUT));
			StringCchLength(pInput,GRS_MAX_INPUT,&szLen);
			if(NULL != pInput[0] && szLen > 0)
			{
				hr = pINewNode->put_text(pInput);
				GRS_COM_CHECK2(hr,_T("设置元素值失败，错误码：0x%08X\n"),hr);
			}

			hr = pIXMLNode->appendChild(pINewNode,NULL);
			GRS_COM_CHECK2(hr,_T("添加节点失败，错误码：0x%08X\n"),hr);

			GRS_SAFERELEASE(pIXMLNode);
			pIXMLNode = pINewNode;
			pINewNode = NULL;
			break;
		case GRS_PARENT_NODE:
			if(NULL != pIXMLNode)
			{
				IXMLDOMNode* pIParent = NULL;
				hr = pIXMLNode->get_parentNode(&pIParent);
				GRS_COM_CHECK2(hr,_T("获取当前节点的父节点失败，错误码：0x%08X\n"),hr);
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
	
	GRS_PRINTF(_T("\t请输入要保存的XML文件名称："));
	while(S_OK != StringCchGets(pInput,GRS_MAX_INPUT));	
	StringCchLength(pInput,GRS_MAX_INPUT,&szLen);

	if(NULL == pInput[0] || szLen < 1)
	{//使用默认文件名称
		GRS_PRINTF(_T("使用默认文件名称保存XML文档.\n"));
		StringCchCopy(pInput,GRS_MAX_INPUT,_T("WorkWithDOM.xml"));
	}

	vtFile.vt = VT_BSTR;
	vtFile.bstrVal = SysAllocString(pInput);
	hr = pIXMLDoc->save(vtFile);
	SysFreeString(vtFile.bstrVal);
	GRS_COM_CHECK2(hr,_T("保存XML文档\"\"失败：错误码：0x%08X\n"),pInput,hr);

	//打开文件，html使用默认浏览器打开
	ShellExecute(NULL,_T("open"),pInput,NULL,NULL,SW_SHOWNORMAL);

CLEAR_UP:
	GRS_SAFERELEASE(pIXMLDoc);
	GRS_SAFERELEASE(pIXMLDocElement);
	GRS_SAFERELEASE(pIXMLNode);
	GRS_SAFERELEASE(pINewNode);
	GRS_SAFERELEASE(pAttributeSet);	//属性列表
	GRS_SAFERELEASE(pAttribute);	//属性
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
		GRS_PRINTF(_T("请输入命令序号执行(NODE：%s)：\n"),pstrName);
		SysFreeString(pstrName);
	}
	else
	{
		GRS_PRINTF(_T("请输入命令序号执行：\n"));
	}

	GRS_PRINTF(_T("\t1-为当前的节点添加一个属性；\n"));
	GRS_PRINTF(_T("\t2-添加一个新的节点；\n"));
	GRS_PRINTF(_T("\t3-返回到上级节点；\n"));
	GRS_PRINTF(_T("\t0-结束输入；\n"));
	GRS_PRINTF(_T("\t请输入：\t"));

	TCHAR pInput[GRS_MAX_INPUT] = {};
	while(S_OK != StringCchGets(pInput,GRS_MAX_INPUT));	
	iCommand = _ttoi(pInput);

	return iCommand;
}
