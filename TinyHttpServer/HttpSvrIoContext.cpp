#include "StdAfx.h"
#include "HttpSvrIoContext.h"
#include "HttpIoContext.h"
#include "ZvLib.h"

CHttpSvrIoContext * m_pHttpSvr;

CHttpSvrIoContext::CHttpSvrIoContext()
{
	m_pHttpSvr = this;
	m_szRootPath = CZvLib::GetAppPath();
    m_szRootPath.resize(m_szRootPath.length() - 1);
    m_szRootPath.resize(m_szRootPath.rfind('\\') + 1);
    m_szRootPath.append("Web Dir\\");

	InitMimeTypes();
}

CHttpSvrIoContext::~CHttpSvrIoContext()
{
	
}

void CHttpSvrIoContext::InitMimeTypes()
{
	MimeTypes["doc"]	= "application/msword";
	MimeTypes["bin"]	= "application/octet-stream";
	MimeTypes["dll"]	= "application/octet-stream";
	MimeTypes["exe"]	= "application/octet-stream";
	MimeTypes["pdf"]	= "application/pdf";
	MimeTypes["p7c"]	= "application/pkcs7-mime";
	MimeTypes["ai"]		= "application/postscript";
	MimeTypes["eps"]	= "application/postscript";
	MimeTypes["ps"]		= "application/postscript";
	MimeTypes["rtf"]	= "application/rtf";
	MimeTypes["fdf"]	= "application/vnd.fdf";
	MimeTypes["arj"]	= "application/x-arj";
	MimeTypes["gz"]		= "application/x-gzip";
	MimeTypes["class"]	= "application/x-java-class";
	MimeTypes["js"]		= "application/x-javascript";
	MimeTypes["lzh"]	= "application/x-lzh";
	MimeTypes["lnk"]	= "application/x-ms-shortcut";
	MimeTypes["tar"]	= "application/x-tar";
	MimeTypes["hlp"]	= "application/x-winhelp";
	MimeTypes["cert"]	= "application/x-x509-ca-cert";
	MimeTypes["zip"]	= "application/zip";
	MimeTypes["cab"]	= "application/x-compressed";
	MimeTypes["arj"]	= "application/x-compressed";
	MimeTypes["aif"]	= "audio/aiff";
	MimeTypes["aifc"]	= "audio/aiff";
	MimeTypes["aiff"]	= "audio/aiff";
	MimeTypes["au"]		= "audio/basic";
	MimeTypes["snd"]	= "audio/basic";
	MimeTypes["mid"]	= "audio/midi";
	MimeTypes["rmi"]	= "audio/midi";
	MimeTypes["mp3"]	= "audio/mpeg";
	MimeTypes["vox"]	= "audio/voxware";
	MimeTypes["wav"]	= "audio/x-wav";
	MimeTypes["ra"]		= "audio/x-pn-realaudio";
	MimeTypes["ram"]	= "audio/x-pn-realaudio";
	MimeTypes["bmp"]	= "image/bmp";
	MimeTypes["gif"]	= "image/gif";
	MimeTypes["jpeg"]	= "image/jpeg";
	MimeTypes["jpg"]	= "image/jpeg";
	MimeTypes["png"]	= "image/png";
	MimeTypes["tif"]	= "image/tiff";
	MimeTypes["tiff"]	= "image/tiff";
	MimeTypes["xbm"]	= "image/xbm";
	MimeTypes["wrl"]	= "model/vrml";
	MimeTypes["htm"]	= "text/html";
	MimeTypes["html"]	= "text/html";
	MimeTypes["c"]		= "text/plain";
	MimeTypes["cpp"]	= "text/plain";
	MimeTypes["def"]	= "text/plain";
	MimeTypes["h"]		= "text/plain";
	MimeTypes["txt"]	= "text/plain";
	MimeTypes["ini"]	= "text/plain";
	MimeTypes["inf"]	= "text/plain";
	MimeTypes["rtx"]	= "text/richtext";
	MimeTypes["rtf"]	= "text/richtext";
	MimeTypes["java"]	= "text/x-java-source";
	MimeTypes["css"]	= "text/css";
	MimeTypes["mpeg"]	= "video/mpeg";
	MimeTypes["mpg"]	= "video/mpeg";
	MimeTypes["mpe"]	= "video/mpeg";
	MimeTypes["avi"]	= "video/msvideo";
	MimeTypes["mov"]	= "video/quicktime";
	MimeTypes["qt"]		= "video/quicktime";
	MimeTypes["shtml"]	= "wwwserver/html-ssi";
	MimeTypes["asa"]	= "wwwserver/isapi";
	MimeTypes["asp"]	= "wwwserver/isapi";
	MimeTypes["cfm"]	= "wwwserver/isapi";
	MimeTypes["dbm"]	= "wwwserver/isapi";
	MimeTypes["isa"]	= "wwwserver/isapi";
	MimeTypes["plx"]	= "wwwserver/isapi";
	MimeTypes["url"]	= "wwwserver/isapi";
	MimeTypes["cgi"]	= "wwwserver/isapi";
	MimeTypes["php"]	= "wwwserver/isapi";
	MimeTypes["wcgi"]	= "wwwserver/isapi";
}

void CHttpSvrIoContext::OnAccepted(CIoBuffer * pIoBuffer)
{
	CHttpIoContext * pIoContext = new CHttpIoContext(*reinterpret_cast<SOCKET*>(pIoBuffer->GetBuffer()));
	if(pIoBuffer==NULL)
	{
		closesocket(*reinterpret_cast<SOCKET*>(pIoBuffer->GetBuffer()));
		delete pIoBuffer;
		return;
	}

	setsockopt(pIoContext->m_sIo, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,  
		( char* )&(m_sIo), sizeof( m_sIo ) );

	SOCKADDR* remote;
	SOCKADDR* local;
	int remote_len = sizeof(SOCKADDR_IN); 
	int local_len = sizeof(SOCKADDR_IN);
	DWORD dwRet=0;
	GetAcceptExSockaddrs(pIoBuffer->GetBuffer()+sizeof(SOCKET),
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&local,
		&local_len,
		&remote,
		&remote_len);
	
	memcpy(&pIoContext->m_addrRemote, remote, sizeof(SOCKADDR_IN));

	if(!(pIoContext->AssociateWithIocp() && pIoContext->AsyncRecv()))
	{
		pIoContext->OnClosed();
	}

	delete pIoBuffer;
}

void CHttpSvrIoContext::OnClosed()
{

}

void CHttpSvrIoContext::SetRootPath(const char * szPath)
{
	m_szRootPath = szPath;
}