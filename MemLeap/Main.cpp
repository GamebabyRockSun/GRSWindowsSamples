#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define H_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define H_CALLOC(sz)	HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define H_REALLOC(p,sz) HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#define H_FREE(p)		HeapFree(GetProcessHeap(),0,p)

//֧��memory leak ��new �� delete ��Ҫ���ڵ���
#ifdef _DEBUG
HANDLE g_hBlockInfoHeap = NULL;
typedef struct _ST_BLOCK_INFO
{
	TCHAR m_pszSourceFile[MAX_PATH];
	INT   m_iSourceLine;
	BOOL  m_bDelete;
	VOID* m_pMemBlock;
}ST_BLOCK_INFO,*PST_BLOCK_INFO;

UINT		   g_nMaxBlockCnt = 100;
UINT		   g_nCurBlockIndex = 0;
PST_BLOCK_INFO g_pMemBlockList = NULL;

void* __cdecl operator new(size_t nSize,LPCTSTR pszCppFile,int iLine)
{
	if(NULL != g_pMemBlockList)
	{	
		if( g_nCurBlockIndex >= g_nMaxBlockCnt )	
		{//�����ǰ����Ϣ�������ֵ,��ô����������
			g_nMaxBlockCnt *= 2;
			g_pMemBlockList = (PST_BLOCK_INFO)HeapReAlloc(g_hBlockInfoHeap,HEAP_ZERO_MEMORY,g_pMemBlockList,g_nMaxBlockCnt * sizeof(ST_BLOCK_INFO));
		}
	}
	else
	{
		if( NULL == g_hBlockInfoHeap )
		{//�������Ϣ�ѻ�û�д�����ô�ʹ���,������ΪLFH
			g_hBlockInfoHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS,0,0);
			ULONG  HeapFragValue = 2;
			HeapSetInformation( g_hBlockInfoHeap,HeapCompatibilityInformation,&HeapFragValue ,sizeof(HeapFragValue) ) ;
		}
		//�������Ϣ����
		g_pMemBlockList = (PST_BLOCK_INFO)HeapAlloc(g_hBlockInfoHeap,HEAP_ZERO_MEMORY,g_nMaxBlockCnt * sizeof(ST_BLOCK_INFO));
	}
	
	void* pRet = H_ALLOC(nSize);
	
	//��¼��ǰ��������������Ϣ
	g_pMemBlockList[g_nCurBlockIndex].m_pMemBlock	= pRet;
	StringCchCopy(g_pMemBlockList[g_nCurBlockIndex].m_pszSourceFile,MAX_PATH,pszCppFile);
	g_pMemBlockList[g_nCurBlockIndex].m_iSourceLine = iLine;
	g_pMemBlockList[g_nCurBlockIndex].m_bDelete		= FALSE;

	++ g_nCurBlockIndex;

	return pRet;
}

void __cdecl operator delete(void* p)
{
	for( UINT i = 0;i < g_nCurBlockIndex; i ++ )
	{//��������Ϣ����,�ҵ���Ӧ�Ŀ���Ϣ,������ɾ�����	
		if( p == g_pMemBlockList[i].m_pMemBlock )
		{
			g_pMemBlockList[i].m_bDelete = TRUE;
			break;
		}
	}
	H_FREE(p);
}

void __cdecl operator delete(void* p,LPCTSTR pszCppFile,int iLine)
{
	::operator delete(p);
	H_FREE(p);
}

void GRSMemoryLeak(BOOL bDestroyHeap = FALSE)
{//�ڴ�й¶���
	TCHAR pszOutPutInfo[2*MAX_PATH];
	BOOL  bRecord = FALSE;
	PROCESS_HEAP_ENTRY phe = {};
	HeapLock(GetProcessHeap());
	OutputDebugString(_T("��ʼ����ڴ�й¶���.........\n"));

	//��������Ĭ�϶�
	while (HeapWalk(GetProcessHeap(), &phe))
	{
		if( PROCESS_HEAP_ENTRY_BUSY & phe.wFlags )
		{
			bRecord = FALSE;
			for(UINT i = 0; i < g_nCurBlockIndex; i ++ )
			{
				if( phe.lpData == g_pMemBlockList[i].m_pMemBlock )
				{
					if(!g_pMemBlockList[i].m_bDelete)
					{
						StringCchPrintf(pszOutPutInfo,2*MAX_PATH,_T("%s(%d):�ڴ��(Point=0x%08X,Size=%u)\n")
							,g_pMemBlockList[i].m_pszSourceFile,g_pMemBlockList[i].m_iSourceLine,phe.lpData,phe.cbData);
						OutputDebugString(pszOutPutInfo);
					}
					bRecord = TRUE;
					break;
				}
			}
			if( !bRecord )
			{
				StringCchPrintf(pszOutPutInfo,2*MAX_PATH,_T("δ��¼���ڴ��(Point=0x%08X,Size=%u)\n")
					,phe.lpData,phe.cbData);
				OutputDebugString(pszOutPutInfo);
			}
		}
		
	}
	HeapUnlock(GetProcessHeap());
	OutputDebugString(_T("�ڴ�й¶������.\n"));
	if( bDestroyHeap )
	{
		HeapDestroy(g_hBlockInfoHeap);
	}
}

//__FILE__Ԥ�����Ŀ��ַ��汾
#define GRS_WIDEN2(x) L ## x
#define GRS_WIDEN(x) GRS_WIDEN2(x)
#define __WFILE__ GRS_WIDEN(__FILE__)

#define new new(__WFILE__,__LINE__)
#define delete(p) ::operator delete(p,__WFILE__,__LINE__)
#endif

int _tmain()
{
	int* pInt1 = new int;
	int* pInt2 = new int;
	float* pFloat1 = new float;

	BYTE* pBt = new BYTE[100];


	delete pInt2;

	GRSMemoryLeak();
	return 0;
}