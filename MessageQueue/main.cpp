#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

DWORD WINAPI ThreadFunction( LPVOID lpParam ) 
{//�̺߳���
	GRS_USEPRINTF();
	MSG msg = {};
	//��佫ǿ��ϵͳ����һ����Ϣ���У�ע�ͺ���ܿ������߳�û���յ��κ���Ϣ
	PeekMessage(&msg,NULL,0,0,PM_NOREMOVE);

	//ģ��һ����ʱ�ĳ�ʼ������
	for(int i = 0; i < 10000000; i++);

	//һ���򻯵���Ϣѭ��
	while(GetMessage(&msg,NULL,0,0))
	{
		GRS_PRINTF(_T("�߳�[ID:0x%x]�յ���Ϣ-0x%04x \tʱ��(GetTickCountֵ)-%u\n")
			,GetCurrentThreadId(),msg.message,msg.time);	
	}
	//ִ�е������ʾ�յ�����WM_QUIT��Ϣ
	GRS_PRINTF(_T("�߳�[ID:0x%x]�յ��˳���Ϣ-0x%04x \tʱ��(GetTickCountֵ)-%u\n")
		,GetCurrentThreadId(),msg.message,msg.time);	

	return msg.wParam; 
} 

int _tmain()
{
	DWORD  dwThreadID = 0;
	HANDLE hThread = CreateThread(NULL,0,ThreadFunction,NULL,0,&dwThreadID);
	//Sleep(100);
	//��������Ϣ�����ղ�������Ϊ���߳�Ĭ��û����Ϣ����
	PostThreadMessage(dwThreadID,0x0001,0,0);
	PostThreadMessage(dwThreadID,0x0002,0,0);
	//ǿ���л������߳�ȥִ�У�ע�ͺ���п��ܿ��������̶߳��ڵȴ�������
	Sleep(1);
	
	PostThreadMessage(dwThreadID,0x0003,0,0);
	PostThreadMessage(dwThreadID,0x0004,0,0);

	//�����̷߳����˳���Ϣ
	PostThreadMessage(dwThreadID,WM_QUIT,(WPARAM)GetCurrentThreadId(),0);

	//�ȴ����߳��˳�
	WaitForSingleObject(hThread,INFINITE);

	CloseHandle(hThread);
	_tsystem(_T("PAUSE"));
	return 0;
}