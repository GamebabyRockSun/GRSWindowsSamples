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

#define GRS_SAFERELEASE(I) if(NULL != (I)){(I)->Release();(I)=NULL;}
#define GRS_COM_CHECK2(hr) if(FAILED(hr)){goto CLEAR_UP;}

//待解析XML最大的深度
#define GRS_MAX_XML_DEP  128

VOID ReportDocErr(LPCTSTR pszXMLFileName,IXMLDOMParseError* pIDocErr)
{
    GRS_USEPRINTF();
    long lLine = 0;
    long lCol = 0;
    LONG lErrCode = 0;
    BSTR bstrReason = NULL;

    pIDocErr->get_line(&lLine);
    pIDocErr->get_linepos(&lCol);
    pIDocErr->get_reason(&bstrReason);

    GRS_PRINTF(_T("XML文档\"%s\"中(行:%ld,列:%ld)处出现错误(0x%08x),原因:%s\n")
        ,pszXMLFileName,lLine,lCol,lErrCode,bstrReason);

    SysFreeString(bstrReason);
    
}

int _tmain(int argc,TCHAR* argv[])
{
    GRS_USEPRINTF();
    if(argc < 2 )
    {
        GRS_PRINTF(_T("Usage:XMLParser XMLFileName\n"));
        GRS_PRINTF(_T("\tXMLFileName :指定一个需要解析的XML文档的文件名\n"));
        _tsystem(_T("PAUSE"));
        return 0;
    }

    //初始化COM
    CoInitialize(NULL); 

    BSTR					bstrAName		= NULL;
    BSTR					bstrAValue		= NULL;
    BSTR					bstrNName		= NULL;
    BSTR					bstrNValue		= NULL;
    long                    lNodeCnt        = 0;
    long                    lChildNodeCnt   = 0;
    long                    lAttrCnt        = 0;

    IXMLDOMParseError*      pXMLErr         = NULL;
    IXMLDOMDocument*		pXMLDoc			= NULL;
    IXMLDOMElement*			pElement		= NULL;
    IXMLDOMNode*			pNode			= NULL;
    IXMLDOMNodeList*		pNodes			= NULL;
    IXMLDOMNodeList*		pNodeList		= NULL;
    IXMLDOMNode*			pCNode			= NULL;
    IXMLDOMNamedNodeMap*	pAttributeSet	= NULL;	//属性列表
    IXMLDOMNode*			pAttribute		= NULL;	//属性
    DOMNodeType	            NodeType	    = NODE_ELEMENT;

    //用于循环遍历所有子节点的堆栈
    UINT32					nIndex				= 0;
    IXMLDOMNodeList*		NodesStack[GRS_MAX_XML_DEP] = {};
    UINT32					IndexStack[GRS_MAX_XML_DEP] = {};
    long					CountStack[GRS_MAX_XML_DEP] = {};
    long					i					= 0;

    
   
    VARIANT_BOOL bSuccess = VARIANT_TRUE;

    HRESULT hr = S_OK;
    TCHAR* pXMLData = NULL;
    TCHAR* pXMLString = NULL;
    HANDLE hXMLFile = INVALID_HANDLE_VALUE;




    //打开XML文件
    hXMLFile = CreateFile(argv[1],
        GENERIC_READ,          
        0,                     
        NULL,                  
        OPEN_EXISTING,         
        NULL,
        NULL);

    DWORD dwFileSize = GetFileSize(hXMLFile,NULL);
    //多分配两个字节用于存储'\0'
    pXMLData = (TCHAR*)GRS_CALLOC(dwFileSize + 2);
    DWORD dwReaded = 0;
    ReadFile(hXMLFile,pXMLData,dwFileSize,&dwReaded,NULL);
    CloseHandle(hXMLFile);

    //跳过开头的UNICODE标记0xFEFF
    if(0xFEFF == (USHORT)pXMLData[0])
    {
        pXMLString = pXMLData + 1;
    }
    else
    {
        pXMLString = pXMLData;
    }

    //创建XML Dom 的com接口 标准的com初始化调用
    hr = CoCreateInstance(CLSID_DOMDocument,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IXMLDOMDocument,
        (void**)&pXMLDoc);

    if(FAILED(hr))
    {
        GRS_PRINTF(_T("无法创建IXMLDOMDocument接口,错误码:0x%08X\n"),hr);
        goto CLEAR_UP;
    }

    //解析XML 文本 
    hr = pXMLDoc->loadXML(pXMLString,&bSuccess);

    GRS_SAFEFREE(pXMLData);

    if( VARIANT_TRUE != bSuccess  || FAILED(hr) )
    {
        pXMLDoc->get_parseError(&pXMLErr);
        ReportDocErr(argv[1],pXMLErr);
        goto CLEAR_UP;
    }

    //取得顶层元素
    hr = pXMLDoc->get_documentElement(&pElement);
    if(FAILED(hr))
    {
        goto CLEAR_UP;
    }

    hr = pElement ->QueryInterface(IID_IXMLDOMNode,(void**)&pNode);
    if(FAILED(hr))
    {
        goto CLEAR_UP;
    }

    hr = pNode->get_nodeName(&bstrNName);
    if(FAILED(hr))
    {
        goto CLEAR_UP;
    }

    GRS_PRINTF(_T("XML Root: \"%s\" \n"),bstrNName);
    SysFreeString(bstrNName);
    hr = pNode->get_childNodes(&pNodes);
    if(FAILED(hr))
    {
        goto CLEAR_UP;
    }
    hr = pNodes->get_length(&lNodeCnt);
    if(FAILED(hr))
    {
        goto CLEAR_UP;
    }
    GRS_PRINTF(_T("Root Have Nodes Count: %ld\n"),lNodeCnt);
    
    NodesStack[nIndex] = pNodes;
    IndexStack[nIndex] = i;
    CountStack[nIndex] = lNodeCnt;
    ++ nIndex;
    while( nIndex > 0 && nIndex < GRS_MAX_XML_DEP )
    {
        while( i < lNodeCnt )
        {
            //取得下一个节点
            hr = pNodes->get_item(i,&pNode);
            GRS_COM_CHECK2(hr)
            hr = pNode->get_attributes(&pAttributeSet);
            GRS_COM_CHECK2(hr);

            //取得节点名称
            hr = pNode->get_nodeName(&bstrNName);
            GRS_COM_CHECK2(hr);
            for(UINT32 k = 0; k < nIndex; k ++)
            {
                GRS_PRINTF(_T("\t"));
            }
            GRS_PRINTF(_T("Node[%ld]: \"%s\""),i,bstrNName);
            SysFreeString(bstrNName);

            if(NULL != pAttributeSet && SUCCEEDED(pAttributeSet->get_length(&lAttrCnt)) && lAttrCnt > 0 )
            {//取属性是数组的索引目前固定的数组下标属性名是i
                for(long j = 0; j < lAttrCnt; j ++)
                {
                    hr = pAttributeSet->get_item(j,&pAttribute);
                    GRS_COM_CHECK2(hr);
                    if(NULL != pAttribute)
                    {
                        hr = pAttribute->get_nodeName(&bstrAName);
                        GRS_COM_CHECK2(hr);
                        hr = pAttribute->get_text(&bstrAValue);
                        GRS_COM_CHECK2(hr);
                        
                        GRS_PRINTF(_T("\t(Attribute[%ld]: \"%s\" = \"%s\")"),j,bstrAName,bstrAValue);

                        SysFreeString(bstrAName);
                        SysFreeString(bstrAValue);
                    }
                    GRS_SAFERELEASE(pAttribute);
                }
            }

            //取得子节点列表				
            hr = pNode->get_firstChild(&pCNode);
            GRS_COM_CHECK2(hr);
            hr = pNode->get_childNodes(&pNodeList);
            GRS_COM_CHECK2(hr);

            if( NULL != pCNode )
            {
                if( SUCCEEDED(pCNode->get_nodeType(&NodeType)) 
                    && NODE_TEXT != NodeType 
                    && NULL != pNodeList
                    && SUCCEEDED(pNodeList->get_length(&lChildNodeCnt))
                    && lChildNodeCnt > 0
                    )
                {//子节点继续递归创建Table
                    //变量入栈
                    NodesStack[nIndex] = pNodes;
                    IndexStack[nIndex] = ++ i;
                    CountStack[nIndex] = lNodeCnt;
                    ++ nIndex;

                    pNodes              = pNodeList;
                    pNodeList           = NULL;			//保护这个节点
                    i                   = 0UL;
                    lNodeCnt            = lChildNodeCnt;
                    //带有子节点的Node,换行
                    GRS_PRINTF(_T("\n"));
                }
                else
                {
                    hr = pCNode->get_text(&bstrNValue);
                    GRS_COM_CHECK2(hr);
                    GRS_PRINTF(_T("\tValue: \"%s\"\n"),bstrNValue);
                    SysFreeString(bstrNValue);
                    ++ i;
                }
            }
            else
            {
                ++ i;
            }		

            //清理循环变量
            GRS_SAFERELEASE(pCNode);
            GRS_SAFERELEASE(pNode);
            GRS_SAFERELEASE(pNodeList);
            GRS_SAFERELEASE(pAttributeSet);
            GRS_SAFERELEASE(pAttribute);
        }

        -- nIndex;
        GRS_SAFERELEASE(pNodes);	//释放之前的接口先
        //变量出栈
        pNodes  = NodesStack[nIndex]; 
        i       = IndexStack[nIndex];
        lNodeCnt = CountStack[nIndex];
    }
CLEAR_UP:
    GRS_SAFEFREE(pXMLData);
    GRS_SAFERELEASE(pXMLDoc);
    GRS_SAFERELEASE(pElement);
    GRS_SAFERELEASE(pNode);
    GRS_SAFERELEASE(pCNode);
    GRS_SAFERELEASE(pNode);
    GRS_SAFERELEASE(pNodeList);
    GRS_SAFERELEASE(pAttributeSet);
    GRS_SAFERELEASE(pAttribute);

    _tsystem(_T("PAUSE"));
    

    
    CoUninitialize();
    return 0;
}