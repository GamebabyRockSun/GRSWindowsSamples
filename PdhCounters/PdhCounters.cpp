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
		TRUE		//��һ����TRUE ˢ����
		);
	if(PDH_MORE_DATA != pdhStatus)
	{
		GRS_PRINTF(_T("��ȡ���������󻺳�ߴ�ʧ�ܣ������룺0x%08X\n"),pdhStatus);
		return FALSE;
	}
	*pszObjects = (LPTSTR)GRS_CALLOC(dwBufLen*sizeof(TCHAR));


	pdhStatus = PdhEnumObjects(NULL,
		pszMachineName,
		*pszObjects,
		&dwBufLen,
		dwDetailLevel,
		FALSE //�ڶ��ξͲ�ˢ����
		);

	if(ERROR_SUCCESS != pdhStatus)
	{
		GRS_PRINTF(_T("��ȡ���������󻺳�ʧ�ܣ������룺0x%08X\n"),pdhStatus);
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
		0 ) ; // ���һ������ϵͳ����Ϊ0

	if(PDH_MORE_DATA != pdhStatus)
	{
		GRS_PRINTF(_T("��ȡ��������ʵ������ߴ�ʧ�ܣ������룺0x%08X\n"),pdhStatus);
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
		0 ) ; // ���һ������ϵͳ����Ϊ0
	if(PDH_CSTATUS_VALID_DATA != pdhStatus)
	{
		GRS_SAFEFREE(*pszCounters);
		GRS_SAFEFREE(*pszInstance);
		GRS_PRINTF(_T("��ȡ��������ʵ������ߴ�ʧ�ܣ������룺0x%08X\n"),pdhStatus);
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
		GRS_PRINTF(_T("���������ʧ��,�����룺0x%08x��\n"),pdhStatus);
		bRet = FALSE; 
	}
	else
	{
		//ȡ��Counter����Ϣ
		pdhStatus = PdhGetCounterInfo(hCounter,TRUE,&dwBufLen,lpBuffer);
		if(PDH_MORE_DATA != pdhStatus)
		{
			GRS_PRINTF(_T("\n\t\t��ȡ����������ߴ���Ϣʧ�ܣ������룺0x%08x\n\t"),pdhStatus);
		}

		lpBuffer = (PPDH_COUNTER_INFO)GRS_CALLOC(dwBufLen);
		pdhStatus = PdhGetCounterInfo(hCounter,TRUE,&dwBufLen,lpBuffer);
		if(PDH_CSTATUS_VALID_DATA != pdhStatus)
		{
			GRS_PRINTF(_T("\n\t\t��ȡ������������Ϣʧ�ܣ������룺0x%08x\n\t"),pdhStatus);
		}
		//GRS_PRINTF(_T("\n������%s\n\t"),lpBuffer->szExplainText);
		GRS_SAFEFREE(lpBuffer);

		pdhStatus =  PdhCollectQueryData(hQuery);
		if(PDH_CSTATUS_VALID_DATA != pdhStatus)
		{
			GRS_PRINTF(_T("�ռ���������Ϣʧ��,�����룺0x%08X��\n"),pdhStatus);	
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
				GRS_PRINTF(_T("��ʽ����������ֵʧ��,�����룺0x%08x��\n"),pdhStatus);
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
		GRS_PRINTF(_T("�޷����������ݿ����������룺0x%08x��\n"),pdhStatus);
		return FALSE;
	}

	GRS_PRINTF(_T("Object[%s]:\n"),pszObject);

	if(EnumCountersInstance(pszMachineName,pszObject,dwDetailLevel,&pszCounters,&pszInstance))
	{//����������
		for( LPTSTR j = pszCounters;NULL != *j; j += (lstrlen(j) + 1))
		{//������
			if(NULL == pszInstance)
			{//��ʵ��������
				FormatCounterName(pszObject,j,NULL,pszCounterName);
				ShowCountValue(hQuery,pszCounterName);
			}
			else
			{//��ʵ��������
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
		GRS_PRINTF(_T("����[%s]û����Ӧ�ļ�������ʵ��\n"),pszObject);
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