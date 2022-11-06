#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <pdh.h>
#include <pdhmsg.h>
#pragma comment(lib, "pdh.lib")

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);


BOOL EnumObjects(LPCTSTR pszMachineName,DWORD dwDetailLevel,LPTSTR* pszObjects)
{
	GRS_USEPRINTF();
	DWORD dwBufLen = 0;
	PDH_STATUS pdhStatus = PdhEnumObjects(NULL,
		pszMachineName,
		NULL,
		&dwBufLen,
		dwDetailLevel,
		TRUE		//第一次用TRUE 刷新下
		);
	if(PDH_MORE_DATA != pdhStatus)
	{
		GRS_PRINTF(_T("获取计数器对象缓冲尺寸失败，错误码：0x%08X\n"),pdhStatus);
		return FALSE;
	}
	*pszObjects = (LPTSTR)GRS_CALLOC(dwBufLen*sizeof(TCHAR));


	pdhStatus = PdhEnumObjects(NULL,
		pszMachineName,
		*pszObjects,
		&dwBufLen,
		dwDetailLevel,
		FALSE //第二次就不刷新了
		);

	if(ERROR_SUCCESS != pdhStatus)
	{
		GRS_PRINTF(_T("获取计数器对象缓冲失败，错误码：0x%08X\n"),pdhStatus);
		GRS_SAFEFREE(*pszObjects);
		return FALSE;
	}

	return TRUE;
}

BOOL EnumCountersInstance(LPCTSTR pszMachineName,LPCTSTR pszObject,DWORD dwDetailLevel,LPTSTR* pszCounters,LPTSTR* pszInstance)
{
	GRS_USEPRINTF();
	DWORD dwCounterBufLen = 0;
	DWORD dwInstanceBufLen = 0;

	PDH_STATUS pdhStatus = PdhEnumObjectItems (
		NULL ,                   
		pszMachineName ,          
		pszObject ,			 
		NULL,    
		& dwCounterBufLen ,    
		NULL ,   
		& dwInstanceBufLen ,   
		dwDetailLevel ,			 
		0 ) ; // 最后一个参数系统保留为0

	if(PDH_MORE_DATA != pdhStatus)
	{
		GRS_PRINTF(_T("获取计数器及实例缓冲尺寸失败，错误码：0x%08X\n"),pdhStatus);
		return FALSE;
	}

	if(0 < dwCounterBufLen)
	{
		*pszCounters = (LPTSTR)GRS_CALLOC(dwCounterBufLen * sizeof(TCHAR));
	}
	if(0 < dwInstanceBufLen)
	{
		*pszInstance = (LPTSTR)GRS_CALLOC(dwInstanceBufLen * sizeof(TCHAR));
	}
	
	pdhStatus = PdhEnumObjectItems (
		NULL ,                   
		pszMachineName ,          
		pszObject ,			 
		*pszCounters,    
		& dwCounterBufLen ,    
		*pszInstance,   
		& dwInstanceBufLen ,   
		dwDetailLevel ,			 
		0 ) ; // 最后一个参数系统保留为0
	if(PDH_CSTATUS_VALID_DATA != pdhStatus)
	{
		GRS_SAFEFREE(*pszCounters);
		GRS_SAFEFREE(*pszInstance);
		GRS_PRINTF(_T("获取计数器及实例缓冲尺寸失败，错误码：0x%08X\n"),pdhStatus);
		return FALSE;
	}
	
	return TRUE;
}

BOOL FormatCounterName(LPCTSTR pszObject,LPCTSTR pszCounter,LPCTSTR pszInstance,LPTSTR pszCounterName)
{
	HRESULT hr = S_OK;
	if(NULL == pszInstance)
	{
		hr  = StringCchPrintf(pszCounterName,PDH_MAX_COUNTER_PATH,_T("\\%s\\%s"),pszObject,pszCounter);
	}
	else
	{
		hr = StringCchPrintf(pszCounterName,PDH_MAX_COUNTER_PATH,_T("\\%s(%s)\\%s"),pszObject,pszInstance,pszCounter);
	}
	return (S_OK == hr);
}
BOOL ShowCountValue(PDH_HQUERY hQuery,LPTSTR pszCounterName)
{
	GRS_USEPRINTF();
	PDH_HCOUNTER hCounter = NULL;
	DWORD dwBufLen = 0;
	PPDH_COUNTER_INFO lpBuffer = NULL;
	PDH_FMT_COUNTERVALUE CounterData = {};
	PDH_RAW_COUNTER CounterRawData1 = {};
	PDH_RAW_COUNTER CounterRawData2 = {};
	DWORD dwType = 0;
	LONGLONG llTimeBase = 0;
	PDH_STATUS	pdhStatus = PDH_CSTATUS_VALID_DATA;
	BOOL bRet = TRUE;

	GRS_PRINTF(_T("\t%s :\t"),pszCounterName);

	pdhStatus = PdhAddCounter(hQuery,pszCounterName,NULL,&hCounter);
	if(PDH_CSTATUS_VALID_DATA != pdhStatus)
	{
		GRS_PRINTF(_T("计数器添加失败,错误码：0x%08x！\n"),pdhStatus);
		bRet = FALSE; 
	}
	else
	{
		//取得Counter的信息
		pdhStatus = PdhGetCounterInfo(hCounter,TRUE,&dwBufLen,lpBuffer);
		if(PDH_MORE_DATA != pdhStatus)
		{
			GRS_PRINTF(_T("\n\t\t获取计数器对象尺寸信息失败，错误码：0x%08x\n\t"),pdhStatus);
		}

		lpBuffer = (PPDH_COUNTER_INFO)GRS_CALLOC(dwBufLen);
		pdhStatus = PdhGetCounterInfo(hCounter,TRUE,&dwBufLen,lpBuffer);
		if(PDH_CSTATUS_VALID_DATA != pdhStatus)
		{
			GRS_PRINTF(_T("\n\t\t获取计数器对象信息失败，错误码：0x%08x\n\t"),pdhStatus);
		}
		//GRS_PRINTF(_T("\n描述：%s\n\t"),lpBuffer->szExplainText);
		GRS_SAFEFREE(lpBuffer);

		pdhStatus =  PdhCollectQueryData(hQuery);
		if(PDH_CSTATUS_VALID_DATA != pdhStatus)
		{
			GRS_PRINTF(_T("收集计数器信息失败,错误码：0x%08X！\n"),pdhStatus);	
			bRet = FALSE;
		}
		else
		{
			ZeroMemory(&CounterData,sizeof( PDH_FMT_COUNTERVALUE));
			//pdhStatus = PdhGetFormattedCounterValue(hCounter,PDH_FMT_RAW ,&dwType,&CounterData);
			pdhStatus = PdhGetCounterTimeBase(hCounter,&llTimeBase);
			pdhStatus = PdhGetRawCounterValue(hCounter,&dwType,&CounterRawData1);
			Sleep(20);
			pdhStatus =  PdhCollectQueryData(hQuery);
			pdhStatus = PdhGetRawCounterValue(hCounter,&dwType,&CounterRawData2);
			pdhStatus = PdhFormatFromRawValue(dwType,PDH_FMT_DOUBLE,&llTimeBase,&CounterRawData2,&CounterRawData1,&CounterData);

			if(	PDH_CSTATUS_VALID_DATA !=  pdhStatus)
			{
				GRS_PRINTF(_T("格式化计数器的值失败,错误码：0x%08x！\n"),pdhStatus);
				bRet = FALSE;
			}
			else
			{
				GRS_PRINTF(_T("%.20g\n"),CounterData.doubleValue);	
			}
		}
		PdhRemoveCounter(hCounter);
	}
	return bRet;
}
BOOL DisplayObjectCounter(LPCTSTR pszObject,LPCTSTR pszMachineName = NULL,DWORD dwDetailLevel = PERF_DETAIL_NOVICE)
{
	GRS_USEPRINTF();
	LPTSTR pszCounters = NULL;
	LPTSTR pszInstance = NULL;
	TCHAR pszCounterName[PDH_MAX_COUNTER_PATH] = {};

	PDH_HQUERY hQuery = NULL;
	PDH_STATUS	pdhStatus = PDH_CSTATUS_VALID_DATA;
	pdhStatus = PdhOpenQuery(NULL,NULL,&hQuery);
	if(PDH_CSTATUS_VALID_DATA != pdhStatus)
	{
		GRS_PRINTF(_T("无法打开性能数据库句柄，错误码：0x%08x！\n"),pdhStatus);
		return FALSE;
	}

	GRS_PRINTF(_T("Object[%s]:\n"),pszObject);

	if(EnumCountersInstance(pszMachineName,pszObject,dwDetailLevel,&pszCounters,&pszInstance))
	{//计数器对象
		for( LPTSTR j = pszCounters;NULL != *j; j += (lstrlen(j) + 1))
		{//计数器
			if(NULL == pszInstance)
			{//无实例计数器
				FormatCounterName(pszObject,j,NULL,pszCounterName);
				ShowCountValue(hQuery,pszCounterName);
			}
			else
			{//有实例计数器
				for(LPTSTR k = pszInstance; NULL != *k; k+= (lstrlen(k) + 1))
				{
					FormatCounterName(pszObject,j,k,pszCounterName);
					ShowCountValue(hQuery,pszCounterName);
				}
			}
		}
		GRS_PRINTF(_T("\n"));
	}
	else
	{
		GRS_PRINTF(_T("对象[%s]没有相应的计数器及实例\n"),pszObject);
	}
	GRS_SAFEFREE(pszCounters);
	GRS_SAFEFREE(pszInstance);
	PdhCloseQuery (hQuery);
	return TRUE;

}
BOOL DisplayAllCounter(LPCTSTR pszMachineName = NULL,DWORD dwDetailLevel = PERF_DETAIL_NOVICE)
{
	LPTSTR pszObjects = NULL;
	if(!EnumObjects(pszMachineName,dwDetailLevel,&pszObjects))
	{
		GRS_SAFEFREE(pszObjects);
		return FALSE;
	}

	for(LPTSTR i = pszObjects; NULL != *i; i += (lstrlen(i) + 1))
	{
		DisplayObjectCounter(i,pszMachineName,dwDetailLevel);
	}
	GRS_SAFEFREE(pszObjects);

	return TRUE;
}


int _tmain()
{
	//DisplayAllCounter();
	DisplayObjectCounter(_T("Process"));

	_tsystem(_T("PAUSE"));
	return 0;
}