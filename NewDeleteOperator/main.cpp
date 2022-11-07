#include <tchar.h>
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN // 从 Windows 头中排除极少使用的资料
#include <Windows.h>

#define H_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define H_CALLOC(sz)	HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define H_FREE(p)		HeapFree(GetProcessHeap(),0,p)

class CMyClass
{
public:
	//一般的new和delete
	void* PASCAL operator new(size_t nSize)
	{
		return H_ALLOC(nSize);
	}
	void PASCAL operator delete(void* p)
	{
		H_FREE(p);
	}
	//一般数组的new和delete
	void* PASCAL operator new[](size_t nSize)
	{
		return H_ALLOC(nSize);
	}
	void PASCAL operator delete[](void*p)
	{
		H_FREE(p);
	}
	
	//placement new and placement delete
	void* PASCAL operator new(size_t nSize, void* p)
	{
		return p;
	}
	void PASCAL operator delete(void* p, void*)
	{
	}
	//placement new[] and placement delete[]
	void* PASCAL operator new[](size_t nSize,void* pPlace)
	{
		return pPlace;
	}
	void PASCAL operator delete[](void*p,void* pPlace)
	{
	}
#ifdef _DEBUG
	//带memory leap支持的new delete和new[] delete[]
	void* PASCAL operator new(size_t nSize, LPCSTR lpszFileName, int nLine)
	{
		return H_ALLOC(nSize);
	}

	void PASCAL operator delete(void *p, LPCSTR lpszFileName, int nLine)
	{
		H_FREE(p);
	}
	void* PASCAL operator new[](size_t nSize, LPCSTR lpszFileName, int nLine)
	{
		return H_ALLOC(nSize);
	}
	void PASCAL operator delete[](void*p, LPCSTR lpszFileName, int nLine)
	{
		H_FREE(p);
	}
#endif
protected:
	int m_iMember;
public:
	CMyClass()
		:m_iMember(0)
	{
	}
	virtual ~CMyClass()
	{
	}
public:
	void SetInt(int iNum)
	{
		m_iMember = iNum;
	}
	int GetInt(void)
	{
		return m_iMember;
	}
};

int _tmain()
{
	CMyClass* pClass = new CMyClass();
	delete pClass;

	pClass = new CMyClass[100];
	delete[] pClass;

	int parInt[100];

	CMyClass*pInPlaceClass = new(parInt) CMyClass();
	pInPlaceClass = new(parInt) CMyClass[(100 * sizeof(int) - 4) / sizeof(CMyClass)];
	return 0;
}

