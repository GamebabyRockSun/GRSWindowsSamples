#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

VOID CALLBACK APCFunction(ULONG_PTR dwParam)
{
	int i = dwParam;
	GRS_USEPRINTF();
	GRS_PRINTF(_T("%d APC Function Run!\n"),i);
	Sleep(20);
}

int _tmain()
{
	//Ϊ���߳����APC����
	for(int i = 0;i < 100;i++)
	{
		QueueUserAPC(APCFunction,GetCurrentThread(),i);
	}
	//ͨ��SleepEx ���롰�ɾ���״̬��,ע�ͺ�APC�������ᱻ����
	SleepEx(10000,TRUE);

	_tsystem(_T("PAUSE"));
	return 0;
}