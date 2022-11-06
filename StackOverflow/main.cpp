#include <tchar.h>
#include <malloc.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

void recursive(int recurse)
{
	int iArray[2000]={}; //����ջ�ռ�
	if (recurse)
	{
		recursive(recurse);
	}
}

void ArrayErr()
{
	int iArray[] = {3,4};
	iArray[10] = 1; //�±�Խ���޷��ָ�
}
int stack_overflow_exception_filter(int exception_code)
{
	if (exception_code == EXCEPTION_STACK_OVERFLOW)
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

int _tmain()
{
	GRS_USEPRINTF();
	int i = 0;
	int recurse = 1, result = 0;

	for (i = 0 ; i < 10 ; i++)
	{
		GRS_PRINTF(_T("loop #%d\n"), i + 1);
		__try
		{
			//ģ��ջ���
			ArrayErr();
			//recursive(recurse);
		}
		__except(stack_overflow_exception_filter(GetExceptionCode()))
		{
			GRS_PRINTF(_T("�ָ�ջ���......\n"));
			result = _resetstkoflw();
		}

		if (!result)
		{
			GRS_PRINTF(_T("�ָ�ʧ��\n"));
			break;
		}
		else
		{
			GRS_PRINTF(_T("�ָ��ɹ�\n"));
		}
	}
	_tsystem(_T("PAUSE"));
	return 0;
}