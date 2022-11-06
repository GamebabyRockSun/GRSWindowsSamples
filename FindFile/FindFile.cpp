#include <tchar.h> 
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

void ErrorHandler(LPTSTR lpszFunction);

VOID ShowFile(LPCTSTR pszDir)
{
	GRS_USEPRINTF();
	TCHAR pTmpDir[MAX_PATH] = {};
	StringCchCopy(pTmpDir,MAX_PATH,pszDir);

	size_t szDirLen = 0;
	StringCchLength(pTmpDir,MAX_PATH,&szDirLen);
	if(_T('\\') != pTmpDir[szDirLen])
	{
		StringCchCat(pTmpDir,MAX_PATH,_T("\\"));
	}
	StringCchCat(pTmpDir,MAX_PATH,_T("*"));

	LARGE_INTEGER filesize = {};
	WIN32_FIND_DATA ffd = {};
	HANDLE hFind = FindFirstFile(pTmpDir, &ffd);


	if (INVALID_HANDLE_VALUE == hFind) 
	{
		ErrorHandler(_T("FindFirstFile"));
		return;
	} 

	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			GRS_PRINTF(_T("%30s   <DIR>\n"), ffd.cFileName);
/*			if(_T('.') != ffd.cFileName[0])
			{
				ShowFile(ffd.cFileName);
			}			*/	
		}
		else
		{
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			GRS_PRINTF(_T("%30s   %lu bytes\n"), ffd.cFileName, filesize.QuadPart);
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);
	if ( GetLastError() != ERROR_NO_MORE_FILES ) 
	{
		ErrorHandler(_T("FindFirstFile"));
	}
	FindClose(hFind);
}

int _tmain(int argc, TCHAR *argv[])
{
	ShowFile(_T("C:"));
	_tsystem(_T("PAUSE"));
	return 0;
}


void ErrorHandler(LPTSTR lpszFunction) 
{ 
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		_T("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf); 
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, _T("Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

