
/*
	zhengv@gmail.com
*/

#include "IOCP.h"
#include "HttpSvrIoContext.h"
#include <conio.h>

BOOL WINAPI HandlerRoutine(DWORD);

void main()
{
	SetConsoleTitle(HTTP_SERVER_NAME);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine, TRUE);

	CIOCP Iocp;
	
	if(!Iocp.Start())
	{
		printf("创建完成端口失败。");
		return;
	}

	CHttpSvrIoContext HttpSvr;

	if(HttpSvr.StartSvr(80))
	{
		string str = "\r\n   http 服务器运行中...\r\n\r\n   主目录："+ HttpSvr.m_szRootPath + "\r\n\r\n";
		printf(str.c_str());
	}
	else
	{
		printf("\r\n    在80端口启动服务器失败，有其它web服务器正在运行？\r\n ");
	}

	Sleep(INFINITE);
}

BOOL WINAPI HandlerRoutine(DWORD Event)
{
    switch(Event)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
		exit(0);
    }
    return TRUE;	
}