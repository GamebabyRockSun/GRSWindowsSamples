#pragma once

#include "IoContext.h"

class CHttpRequest;

class CHttpIoContext : public CIoContext  
{
public:
	CHttpIoContext(SOCKET sock);
	virtual ~CHttpIoContext();

	virtual void ProcessJob(CIoBuffer * pIoBuffer);

	//
	void HttpProcessRequest(const string & strRequest);
	void HttpSendError(const string & strState, const string & strErrMsg, bool bKeepAlive=false, const char * strTitle = NULL);
	void HttpSendDir(string & strDirPath, bool bKeepAlive=false);
	bool HttpEncodeURL(string & strUrlDecode);
	void HttpSendFile(string & szFile, DWORD dwRangeStart, DWORD dwRangeEnd, bool bKeepAlive, bool bDownload=false);
};

class CHttpRequest
{
public:

	CHttpRequest();

	bool HttpDecodeURL(string & strUrlEncode);
	bool HttpParseRequest(const string & szRequest);
	
	string GetCookie(const char * szName);
	string GetPost(const char * szName);

	string	szType;
	string	szUrl;
	string  szFile;
	DWORD	dwRangeStart;
	DWORD	dwRangeEnd;
	bool	bKeepAlive;
	map<string, string> mapParam;
	map<string, string> mapCookie;
	map<string, string> mapPost;
};