#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

static DWORD g_dwTLS = 0;
static UINT	 g_nBufSize = 256;

BOOL APIENTRY  DllMain(HMODULE hModule, DWORD dwReason,LPVOID lpReserved)
{
	GRS_USEPRINTF();
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		{
			g_dwTLS = TlsAlloc();
			if(TLS_OUT_OF_INDEXES == g_dwTLS)
			{
				GRS_PRINTF(_T("为进程[ID:0x%x]分配TLS索引失败!\n"),GetCurrentProcessId());
				return FALSE;
			}
			GRS_PRINTF(_T("为进程[ID:0x%x]分配TLS索引:%u\n"),GetCurrentProcessId(),g_dwTLS);
		}
		break;
	case DLL_THREAD_ATTACH:
		{
			VOID* pThdData = GRS_CALLOC(g_nBufSize);
			
			if(!TlsSetValue(g_dwTLS,pThdData))
			{
				GRS_PRINTF(_T("为线程[ID:0x%x]设置TLS索引[%u]变量[0x%08X]失败!\n")
					,GetCurrentThreadId(),g_dwTLS,pThdData);
				GRS_SAFEFREE(pThdData);
				return FALSE;
			}
			GRS_PRINTF(_T("为线程[ID:0x%x]设置TLS索引[%u]变量[0x%08X]\n")
				,GetCurrentProcessId(),g_dwTLS,pThdData);
		}
		break;
	case DLL_THREAD_DETACH:
		{
			VOID* pThdData = NULL;
			if(!(pThdData = TlsGetValue(g_dwTLS)))
			{
				GRS_PRINTF(_T("为线程[ID:0x%x]获取TLS索引[%u]变量失败!\n")
					,GetCurrentThreadId(),g_dwTLS);
				return FALSE;
			}
			GRS_PRINTF(_T("为线程[ID:0x%x]获取TLS索引[%u]变量[0x%08X]并销毁\n")
				,GetCurrentProcessId(),g_dwTLS,pThdData);
			GRS_SAFEFREE(pThdData);
		}
		break;
	case DLL_PROCESS_DETACH:
		{
			GRS_PRINTF(_T("释放进程[ID:0x%x]TLS索引:%u\n"),GetCurrentProcessId(),g_dwTLS);
			TlsFree(g_dwTLS);
		}
		break;
	}
	return TRUE;
}

VOID* GetThreadBuf()
{
	return TlsGetValue(g_dwTLS);
}

UINT GetThreadBufSize()
{
	return g_nBufSize;
}