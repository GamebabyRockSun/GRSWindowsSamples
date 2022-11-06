#include <tchar.h>
#include <windows.h>

#define H_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define H_CALLOC(sz)	HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define H_FREE(p)		HeapFree(GetProcessHeap(),0,p)

//��ͨ��new �� delete
void* __cdecl operator new(size_t nSize)
{
	return H_ALLOC(nSize);
}

void __cdecl operator delete(void* p)
{
	H_FREE(p);
}

#if _MSC_VER >= 1210
void* __cdecl operator new[](size_t nSize)
{
	return H_ALLOC(nSize);
}

void __cdecl operator delete[](void* p)
{
	H_FREE(p);
}
#endif

//placement �� new �� delete
void* __cdecl operator new(size_t nSize,void* pPlace)
{
	return pPlace;
}

void __cdecl operator delete(void* p,void* pPlace)
{
}

#if _MSC_VER >= 1210
void* __cdecl operator new[](size_t nSize,void* pPlace)
{
	return pPlace;
}

void __cdecl operator delete[](void* p,void* pPlace)
{
}
#endif

//֧��memory leap ��new �� delete ��Ҫ���ڵ���
#ifdef _DEBUG

void* __cdecl operator new(size_t nSize,LPCTSTR pszCppFile,int iLine)
{
	return H_ALLOC(nSize);
}

void __cdecl operator delete(void* p,LPCTSTR pszCppFile,int iLine)
{
	H_FREE(p);
}

#if _MSC_VER >= 1210
void* __cdecl operator new[](size_t nSize,LPCTSTR pszCppFile,int iLine)
{
	return H_ALLOC(nSize);
}

void __cdecl operator delete[](void* p,LPCTSTR pszCppFile,int iLine)
{
	H_FREE(p);
}
#endif

//__FILE__Ԥ�����Ŀ��ַ��汾
#define GRS_WIDEN2(x) L ## x
#define GRS_WIDEN(x) GRS_WIDEN2(x)
#define __WFILE__ GRS_WIDEN(__FILE__)

#define new new(__WFILE__,__LINE__)

#endif


int _tmain()
{
	int* pIa = new int[100];
	delete[] pIa;
	pIa = new int;
	delete pIa;

	return 0;
}