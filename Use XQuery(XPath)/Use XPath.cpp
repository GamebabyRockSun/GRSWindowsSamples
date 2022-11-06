#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#include <msxml6.h>
#pragma comment (lib,"msxml6.lib")

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {};DWORD dwRead = 0;
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_SAFERELEASE(I) if(NULL != I){I->Release();I=NULL;}
#define GRS_COM_CHECK(hr) if(FAILED(hr)){goto CLEAR_UP;}
//����XML�������
#define GRS_MAX_XML_DEP  128

VOID ReportDocErr(LPCTSTR pszXMLFileName,IXMLDOMParseError* pIDocErr)
{
	GRS_USEPRINTF();
	long lLine = 0;
	long lCol = 0;
	LONG lErrCode = 0;
	BSTR bstrReason = NULL;
	BSTR bstrSrcText = NULL;
	pIDocErr->get_line(&lLine);
	pIDocErr->get_linepos(&lCol);
	pIDocErr->get_reason(&bstrReason);
	pIDocErr->get_srcText(&bstrSrcText);

	GRS_PRINTF(_T("XML�ĵ�\"%s\"��(��:%ld,��:%ld)\"%s\"�����ִ���(0x%08x),ԭ��:%s\n")
		,pszXMLFileName,lLine,lCol,bstrSrcText,lErrCode,bstrReason);

	SysFreeString(bstrReason);
	SysFreeString(bstrSrcText);

}

int _tmain()
{
	::CoInitialize(NULL);
	GRS_USEPRINTF();
	TCHAR						pXMLFile[MAX_PATH]	= _T("cdcatalog.xml"); 
	IXMLDOMParseError*			pIXMLErr			= NULL;
	IXMLDOMDocument3*			pIXMLDoc			= NULL;
	IXMLDOMElement*				pIXMLDocElement		= NULL;
	IXMLDOMNodeList*			pIXMLNodes			= NULL;
	IXMLDOMNode*				pIXMLNode			= NULL;
	IXMLDOMNodeList*			pNodeList			= NULL;
	IXMLDOMNode*				pCNode				= NULL;
	IXMLDOMNamedNodeMap*		pAttributeSet		= NULL;	//�����б�
	IXMLDOMNode*				pAttribute			= NULL;	//����
	DOMNodeType					NodeType	        = NODE_ELEMENT;

	//����ѭ�����������ӽڵ�Ķ�ջ
	UINT32						nIndex				= 0;
	IXMLDOMNodeList*			NodesStack[GRS_MAX_XML_DEP] = {};
	UINT32						IndexStack[GRS_MAX_XML_DEP]	= {};
	long						CountStack[GRS_MAX_XML_DEP] = {};
	long						i					= 0;

	VARIANT_BOOL                bRet				= VARIANT_FALSE;
	HRESULT                     hr					= S_OK;           
	VARIANT                     vtXMLFile			= {};
	VARIANT						vtStrVal			= {};
	long						lChildNodeCnt		= 0;
	long						lNodesCnt			= 0;
	long						lIndex				= 0;
	long						lAttrCnt			= 0;

	BSTR						pbstrName			= NULL;
	BSTR						pbstrNValue			= NULL;
	BSTR						pbstrAName			= NULL;
	BSTR						pbstrAValue			= NULL;
	
	//TCHAR						pXQuery[]			
	//		= _T("for $h in /catalog/cd \
	//				where $h/country = \'USA\' \
	//				order by $h/price \
	//				return $h ");

	//��������country=USA��Ԫ��		
	TCHAR						pXQuery[]			
			= _T("/catalog/cd[country='USA']");  //

	hr = CoCreateInstance(CLSID_DOMDocument60,NULL,
		CLSCTX_INPROC_SERVER,IID_IXMLDOMDocument3,(void**)&pIXMLDoc);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("�޷�����IXMLDOMDocument�ӿ�,������:0x%08X\n"),hr);
		goto CLEAR_UP;
	}

	vtXMLFile.vt = VT_BSTR;
	vtXMLFile.bstrVal = SysAllocString(pXMLFile);

	hr = pIXMLDoc->load(vtXMLFile,&bRet);
	SysFreeString(vtXMLFile.bstrVal);
	if( VARIANT_TRUE != bRet  || FAILED(hr) )
	{
		pIXMLDoc->get_parseError(&pIXMLErr);
		ReportDocErr(pXMLFile,pIXMLErr);
		goto CLEAR_UP;
	}
    //=============================================================================
	vtStrVal.vt = VT_BSTR;
	vtStrVal.bstrVal = SysAllocString(_T("XPath"));
	hr = pIXMLDoc->setProperty(_T("SelectionLanguage"),vtStrVal);
	SysFreeString(vtStrVal.bstrVal);
	hr = pIXMLDoc->get_documentElement(&pIXMLDocElement);
	hr = pIXMLDocElement->selectNodes(pXQuery,&pIXMLNodes);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("ִ��XPath\"%s\"ʧ�ܣ������룺0x%08X\n"),pXQuery,hr);
		goto CLEAR_UP;
	}
	//=============================================================================
	hr = pIXMLNodes->get_length(&lNodesCnt);
	if(FAILED(hr))
	{
		goto CLEAR_UP;
	}
	GRS_PRINTF(_T("Nodes Count: %ld\n"),lNodesCnt);

	NodesStack[nIndex] = pIXMLNodes;
	IndexStack[nIndex] = i;
	CountStack[nIndex] = lNodesCnt;
	++ nIndex;
	while( nIndex > 0 && nIndex < GRS_MAX_XML_DEP )
	{
		while( i < lNodesCnt )
		{
			//ȡ����һ���ڵ�
			hr = pIXMLNodes->get_item(i,&pIXMLNode);
			GRS_COM_CHECK(hr)
				hr = pIXMLNode->get_attributes(&pAttributeSet);
			GRS_COM_CHECK(hr);

			//ȡ�ýڵ�����
			hr = pIXMLNode->get_nodeName(&pbstrName);
			GRS_COM_CHECK(hr);
			for(UINT32 k = 0; k < nIndex; k ++)
			{
				GRS_PRINTF(_T("\t"));
			}
			GRS_PRINTF(_T("Node[%ld]: \"%s\""),i,pbstrName);
			SysFreeString(pbstrName);

			if(NULL != pAttributeSet && SUCCEEDED(pAttributeSet->get_length(&lAttrCnt)) && lAttrCnt > 0 )
			{//ȡ���������������Ŀǰ�̶��������±���������i
				for(long j = 0; j < lAttrCnt; j ++)
				{
					hr = pAttributeSet->get_item(j,&pAttribute);
					GRS_COM_CHECK(hr);
					if(NULL != pAttribute)
					{
						hr = pAttribute->get_nodeName(&pbstrAName);
						GRS_COM_CHECK(hr);
						hr = pAttribute->get_text(&pbstrAValue);
						GRS_COM_CHECK(hr);

						GRS_PRINTF(_T("\t(Attribute[%ld]: \"%s\" = \"%s\")"),j,pbstrAName,pbstrAValue);

						SysFreeString(pbstrAName);
						SysFreeString(pbstrAValue);
					}
				}
			}

			//ȡ���ӽڵ��б�				
			hr = pIXMLNode->get_firstChild(&pCNode);
			GRS_COM_CHECK(hr);
			hr = pIXMLNode->get_childNodes(&pNodeList);
			GRS_COM_CHECK(hr);

			if( NULL != pCNode )
			{
				if( SUCCEEDED(pCNode->get_nodeType(&NodeType)) 
					&& NODE_TEXT != NodeType 
					&& NULL != pNodeList
					&& SUCCEEDED(pNodeList->get_length(&lChildNodeCnt))
					&& lChildNodeCnt > 0
					)
				{//�ӽڵ�����ݹ鴴��Table
					//������ջ
					NodesStack[nIndex] = pIXMLNodes;
					IndexStack[nIndex] = ++ i;
					CountStack[nIndex] = lNodesCnt;
					++ nIndex;

					pIXMLNodes = pNodeList;
					pNodeList = NULL;			//��������ڵ�
					i = 0UL;
					lNodesCnt = lChildNodeCnt;
					//�����ӽڵ��Node,����
					GRS_PRINTF(_T("\n"));
				}
				else
				{
					hr = pCNode->get_text(&pbstrNValue);
					GRS_COM_CHECK(hr);
					GRS_PRINTF(_T("\tValue: \"%s\"\n"),pbstrNValue);
					SysFreeString(pbstrNValue);
					++ i;
				}
			}
			else
			{
				++ i;
			}		

			//����ѭ������
			GRS_SAFERELEASE(pCNode);
			GRS_SAFERELEASE(pIXMLNode);
			GRS_SAFERELEASE(pNodeList);
			GRS_SAFERELEASE(pAttributeSet);
			GRS_SAFERELEASE(pAttribute);
		}

		--nIndex;
		GRS_SAFERELEASE(pIXMLNodes);	//�ͷ�֮ǰ�Ľӿ���
		//������ջ
		pIXMLNodes  = NodesStack[nIndex]; 
		i			= IndexStack[nIndex];
		lNodesCnt	= CountStack[nIndex];
	}

CLEAR_UP:
	GRS_SAFERELEASE(pCNode);
	GRS_SAFERELEASE(pIXMLNode);
	GRS_SAFERELEASE(pNodeList);
	GRS_SAFERELEASE(pAttributeSet);
	GRS_SAFERELEASE(pAttribute);
	GRS_SAFERELEASE(pIXMLErr);
	GRS_SAFERELEASE(pIXMLDocElement);
	GRS_SAFERELEASE(pIXMLDoc);
	_tsystem(_T("PAUSE"));
	::CoUninitialize();
	return 0;
}