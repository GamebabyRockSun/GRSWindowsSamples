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

    GRS_PRINTF(_T("XML�ĵ�\"%s\"��(��:%ld,��:%ld)\"%s\"�����ִ���(0x%08x),ԭ��:%s\n")
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

    //1.xml �ڹ����� schema �ļ���ֱ���жϼ����Ƿ�ɹ� 
    //����XML Dom ��com�ӿ� ��׼��com��ʼ������,��Ҫע�����Ϊ���ܹ�֧��XSD���봴��
    //DOMDocument40�����IXMLDomDocument2�����ϵĽӿ�
    hr = CoCreateInstance(CLSID_DOMDocument40,NULL,
        CLSCTX_INPROC_SERVER,IID_IXMLDOMDocument2,(void**)&pXMLDoc);
    if(FAILED(hr))
    {
        GRS_PRINTF(_T("�޷�����IXMLDOMDocument�ӿ�,������:0x%08X\n"),hr);
        goto CLEAR_UP;
    }
    //�ر�ͬ������
    hr = pXMLDoc->put_async(VARIANT_FALSE);
    //�򿪽���ʱУ�������
    hr = pXMLDoc->put_validateOnParse(VARIANT_TRUE);
    // ���غ��ж� 
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
    GRS_PRINTF(_T("%s ���� %s �ܹ������Ҫ��,��֤�ɹ�!\n"),pXMLFile1,pXSDFile);

    GRS_SAFERELEASE(pXMLErr);
    GRS_SAFERELEASE(pXMLDoc);

    //2.xml ��û�й��� schema �ļ�����Ҫ������� 
    hr = CoCreateInstance(CLSID_XMLSchemaCache40,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IXMLDOMSchemaCollection2,
            (void**)&pXSDCollection);
    GRS_COM_CHECK(hr);
        
    //����һ: ���� schema �ļ�ΪDOMDocument
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

    //������:�üܹ����Ͻӿ�ֱ�Ӽ���XSD�ļ�,���ַ�ʽ�ڻ�ȡXSD���������Ϣʱ��Щ����
    vtXSDFile.vt = VT_BSTR;
    vtXSDFile.bstrVal = SysAllocString(pXSDFile);
    hr = pXSDCollection->add(_T(""),vtXSDFile); 
    SysFreeString(vtXSDFile.bstrVal);
    GRS_COM_CHECK(hr);

    // ����XML�ĵ�
    hr = CoCreateInstance(CLSID_DOMDocument40,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IXMLDOMDocument2,
        (void**)&pXMLDoc);

    if(FAILED(hr))
    {
        GRS_PRINTF(_T("�޷�����IXMLDOMDocument2�ӿ�,������:0x%08X\n"),hr);
        goto CLEAR_UP;
    }

    pXMLDoc->put_async(VARIANT_FALSE);
    pXMLDoc->put_validateOnParse(VARIANT_TRUE);

    // ���� xml �� schema 
    vtSchema.vt = VT_DISPATCH;
    vtSchema.pdispVal = pXSDCollection;
    hr = pXMLDoc ->putref_schemas(vtSchema); 
    //GRS_COM_CHECK(hr);
    // �����ļ����ж� 
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

    GRS_PRINTF(_T("%s ���� %s �ܹ������Ҫ��,��֤�ɹ�!\n"),pXMLFile2,pXSDFile);
CLEAR_UP:
    GRS_SAFERELEASE(pXMLDoc);
    GRS_SAFERELEASE(pXMLErr);
    GRS_SAFERELEASE(pXSDCollection);
    GRS_SAFERELEASE(pSchemaDoc);
    _tsystem(_T("PAUSE"));
    CoUninitialize();
    return 0;
}