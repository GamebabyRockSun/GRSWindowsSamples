#include <tchar.h>
#define WIN32_LEAN_AND_MEAN	
#include <windows.h>
#include <strsafe.h>
#include <Winsock2.h>
#include "resource.h"
#pragma comment( lib, "Ws2_32.lib" )

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_CREALLOC(p,sz)	HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_PRINTFA(...) \
    StringCchPrintfA(pOutBufA,1024,__VA_ARGS__);\
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE),pOutBufA,lstrlenA(pOutBufA),NULL,NULL);

#define SOCK_VERH 2
#define SOCK_VERL 2

#define GRS_SERVER_PORT 8080

#define GRS_GETINSTANCE()    (HINSTANCE)GetModuleHandle(NULL)

#define MAX_LOADSTRING 100

#define MYWM_SOCK_MSGSTR _T("MYWM_SOCK_MSG")

// 全局变量:
UINT    g_nNetMsgID                     = 0;    //网络消息ID
HWND    g_hWnd                          = NULL; // 接收SOCKET消息的窗口句柄
TCHAR   g_szTitle[MAX_LOADSTRING]       = _T("Winsock WSAAsyncSelect Wnd");					// 标题栏文本
TCHAR   g_szWindowClass[MAX_LOADSTRING] = _T("Winsock WSAAsyncSelect Wnd Class");			// 主窗口类名

ATOM                GRSRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
BOOL				InitInstance(HINSTANCE, int);
VOID                MsgLoop();

int _tmain()
{
    GRS_USEPRINTF();
    WORD wVer = MAKEWORD(SOCK_VERH,SOCK_VERL);
    WSADATA wd;
    int err = ::WSAStartup(wVer,&wd);
    if(0 != err)
    {
        GRS_PRINTF(_T("无法初始化Socket2系统环境，错误码为：%d！\n"),WSAGetLastError());
        return 1;
    }
    if ( LOBYTE( wd.wVersion ) != SOCK_VERH ||
        HIBYTE( wd.wVersion ) != SOCK_VERL ) 
    {
        GRS_PRINTF(_T("无法初始化%d.%d版本的Socket环境！\n"),SOCK_VERH,SOCK_VERL);
        WSACleanup( );
        return 2; 
    }
    GRS_PRINTF(_T("Winsock库初始化成功!\n\t当前系统中支持最高的Winsock版本为%d.%d\n\t当前应用加载的版本为%d.%d\n")
        ,LOBYTE(wd.wHighVersion),HIBYTE(wd.wHighVersion)
        ,LOBYTE(wd.wVersion),HIBYTE(wd.wVersion));
    //==========================================================================================================
    //在这里添加Winsock的调用

    //动态注册消息ID
    g_nNetMsgID = RegisterWindowMessage(MYWM_SOCK_MSGSTR);

    //注册网络消息窗口类
    GRSRegisterClass(GRS_GETINSTANCE());
    //创建窗口
    InitInstance(GRS_GETINSTANCE(),SW_SHOWNORMAL);

    SOCKET skServer         = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    SOCKADDR_IN sa          = {AF_INET};
    sa.sin_addr.s_addr      = htonl(INADDR_ANY);
    sa.sin_port             = htons(GRS_SERVER_PORT);

    if(0 != bind(skServer,(SOCKADDR*)&sa,sizeof(SOCKADDR_IN)))
    {
        GRS_PRINTF(_T("绑定到指定地址端口出错!\n"));
        goto CLEAN_UP;
    }

    //设置响应的消息以及响应消息的窗口句柄
    if( 0 != WSAAsyncSelect(skServer,g_hWnd,g_nNetMsgID,FD_ACCEPT|FD_CLOSE) )
    {
        GRS_PRINTF(_T("在监听SOCKET上设置网络消息失败,错误码:0x%08X.\n"),WSAGetLastError());
        goto CLEAN_UP;
    }

    //这之后skServer已经变成非阻塞模式工作了

    if(0 != listen(skServer,SOMAXCONN))
    {
        GRS_PRINTF(_T("SOCKET进入监听模式出错!\n"));
        goto CLEAN_UP;
    }

    //消息循环
    MsgLoop();

    //==========================================================================================================
CLEAN_UP:
    _tsystem(_T("PAUSE"));
    if(INVALID_SOCKET != skServer)
    {
        closesocket(skServer);
    }    
    ::WSACleanup();
    return 0;
}


ATOM GRSRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= WndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= GRS_GETINSTANCE();
    wcex.hIcon			= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_WNDSAMPLE));
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_WNDSAMPLE);
    wcex.lpszClassName	= g_szWindowClass;
    wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    GRS_USEPRINTF();
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    //-------------------------------------------------------------------------------
    int iError = 0;
    long lEvent = 0;
    SOCKET MsgSocket = INVALID_SOCKET;
    if( message == g_nNetMsgID )
    {
        iError = WSAGETSELECTERROR(lParam);
        lEvent = WSAGETSELECTEVENT(lParam);
        MsgSocket = (SOCKET)wParam;

        if(0 != iError)
        {
            GRS_PRINTF(_T("在SOCKET[0x%08X]上的操作[%d]发生一个错误:0x%08X\n")
                ,MsgSocket,lEvent,iError);
            return 0;
        }

        switch(lEvent)
        {
        case FD_ACCEPT:
            {
                sockaddr_in ClientAddress = {};
                int nClientLength = sizeof(ClientAddress);
                SOCKET skClient = accept(MsgSocket,(SOCKADDR*)&ClientAddress, &nClientLength);

                if (INVALID_SOCKET == skClient)
                {
                    GRS_PRINTF(_T("\naccept调用失败,错误码: 0x%08X."), WSAGetLastError());
                    return 0;
                }

                GRS_PRINTFA("\n客户端连接进来: [%s : %u]"
                    , inet_ntoa(ClientAddress.sin_addr),ntohs(ClientAddress.sin_port)); 

                //设置消息模式
                if( 0 != WSAAsyncSelect(skClient,hWnd,g_nNetMsgID,FD_READ | FD_WRITE | FD_CLOSE) )
                {
                    GRS_PRINTF(_T("在客户端SOCKET[0x%08X]上设置网络消息失败,错误码:0x%08X.\n")
                        ,skClient,WSAGetLastError());
                    closesocket(skClient);
                    return 0;
                }
            }
            break;
        case FD_READ:
            {       
                int iBufLen = 1024;
                int iBufLenTmp = 0;
                int iRecv = 0;
                VOID* pskBuf = GRS_CALLOC(iBufLen);
                VOID* pskBufTmp = pskBuf;

                do 
                {
                    iRecv = recv(MsgSocket,(char*)pskBufTmp,iBufLen - iBufLenTmp,0);
                    if( SOCKET_ERROR == iRecv || 0 == iRecv )
                    {
                        if(WSAEWOULDBLOCK == WSAGetLastError())
                        {
                            Sleep(20);//停20ms
                            continue;
                        }

                        break;
                    }

                    if( iRecv >= (iBufLen - iBufLenTmp) )
                    {//其实只可能出现==的情况
                        iBufLenTmp = iBufLen;
                        iBufLen += 1024;
                        pskBuf = GRS_CREALLOC(pskBuf,iBufLen);
                        pskBufTmp = (void*)((BYTE*)pskBuf + iBufLenTmp);

                        continue;
                    }
                    else
                    {
                        break;
                    }
                } 
                while( 1 );

                if( SOCKET_ERROR != iRecv && 0 != iRecv )
                {
                    do 
                    {//直接将数据发出去,不用等FD_WRITE
                        send( MsgSocket,(const char*)pskBuf,iBufLen,0 );
                        Sleep(20);//停20ms
                    } while (WSAEWOULDBLOCK == WSAGetLastError());
                }
            }
            break;
        case FD_WRITE:
            {//这里就不处理了

            }
            break;
        case FD_CLOSE:
            {
                closesocket(MsgSocket);
            }
            break;
        }
    }
    //--------------------------------------------------------------------------------

    switch (message)
    {
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // 分析菜单选择:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(GRS_GETINSTANCE(), MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: 在此添加任意绘图代码...
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    g_hWnd = CreateWindow(g_szWindowClass, g_szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!g_hWnd)
    {
        return FALSE;
    }

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    return TRUE;
}

VOID  MsgLoop()
{
    // TODO: 在此放置代码。
    MSG msg = {};
    HACCEL hAccelTable = LoadAccelerators(GRS_GETINSTANCE(), MAKEINTRESOURCE(IDC_WNDSAMPLE));

    // 主消息循环:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}