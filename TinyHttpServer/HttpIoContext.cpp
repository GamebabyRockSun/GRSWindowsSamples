#include "StdAfx.h"
#include "HttpIoContext.h"
#include "IOCP.h"
#include "ZvLib.h"
#include "HttpSvrIoContext.h"

extern CHttpSvrIoContext * m_pHttpSvr;

CHttpIoContext::CHttpIoContext(SOCKET sock)
: CIoContext(sock)
{
	
}

CHttpIoContext::~CHttpIoContext()
{
}

void CHttpIoContext::ProcessJob(CIoBuffer * pIoBuffer)
{
	HttpProcessRequest(pIoBuffer->GetBuffer());
}

void CHttpIoContext::HttpProcessRequest(const string & strRequest)
{
	CHttpRequest Request;

	if(!Request.HttpParseRequest(strRequest))
	{
		HttpSendError("400 Bad Request", 
			"Your browser sent a request that this server could not understand.", 
			Request.bKeepAlive);
		return;
	}

	string & strType = Request.szType;
	string & strUrl = Request.szUrl;
	bool bKeepAlive = Request.bKeepAlive;
	DWORD dwRangeStart = Request.dwRangeStart;
	DWORD dwRangeEnd = Request.dwRangeEnd;

	string str;

	if( strType != "GET" )
	{
		str = strType+" to "+strUrl+" not supported.<br />";
		HttpSendError("501 Method Not Implemented", str, bKeepAlive);
		return;
	}

	string strFilePath = m_pHttpSvr->m_szRootPath;
	strFilePath += strUrl.c_str()+1;

	if(strUrl[strUrl.length()-1]=='/')
	{
		strFilePath += HTTP_DEFAULT_FILE;
	}

	HANDLE hFile;
	hFile = CreateFile(strFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, 
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hFile==INVALID_HANDLE_VALUE)
	{
		HttpSendDir(strUrl, bKeepAlive);
		return;
	}

	DWORD dwSize = GetFileSize(hFile, NULL);

	char pResponseHeader[1024];

	string strFileExt;
	string strContentType = "text/plain";
	string strState = "200 OK";
	bool bSetRange = false;
	string strContentRange;

	if(dwRangeStart || dwRangeEnd)
	{	
		if(dwRangeEnd==0)
			dwRangeEnd = dwSize-1;
		
		if(dwRangeEnd>dwSize-1 ||
			dwRangeStart>dwRangeEnd ||
			SetFilePointer(hFile, dwRangeStart, NULL, FILE_BEGIN)==0xFFFFFFFF)
		{
			CloseHandle(hFile);
			
			HttpSendError("416 Requested Range Not Satisfiable", "Requested Range Not Satisfiable", bKeepAlive);

			return;
		}	

		sprintf(pResponseHeader, "Content-Range: bytes %u-%u/%u\r\n", dwRangeStart, dwRangeEnd, dwSize);
		strContentRange = pResponseHeader;
		dwSize = dwRangeEnd-dwRangeStart+1;

		bSetRange = true;
		strState = "206 Partial Content";
	}

	int nPointPos = strFilePath.rfind(".");
	if(nPointPos != string::npos)
	{
		strFileExt = strFilePath.substr(nPointPos + 1, strFilePath.size());
		strlwr((char*)strFileExt.c_str());
		MIMETYPES::iterator it;
		it = m_pHttpSvr->MimeTypes.find(strFileExt);
		if(it != m_pHttpSvr->MimeTypes.end())
			strContentType = (*it).second;
	}

	sprintf(pResponseHeader, 
		"HTTP/1.1 %s\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Length: %d\r\n"
		"Connection: %s\r\n"
		"%s"
		"Content-Type: %s\r\n\r\n",
		strState.c_str(),
		dwSize,
		bKeepAlive?"Keep-Alive":"close",
		bSetRange?strContentRange.c_str():"",
		strContentType.c_str());

	CIoBuffer * pIoBuffer = new CIoBuffer();
	pIoBuffer->AddData(pResponseHeader);
	if(AsyncSend(pIoBuffer))
	{
		AsyncTransmitFile(hFile, dwSize, bKeepAlive);
	}
}

void CHttpIoContext::HttpSendFile(string & szFile, DWORD dwRangeStart, DWORD dwRangeEnd, bool bKeepAlive, bool bDownload)
{
	string str;

	string strFilePath = szFile;

	HANDLE hFile;
	hFile = CreateFile(strFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, 
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hFile==INVALID_HANDLE_VALUE)
	{
		HttpSendError("404 Not Found", szFile, bKeepAlive);
		return;
	}

	DWORD dwSize = GetFileSize(hFile, NULL);

	char pResponseHeader[1024];

	string strFileExt;
	string strContentType = "text/plain";
	string strState = "200 OK";
	bool bSetRange = false;
	string strContentRange;
	string szFileName;

	int i = strFilePath.rfind("/");
	if(i==strFilePath.npos)
		i = strFilePath.rfind("\\");

	szFileName = strFilePath.substr(i+1, strFilePath.npos);

	if(dwRangeStart || dwRangeEnd)
	{	
		if(dwRangeEnd==0)
			dwRangeEnd = dwSize-1;
		
		if(dwRangeEnd>dwSize-1 ||
			dwRangeStart>dwRangeEnd ||
			SetFilePointer(hFile, dwRangeStart, NULL, FILE_BEGIN)==0xFFFFFFFF)
		{
			CloseHandle(hFile);
			
			HttpSendError("416 Requested Range Not Satisfiable", "Requested Range Not Satisfiable", bKeepAlive);

			return;
		}	

		sprintf(pResponseHeader, "Content-Range: bytes %u-%u/%u\r\n", dwRangeStart, dwRangeEnd, dwSize);
		strContentRange = pResponseHeader;
		dwSize = dwRangeEnd-dwRangeStart+1;

		bSetRange = true;
		strState = "206 Partial Content";
	}

	int nPointPos = strFilePath.rfind(".");
	if(nPointPos != string::npos)
	{
		strFileExt = strFilePath.substr(nPointPos + 1, strFilePath.size());
		strlwr((char*)strFileExt.c_str());
		MIMETYPES::iterator it;
		it = m_pHttpSvr->MimeTypes.find(strFileExt);
		if(it != m_pHttpSvr->MimeTypes.end())
			strContentType = (*it).second;
	}

	sprintf(pResponseHeader, 
		"HTTP/1.1 %s\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Length: %d\r\n"
		"Content-Disposition: %s; filename=\"%s\"\r\n" 
		"Connection: %s\r\n"
		"%s"
		"Content-Type: %s\r\n\r\n",
		strState.c_str(),
		dwSize,
		bDownload?"attachment":"inline",
		szFileName.c_str(),
		bKeepAlive?"Keep-Alive":"close",
		bSetRange?strContentRange.c_str():"",
		strContentType.c_str());

	CIoBuffer * pIoBuffer = new CIoBuffer();
	pIoBuffer->AddData(pResponseHeader);
	if(AsyncSend(pIoBuffer))
	{
		AsyncTransmitFile(hFile, dwSize, bKeepAlive);
	}
}

void CHttpIoContext::HttpSendDir(string & strUrl, bool bKeepAlive)
{
	string strDirPath = m_pHttpSvr->m_szRootPath;

	strDirPath += strUrl.c_str()+1;

	if(strDirPath[strDirPath.size()-1]!='/')
		strDirPath += '/';

	strDirPath+="*.*";

	WIN32_FIND_DATA fd;
	HANDLE hd = FindFirstFile(strDirPath.c_str(), &fd);
	if(hd==INVALID_HANDLE_VALUE)
	{
		string str = "The requested URL "+strUrl+" was not found on this server.";
		HttpSendError("404 Not Found", str, bKeepAlive);
		return;
	}

	CIoBuffer * pIoBuffer = new CIoBuffer();

	pIoBuffer->AddData("<html>\r\n <head>\r\n  <title>Index of ");
	pIoBuffer->AddData(strUrl);
	pIoBuffer->AddData("</title>\r\n </head>\r\n <body>\r\n<h1>Index of ");
	pIoBuffer->AddData(strUrl);
	pIoBuffer->AddData("</h1>\r\n<ul>");

	if(strUrl[strUrl.size()-1]!='/')
		strUrl += '/';

	string str;

	do
	{
		pIoBuffer->AddData("<li><a href=\"");
		str = strUrl+fd.cFileName;
		HttpEncodeURL(str);
		pIoBuffer->AddData(str);
		pIoBuffer->AddData("\"> ");
		pIoBuffer->AddData(fd.cFileName);
		pIoBuffer->AddData("</a></li>\r\n");
	}
	while(FindNextFile(hd,&fd));

	FindClose(hd);
	pIoBuffer->AddData("</ul>\r\n</body></html>");
	
	char pResponseHeader[1024];
	sprintf(pResponseHeader, "HTTP/1.1 200 OK\r\nAccept-Ranges: "
		"bytes\r\nContent-Length: %d\r\nConnection: %s\r\nContent-Type: text/html\r\n\r\n",
		pIoBuffer->GetLen(), bKeepAlive?"Keep-Alive":"close");

	pIoBuffer->InsertData(0, pResponseHeader, strlen(pResponseHeader));
	pIoBuffer->m_bKeepAlive = bKeepAlive;

	AsyncSend(pIoBuffer);
}

void CHttpIoContext::HttpSendError(const string & strState, const string & strErrMsg, bool bKeepAlive, const char * szTitle)
{
	CIoBuffer * pIoBuffer = new CIoBuffer();
	string str;
	char pResponseHeader[1024];
	
	str = (szTitle==NULL?strState:szTitle);

	pIoBuffer->AddData("<html><head>\r\n<title>");
	pIoBuffer->AddData(str);
	pIoBuffer->AddData("</title>\r\n</head><body>\r\n<h1>");
	pIoBuffer->AddData(strState.c_str()+4);
	pIoBuffer->AddData("</h1>\r\n<p>");
	pIoBuffer->AddData(strErrMsg);
	pIoBuffer->AddData("\r\n</p>\r\n</body></html>");
	
	sprintf(pResponseHeader, "HTTP/1.1 %s\r\nAccept-Ranges: "
		"bytes\r\nContent-Length: %d\r\nConnection: %s\r\nContent-Type: text/html\r\n\r\n",
		strState.c_str(), 
		pIoBuffer->GetLen(), bKeepAlive?"Keep-Alive":"close");

	pIoBuffer->InsertData(0, pResponseHeader, strlen(pResponseHeader));
	pIoBuffer->m_bKeepAlive = bKeepAlive;

	AsyncSend(pIoBuffer);
}

bool CHttpIoContext::HttpEncodeURL(string & strUrlDecode)
{
	const char * inSrc = strUrlDecode.c_str();
	int inSrcLen=strUrlDecode.size();
	char buf[MAX_HTTP_REQUEST] = {0};

	char* ioDest = buf;
	int inDestLen = MAX_HTTP_REQUEST;
    
    int theLengthWritten = 0;
    
    while (inSrcLen > 0)
    {
        if (theLengthWritten == inDestLen)
            return false;
            
        if ((unsigned char)*inSrc > 127)
        {
            if (inDestLen - theLengthWritten < 3)
                return false;
                
            sprintf(ioDest,"%%%X",(unsigned char)*inSrc);
            ioDest += 3;
            theLengthWritten += 3;
                        inSrc++;
                        inSrcLen--;
            continue;
        }
       
        switch (*inSrc)
        {
            case (' '):
            case ('\r'):
            case ('\n'):
            case ('\t'):
            case ('<'):
            case ('>'):
            case ('#'):
            case ('%'):
            case ('{'):
            case ('}'):
            case ('|'):
            case ('\\'):
            case ('^'):
            case ('~'):
            case ('['):
            case (']'):
            case ('`'):
            case (';'):
            case ('?'):
            case ('@'):
            case ('='):
            case ('&'):
            case ('$'):
            case ('"'):
            {
                if ((inDestLen - theLengthWritten) < 3)
                    return false;
                    
                sprintf(ioDest,"%%%X",(int)*inSrc);
                ioDest += 3;
                theLengthWritten += 3;
                break;
            }
            default:
            {
                *ioDest = *inSrc;
                ioDest++;
                theLengthWritten++;
            }
        }
        
        inSrc++;
        inSrcLen--;
    }
    
	strUrlDecode = buf;

    return true;
}

//======================================================================================
CHttpRequest::CHttpRequest()
{
	bKeepAlive = false;
	dwRangeEnd = 0;
	dwRangeStart = 0;
	szType = "";
	szUrl = "";
}

bool CHttpRequest::HttpParseRequest(const string & szRequest)
{
	if(szRequest.size()>MAX_HTTP_REQUEST)
		return false;

	int i,j,k;

	i = szRequest.rfind("live");
	bKeepAlive = (i!=string::npos);

	i = szRequest.find(' ');
	if(i==string::npos)
		return false;

	szType = szRequest.substr(0, i);

	j = szRequest.find(' ', ++i);
	if(i==string::npos)
		return false;

	szUrl = szRequest.substr(i, j-i);

	HttpDecodeURL(szUrl);

	// /?aa=bb&cc=dd  ==> aa=bb ==> 
	vector<string> szParamArr;
	k = szUrl.find('?');
	szFile = szUrl.substr(0, k);

	if(k!=szUrl.npos)
	{	
		CZvLib::SplitString(szUrl.c_str()+k+1, "&", szParamArr);
		for(i=0; i<szParamArr.size(); i++)
		{
			j = szParamArr[i].find('=');
			if(j!=szParamArr[i].npos && j>0)
			{
				mapParam[szParamArr[i].substr(0,j)] = szParamArr[i].substr(j+1,szParamArr[i].npos);
			}
		}
	}

	// \r\n\r\nRange: bytes=aaa-bbb\r\n
	i = szRequest.rfind("Range: bytes=");
	if(i!=string::npos)
	{
		i+=15;
		j = szRequest.find("\r\n", i);
		if(j!=string::npos)
		{
			string str;
			k = szRequest.find('-', i);
			if(k>=i && k<=j)
			{
				str = szRequest.substr(i, k-i);
				if(str.size()>0)
					dwRangeStart = atol(str.c_str());
				str = szRequest.substr(k+1, j-k-1);
				if(str.size()>0)
					dwRangeEnd = atol(str.c_str());
			}
		}
	}

	// cookies
	vector<string> szCookie;
	k = szRequest.rfind("Cookie:");

	if(k!=szUrl.npos)
	{	
		CZvLib::SplitString(szRequest.c_str()+k+8, "; ", szCookie, "\r\n");

		for(i=0; i<szCookie.size(); i++)
		{
			j = szCookie[i].find('=');
			if(j!=szCookie[i].npos && j>0)
			{
				mapCookie[szCookie[i].substr(0,j)] = szCookie[i].substr(j+1,szCookie[i].npos);
			}
		}
	}

	// Post
	if(szType=="POST")
	{
		vector<string> szPost;
		k = szRequest.rfind("\r\n\r\n");

		if(k!=szRequest.npos)
		{	
			string strDecoded = szRequest.c_str()+k+4;
	
			if(!HttpDecodeURL(strDecoded))
				return false;

			CZvLib::SplitString(strDecoded.c_str(), "&", szPost);

			for(i=0; i<szPost.size(); i++)
			{
				j = szPost[i].find('=');
				if(j!=szPost[i].npos && j>0)
				{
					string str1 = szPost[i].substr(0,j);
					string str2 = szPost[i].substr(j+1,szPost[i].npos);

					mapPost[str1] = str2;
				}
			}
		}
	}
	
	return true;
}

bool CHttpRequest::HttpDecodeURL(string & strUrlEncode)
{
	const char* inSrc = strUrlEncode.c_str();
	int inSrcLen = strUrlEncode.size();

	char buf[MAX_HTTP_REQUEST]={0};
	char* ioDest = buf;

    if ( inSrcLen <= 0)//  || *inSrc != '/' )
        return false;
   
    int tempChar = 0;
    int numDotChars = 0;
    bool inQuery = false; 
    
    while (inSrcLen > 0)
    {
        if (*inSrc == '?')
            inQuery = true;

        if (*inSrc == '%')
        {
            if (inSrcLen < 3)
                return false;

            char tempbuff[3];
            inSrc++;
            if (!isxdigit(*inSrc))
                return false;
            tempbuff[0] = *inSrc;
            inSrc++;
            if (!isxdigit(*inSrc))
                return false;
            tempbuff[1] = *inSrc;
            inSrc++;
            tempbuff[2] = '\0';
            sscanf(tempbuff, "%x", &tempChar);
            inSrcLen -= 3;      
        }
        else if (*inSrc == '\0')
        {
            return false;
        }
        else
        {
            tempChar = *inSrc;
            inSrcLen--;
            inSrc++;
        }
        
        if (!inQuery)
        {
            if ((tempChar == '\\'))
                return false;
        
            if ((tempChar == '/') && (numDotChars <= 2) && (numDotChars > 0))
            {
                ioDest -= (numDotChars + 1);
            }

            *ioDest = (char)tempChar&0xFF;

            if (*ioDest == '.')
            {
                if ((numDotChars == 0) && (*(ioDest - 1) == '/'))
                    numDotChars++;
                else if ((numDotChars > 0) && (*(ioDest - 1) == '.'))
                    numDotChars++;
            }
            else
                numDotChars = 0;
        }
        else
        {
            *ioDest = (char)tempChar&0xFF;
        }

        ioDest++;
    }

    //对UTF-8编码进行必要的转换
    WCHAR *strSrc = NULL;    
    CHAR *szRes = NULL;    
    int i = MultiByteToWideChar(CP_UTF8, 0, buf, -1, NULL, 0);    
    strSrc = (WCHAR*)GRS_CALLOC((i + 1) * sizeof(WCHAR));  
    //转为UNICODE先
    MultiByteToWideChar(CP_UTF8, 0, buf, -1, strSrc, i);    
    i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);    
    szRes = (CHAR*)GRS_CALLOC(sizeof(CHAR)*(i+1));    
    //再转换为线程默认编码一般是GB2312或MBCS等
    WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);   
    
    strUrlEncode = szRes;
    GRS_SAFEFREE(strSrc);    
    GRS_SAFEFREE(szRes);    

    return true;
}

string CHttpRequest::GetCookie(const char * szName)
{
	return mapCookie[szName];
}

string CHttpRequest::GetPost(const char * szName)
{
	return mapPost[szName];
}