#pragma once

#include <atlcoll.h>
#include "IoDefine.h"

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_CREALLOC(p,sz)	HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

//--------------------
// IO ²Ù×÷¶¯Ì¬»º³åÇø
//--------------------
class CIoBuffer  
{
public:

	OVERLAPPED m_ol;
	int m_nType;
	UINT m_nIoSize;
	
	bool m_bKeepAlive;

	//CAtlArray<char> m_vtBuffer;
    char* m_vtBuffer;
    UINT m_nLen;
    UINT m_nMaxLen;
   
	WSABUF m_wsabuf;

	CIoBuffer();
	CIoBuffer(int nIoType);

	virtual ~CIoBuffer();
	
	void SetType(int nIoType);
	void AddData(const char * pData, UINT nSize);
	void AddData(const void * pData, UINT nSize);
	void AddData(UINT data);
	void AddData(int data);
	void AddData(char c);
	void AddData(const string & str);
	void AddData(CIoBuffer & IoBuffer);
	void Clear();

	void InsertData(UINT nPos, const char * pData, UINT nSize);
	void Init();

	void Reserve(UINT len);
	void IncreaseCapacity(UINT LenInc);

	void SetIoSize(UINT nSize) 
    {
        m_nIoSize = nSize;
    }
	int GetLen() 
    {
        return m_nLen;
    }
	int GetIoSize() 
    {
        return m_nIoSize;
    }

	int GetType() 
    {
        return m_nType;
    }
	WSABUF * GetWSABuffer() 
    {
        return &m_wsabuf;
    }
	char * GetBuffer() 
    {
        return m_vtBuffer;
    }

private:

};