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
#define GRS_COM_CHECK(hr) if(FAILED(hr)){goto CLEAR_UP;}

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

    GRS_PRINTF(_T("XML文档\"%s\"中(行:%ld,列:%ld)\"%s\"处出现错误(0x%08x),原因:%s\n")
        ,pszXMLFileName,lLine,lCol,bstrSrcText,lErrCode,bstrReason);

    SysFreeString(bstrReason);
    SysFreeString(bstrSrcText);

}

int _tmain()
{
    CoInitialize(NULL);
    GRS_USEPRINTF();

    TCHAR pXMLFile1[MAX_PATH]   = _T("xml_1.xml"); 
    TCHAR pXMLFile2[MAX_PATH]   = _T("XML_2.xml");
    TCHAR pXSDFile[MAX_PATH]    = _T("test.xsd");
    
    IXMLDOMDocument2*           pXMLDoc         = NULL;
    IXMLDOMParseError*          pXMLErr         = NULL;
    IXMLDOMDocument2*           pSchemaDoc      = NULL; 
    IXMLDOMSchemaCollection2*   pXSDCollection  = NULL;
    VARIANT_BOOL                bRet            = VARIANT_FALSE;
    HRESULT                     hr              = S_OK;           
    VARIANT                     vtXMLFileName   = {};
    VARIANT                     vtXSDFile       = {};
    VARIANT                     vtSchema        = {};
    VARIANT                     vtSchemaDoc     = {};

    //1.xml 内关联了 schema 文件，直接判断加载是否成功 
    //创建XML Dom 的com接口 标准的com初始化调用,需要注意的是为了能够支持XSD必须创建
    //DOMDocument40对象的IXMLDomDocument2及以上的接口
    hr = CoCreateInstance(CLSID_DOMDocument40,NULL,
        CLSCTX_INPROC_SERVER,IID_IXMLDOMDocument2,(void**)&pXMLDoc);
    if(FAILED(hr))
    {
        GRS_PRINTF(_T("无法创建IXMLDOMDocument接口,错误码:0x%08X\n"),hr);
        goto CLEAR_UP;
    }
    //关闭同步特性
    hr = pXMLDoc->put_async(VARIANT_FALSE);
    //打开解析时校验的特性
    hr = pXMLDoc->put_validateOnParse(VARIANT_TRUE);
    // 加载和判断 
    vtXMLFileName.vt = VT_BSTR;
    vtXMLFileName.bstrVal = SysAllocString(pXMLFile1);
    hr = pXMLDoc->load(vtXMLFileName,&bRet); 
    SysFreeString(vtXMLFileName.bstrVal);
    if( VARIANT_TRUE != bRet  || FAILED(hr) )
    {
        pXMLDoc->get_parseError(&pXMLErr);
        ReportDocErr(pXMLFile1,pXMLErr);
        goto CLEAR_UP;
    }
    GRS_PRINTF(_T("%s 符合 %s 架构定义的要求,验证成功!\n"),pXMLFile1,pXSDFile);

    GRS_SAFERELEASE(pXMLErr);
    GRS_SAFERELEASE(pXMLDoc);

    //2.xml 内没有关联 schema 文件，需要程序关联 
    hr = CoCreateInstance(CLSID_XMLSchemaCache40,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IXMLDOMSchemaCollection2,
            (void**)&pXSDCollection);
    GRS_COM_CHECK(hr);
        
    //方法一: 加载 schema 文件为DOMDocument
    //hr = CoCreateInstance(CLSID_DOMDocument40,NULL,
    //    CLSCTX_INPROC_SERVER,IID_IXMLDOMDocument2,(void**)&pSchemaDoc);
    //GRS_COM_CHECK(hr);

    //pSchemaDoc->put_async(VARIANT_FALSE); 
    //
    //vtXSDFile.vt = VT_BSTR;
    //vtXSDFile.bstrVal = SysAllocString(pXSDFile);
    //hr = pSchemaDoc->load(vtXSDFile,&bRet); 
    //SysFreeString(vtXSDFile.bstrVal);
    //if( VARIANT_TRUE != bRet  || FAILED(hr) )
    //{
    //    pSchemaDoc->get_parseError(&pXMLErr);
    //    ReportDocErr(pXSDFile,pXMLErr);
    //    goto CLEAR_UP;
    //}
  
    //vtSchemaDoc.vt = VT_UNKNOWN;
    //vtSchemaDoc.punkVal = pSchemaDoc;
    //hr = pXSDCollection->add(_T(""), vtSchemaDoc); 

    //方法二:让架构集合接口直接加载XSD文件,这种方式在获取XSD本身错误信息时有些问题
    vtXSDFile.vt = VT_BSTR;
    vtXSDFile.bstrVal = SysAllocString(pXSDFile);
    hr = pXSDCollection->add(_T(""),vtXSDFile); 
    SysFreeString(vtXSDFile.bstrVal);
    GRS_COM_CHECK(hr);

    // 创建XML文档
    hr = CoCreateInstance(CLSID_DOMDocument40,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IXMLDOMDocument2,
        (void**)&pXMLDoc);

    if(FAILED(hr))
    {
        GRS_PRINTF(_T("无法创建IXMLDOMDocument2接口,错误码:0x%08X\n"),hr);
        goto CLEAR_UP;
    }

    pXMLDoc->put_async(VARIANT_FALSE);
    pXMLDoc->put_validateOnParse(VARIANT_TRUE);

    // 关联 xml 和 schema 
    vtSchema.vt = VT_DISPATCH;
    vtSchema.pdispVal = pXSDCollection;
    hr = pXMLDoc ->putref_schemas(vtSchema); 
    //GRS_COM_CHECK(hr);
    // 加载文件和判断 
    vtXMLFileName.vt = VT_BSTR;
    vtXMLFileName.bstrVal = SysAllocString(pXMLFile2);
    hr = pXMLDoc ->load (vtXMLFileName,&bRet); 
    SysFreeString(vtXMLFileName.bstrVal);
    if( VARIANT_TRUE != bRet  || FAILED(hr) )
    {
        pXMLDoc->get_parseError(&pXMLErr);
        ReportDocErr(pXMLFile2,pXMLErr);
        goto CLEAR_UP;
    }

    GRS_PRINTF(_T("%s 符合 %s 架构定义的要求,验证成功!\n"),pXMLFile2,pXSDFile);
CLEAR_UP:
    GRS_SAFERELEASE(pXMLDoc);
    GRS_SAFERELEASE(pXMLErr);
    GRS_SAFERELEASE(pXSDCollection);
    GRS_SAFERELEASE(pSchemaDoc);
    _tsystem(_T("PAUSE"));
    CoUninitialize();
    return 0;
}