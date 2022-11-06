// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.
//
// Module Name: asyncselect.cpp
//
// Description:
//    This file contains the routines necessary to implement the WSPAsyncSelect
//    API. In order for our LSP to count the bytes sent and received by an 
//    application when the app uses WSAAsyncSelect we need to intercept all
//    of the app's send and receive calls. To do this we implement a hidden
//    window. When the app makes a send/recv call we intercept this and post
//    the operation with the provider's socket to our own window. This allows
//    the LSP to receive the completion notification. At this point we will
//    update the statistics and then post the completion to the app's window
//    so that it may continue as expected.
//
//    This file contains the I/O manager for all WSAAsyncselect I/O operations
//
#include "provider.h"

#include <windows.h>

#define PROVIDER_CLASS TEXT("Layered WS2 Provider")

//
// External globals defined in spi.cpp
//
extern HINSTANCE        hDllInstance;
extern CRITICAL_SECTION gCriticalSection;

//
// Function Prototypes
//
static DWORD WINAPI AsyncMsgHandler(LPVOID lpParameter);
static LRESULT CALLBACK AsyncWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//
// Globals
//
static HANDLE   WorkerThreadHandle = NULL;
static HWND     AsyncWindow = NULL;

static TCHAR    Msg[512];                         // For outputting debug messages

//
// Function: StopAsyncWindowManager
//
// Description:
//    This function cleans up the subsystem that handles asynchronous
//    window IO (e.g. WSAAsyncSelect). Basically, it destroys the the
//    hidden window if its been created. Note that this function is
//    only called from WSPCleanup when the DLL is about to be unloaded,
//    and we've already entered the critical section (gCriticalSection).
//
int StopAsyncWindowManager()
{
    if (AsyncWindow)
    {
        DestroyWindow(AsyncWindow);
        AsyncWindow = NULL;

        CloseHandle(WorkerThreadHandle);
    }
    return 0;
}

//
// Function: GetWorkerWindow
//
// Description:
//    This returns a handle to our hidden window that acts as an 
//    intermediary between the apps window and winsock. If the window
//    hasn't already been created, then create it.
//
HWND GetWorkerWindow()
{
	DWORD ThreadId;

    EnterCriticalSection(&gCriticalSection);
	if (WorkerThreadHandle == NULL) 
	{
		WorkerThreadHandle = CreateThread(NULL, 0, AsyncMsgHandler, NULL, 0, &ThreadId); 
        if (WorkerThreadHandle)
        {
            // Wait for the window to become initialized
            while (AsyncWindow == NULL)
                SleepEx(0, TRUE);
        }
        else
        {
            dbgprint("GetWorkerWindow: CreateThread failed");
            AsyncWindow = NULL;
        }
	}
    LeaveCriticalSection(&gCriticalSection);

	return AsyncWindow;
}

//
// Function: AsyncMsgHandler
//
// Description:
//    This is the message pump for our hidden window.
//
static DWORD WINAPI AsyncMsgHandler(LPVOID lpParameter)
{
	MSG      msg;
	DWORD    Ret;
    WNDCLASS wndclass;

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (WNDPROC)AsyncWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hDllInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = PROVIDER_CLASS;

    if(RegisterClass(&wndclass) == 0)
    {
        dbgprint("AsyncMsgHandle: RegisterClass failed: %d", GetLastError());
        return NULL;
    }

    // Create a window.

    AsyncWindow = CreateWindow(
        PROVIDER_CLASS,
        TEXT("Layered Hidden Window"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hDllInstance,
        NULL
    );

    if (AsyncWindow == NULL)
    {
        dbgprint("AsyncMessageHandler: CreateWindow failed");
        return NULL;
    }
	while((Ret = GetMessage (&msg, NULL, 0, 0)))
	{
		if (Ret == -1)
		{
            dbgprint("AsyncMessageHandler: GetMessage returned -1, exiting loop");
			return 0;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

//
// Function: AsyncWndProc
//
// Description:
//    This is the window proc for our hidden window. Once we receive a message
//    on our hidden window we must translate our lower provider's socket handle
//    to the app's socket handle and then complete the notification to the user.
//
static LRESULT CALLBACK AsyncWndProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	SOCK_INFO *si;

	if (uMsg == WM_SOCKET)
	{
		if (si = GetCallerSocket(NULL, wParam))
		{
			MainUpCallTable.lpWPUPostMessage(
                si->hWnd, 
                si->uMsg, 
                si->LayeredSocket, 
                lParam);
			return 0;
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
