#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#include <msxml6.h>
#pragma comment (lib,"msxml6.lib")

#include <comutil.h>
#ifdef _DEBUG
#pragma comment (lib,"comsuppwd.lib")
#else
#pragma comment (lib,"comsuppw.lib")
#endif


#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {};DWORD dwRead = 0;
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_SAFERELEASE(I) if(NULL != I){I->Release();I=NULL;}
#define GRS_COM_CHECK(hr) if(FAILED(hr)){goto CLEAR_UP;}

#define MAX_OUTPUT_LENGTH	809600

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
    ::CoInitialize(NULL);
	GRS_USEPRINTF();

	TCHAR						pXMLFile[MAX_PATH]	= _T("cdcatalog.xml"); 
	TCHAR						pXSLFile[MAX_PATH]	= _T("cdcatalog.xsl");
	TCHAR						pOutFile[MAX_PATH]	= _T("cdcatalog.html");
	VARIANT						vtbStr				= {};
	IXMLDOMParseError*          pIXMLErr			= NULL;
	IXMLDOMDocument*			pIXMLDoc			= NULL;
	IXMLDOMDocument*			pIXSLDoc			= NULL;
	IXSLTemplate*				pIXSLTmp			= NULL;
	IXSLProcessor*				pIXSLProc			= NULL;
	
	VARIANT_BOOL				vtbRet				= NULL;
	VARIANT						vtOutput			= {};
	size_t						szLen				= 0;
	DWORD						dwWriteLen			= 0;
	BYTE						pbtUnicode[]		= {0xFF,0xFE};
	HANDLE						hOutFile			= INVALID_HANDLE_VALUE;
	
	IStream*					pIStream			= NULL;
	//void*						pOutput				= NULL;	

	HRESULT hr = CoCreateInstance(CLSID_DOMDocument,NULL,
		CLSCTX_INPROC_SERVER,IID_IXMLDOMDocument,(void**)&pIXMLDoc);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("无法创建IXMLDOMDocument接口,错误码:0x%08X\n"),hr);
		goto CLEAR_UP;
	}

	vtbStr.vt = VT_BSTR;
	vtbStr.bstrVal = SysAllocString(pXMLFile);
	hr = pIXMLDoc->load(vtbStr,&vtbRet);
	SysFreeString(vtbStr.bstrVal);
	if( VARIANT_TRUE != vtbRet  || FAILED(hr) )
	{
		pIXMLDoc->get_parseError(&pIXMLErr);
		ReportDocErr(pXMLFile,pIXMLErr);
		goto CLEAR_UP;
	}
	//使用独立线程模型的XMLDoc对象接口
	hr = CoCreateInstance(CLSID_FreeThreadedDOMDocument,NULL,
		CLSCTX_INPROC_SERVER,IID_IXMLDOMDocument,(void**)&pIXSLDoc);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("无法创建IXMLDOMDocument接口,错误码:0x%08X\n"),hr);
		goto CLEAR_UP;
	}

	vtbStr.bstrVal = SysAllocString(pXSLFile);
	hr = pIXSLDoc->load(vtbStr,&vtbRet);
	SysFreeString(vtbStr.bstrVal);
	
	if( VARIANT_TRUE != vtbRet  || FAILED(hr) )
	{
		pIXSLDoc->get_parseError(&pIXMLErr);
		ReportDocErr(pXSLFile,pIXMLErr);
		goto CLEAR_UP;
	}

	hr = CoCreateInstance(CLSID_XSLTemplate,NULL,
		CLSCTX_INPROC_SERVER,IID_IXSLTemplate,(void**)&pIXSLTmp);
	if(FAILED(hr))
	{
		GRS_PRINTF(_T("无法创建IXSLTemplate接口,错误码:0x%08X\n"),hr);
		goto CLEAR_UP;
	}

	//以下几步省略错误判断和相应处理
	hr = pIXSLTmp->putref_stylesheet(pIXSLDoc);
	hr = pIXSLTmp->createProcessor(&pIXSLProc);
//==================================================================================
    //第一种获取转换结果的方法
	hr = pIXSLProc->put_input(_variant_t((IUnknown*)pIXMLDoc));
	hr = pIXSLProc->transform(&vtbRet);
	hr = pIXSLProc->get_output(&vtOutput);

	//Sleep(1000);
	GRS_PRINTF(_T("%s"),vtOutput.bstrVal);
	hOutFile = CreateFile(pOutFile,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(INVALID_HANDLE_VALUE == hOutFile)
	{
		GRS_PRINTF(_T("无法创建输出文件\"%s\"，错误码：0x%08X\n"),pOutFile,GetLastError());
		goto CLEAR_UP;
	}
	//写入UNICODE头
	WriteFile(hOutFile,pbtUnicode,2,&dwWriteLen,NULL);
	
	StringCchLength(vtOutput.bstrVal,MAX_OUTPUT_LENGTH,&szLen);
	if(!WriteFile(hOutFile,vtOutput.bstrVal,szLen * sizeof(WCHAR),&dwWriteLen,NULL))
	{
		GRS_PRINTF(_T("写入到输出文件\"%s\"失败，错误码：0x%08X\n"),pOutFile,GetLastError());
		goto CLEAR_UP;
	}
	CloseHandle(hOutFile);
	hOutFile = INVALID_HANDLE_VALUE;

	//打开文件，html使用默认浏览器打开
	ShellExecute(NULL,_T("open"),pOutFile,NULL,NULL,SW_SHOWNORMAL);
//==================================================================================	

	//第二种取得转换结果的方式

	//CreateStreamOnHGlobal(NULL,TRUE,&pIStream);		
	//pIXSLProc->put_output(_variant_t(pIStream));

	//hr = pIXSLProc->put_input(_variant_t((IUnknown*)pIXMLDoc));		
	//pIXSLProc->addParameter(_bstr_t("maxprice"),_variant_t("35"),_bstr_t(""));		
	

	////get results of transformation and print it to stdout
	//HGLOBAL hg = NULL;
	//pIStream->Write((void const*)_T("\0"),1,0);
	//GetHGlobalFromStream(pIStream,&hg);	
	//pOutput = GlobalLock(hg);
	//GRS_PRINTF(_T("%s"),(WCHAR*)pOutput);
	//GlobalUnlock(hg);
	//pOutPut中就包含了最终的转换结果字符串



CLEAR_UP:
	
	if(INVALID_HANDLE_VALUE != hOutFile)
	{
		CloseHandle(hOutFile);
		hOutFile = INVALID_HANDLE_VALUE;
	}
	GRS_SAFERELEASE(pIStream);
	GRS_SAFERELEASE(pIXSLProc);
	GRS_SAFERELEASE(pIXSLTmp);
	GRS_SAFERELEASE(pIXMLErr);
	GRS_SAFERELEASE(pIXMLDoc);
	GRS_SAFERELEASE(pIXSLDoc);

    _tsystem(_T("PAUSE"));
    ::CoUninitialize();
    return 0;
}