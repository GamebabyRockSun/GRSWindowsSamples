
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
		printf("������ɶ˿�ʧ�ܡ�");
		return;
	}

	CHttpSvrIoContext HttpSvr;

	if(HttpSvr.StartSvr(80))
	{
		string str = "\r\n   http ������������...\r\n\r\n   ��Ŀ¼��"+ HttpSvr.m_szRootPath + "\r\n\r\n";
		printf(str.c_str());
	}
	else
	{
		printf("\r\n    ��80�˿�����������ʧ�ܣ�������web�������������У�\r\n ");
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