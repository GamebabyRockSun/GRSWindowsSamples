#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

DWORD WINAPI ThreadFunction( LPVOID lpParam ) 
{//线程函数
	GRS_USEPRINTF();
	MSG msg = {};
	//这句将强制系统创建一个消息队列，注释后可能看到该线程没有收到任何消息
	PeekMessage(&msg,NULL,0,0,PM_NOREMOVE);

	//模拟一个耗时的初始化过程
	for(int i = 0; i < 10000000; i++);

	//一个简化的消息循环
	while(GetMessage(&msg,NULL,0,0))
	{
		GRS_PRINTF(_T("线程[ID:0x%x]收到消息-0x%04x \t时间(GetTickCount值)-%u\n")
			,GetCurrentThreadId(),msg.message,msg.time);	
	}
	//执行到这里表示收到的是WM_QUIT消息
	GRS_PRINTF(_T("线程[ID:0x%x]收到退出消息-0x%04x \t时间(GetTickCount值)-%u\n")
		,GetCurrentThreadId(),msg.message,msg.time);	

	return msg.wParam; 
} 

int _tmain()
{
	DWORD  dwThreadID = 0;
	HANDLE hThread = CreateThread(NULL,0,ThreadFunction,NULL,0,&dwThreadID);
	//Sleep(100);
	//这两个消息可能收不到，因为新线程默认没有消息队列
	PostThreadMessage(dwThreadID,0x0001,0,0);
	PostThreadMessage(dwThreadID,0x0002,0,0);
	//强制切换到新线程去执行，注释后就有可能看到两个线程都在等待而死锁
	Sleep(1);
	
	PostThreadMessage(dwThreadID,0x0003,0,0);
	PostThreadMessage(dwThreadID,0x0004,0,0);

	//向新线程发送退出消息
	PostThreadMessage(dwThreadID,WM_QUIT,(WPARAM)GetCurrentThreadId(),0);

	//等待新线程退出
	WaitForSingleObject(hThread,INFINITE);

	CloseHandle(hThread);
	_tsystem(_T("PAUSE"));
	return 0;
}