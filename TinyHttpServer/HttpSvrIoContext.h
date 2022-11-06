#pragma once

#include "IoContext.h"

class CHttpSvrIoContext : public CIoContext  
{
public:
	CHttpSvrIoContext();
	virtual ~CHttpSvrIoContext();

	void OnAccepted(CIoBuffer * pIoBuffer);
	void OnClosed();
	void InitMimeTypes();

	void SetRootPath(const char * szPath);

	string m_szRootPath;
	MIMETYPES MimeTypes;
};