#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <winperf.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}
#define GRS_MSIZE(p)        HeapSize(GetProcessHeap(),0,p)
#define GRS_REALLOC(p,sz)   HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define RESERVED        0L
#define INITIAL_SIZE    40960L
#define EXTEND_SIZE     4096L
#define LINE_LENGTH     80
#define WRAP_POINT      LINE_LENGTH - 12
#define OBJECT_TEXT_INDENT 4

#define COMMAND_ERROR   0
#define COMMAND_HELP    1
#define COMMAND_COUNTER 2

typedef struct _rawdata
{
    DWORD CounterType;
    ULONGLONG Data;          //Raw counter data
    LONGLONG Time;           //Is a time value or a base value
    DWORD MultiCounterData;  //Second raw counter value for multi-valued counters
    LONGLONG Frequency;
}RAW_DATA, *PRAW_DATA;


LPTSTR NamesKey = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib");
//LPTSTR DefaultLangId = _T("009");
LPTSTR DefaultLangId = _T("CurrentLanguage");
LPTSTR Counters = _T("Counter");
LPTSTR Help = _T("Help");
LPTSTR LastHelp = _T("Last Help");
LPTSTR LastCounter = _T("Last Counter");
LPTSTR Slash = _T("\\");
LPTSTR Global = _T("Global");

TCHAR cValueName[LINE_LENGTH] = {};
CHAR  cExeName[LINE_LENGTH * 2] = {};
CHAR  cCtrName[LINE_LENGTH] = {};

BOOL StringToInt(LPTSTR inStr,PDWORD pdwOutInt)
{
    if( (!inStr) || (!pdwOutInt) )
    {
        SetLastError(ERROR_BAD_ARGUMENTS);
        return FALSE;
    }

    if( (_stscanf(inStr,_T(" %d"),pdwOutInt)) != EOF )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

VOID DisplayCommandHelp()
{
    GRS_USEPRINTF();
    GRS_PRINTF(_T("\n\nPERFEX"));
    GRS_PRINTF(_T("\n\tSample program to demonstrate Win 32 Performance API"));
    GRS_PRINTF(_T("\n\nUsage:"));
    GRS_PRINTF(_T("\n\n\t perfex [-? | counter]"));
    GRS_PRINTF(_T("\n\n\t -? Displays a list of all counter objects in the system"));
    GRS_PRINTF(_T("\n\tcounter is the name of a counter object"));
    GRS_PRINTF(_T("\n\t\tand displays the performance counters for that object"));
    GRS_PRINTF(_T("\n"));
}

LPTSTR* BuildNameTable(HKEY hKeyRegistry,LPTSTR lpszLangId,PDWORD pdwLastItem)
{
    LPTSTR* lpReturnValue = NULL;
    LPTSTR* lpCounterId = NULL;
    LPTSTR  lpCounterNames = NULL;
    LPTSTR  lpHelpText = NULL;

    LPTSTR  lpThisName = NULL;

    BOOL bStatus = TRUE;
    LONG lWin32Status = 0;

    DWORD dwHelpItems = 0;
    DWORD dwCounterItems = 0;
    DWORD dwValueType = 0;
    DWORD dwArraySize = 0;
    DWORD dwBufferSize = 0;
    DWORD dwCounterSize = 0;
    DWORD dwHelpSize = 0;
    DWORD dwThisCounter = 0;

    DWORD dwLastId = 0;

    HKEY  hKeyValue = 0;
    HKEY  hKeyNames = 0;

    LPTSTR lpValueNameString = NULL;

    if(!lpszLangId)
    {
        lpszLangId = DefaultLangId;
    }

    lWin32Status = RegOpenKeyEx(hKeyRegistry,NamesKey,RESERVED,KEY_READ,&hKeyValue);
    if(ERROR_SUCCESS != lWin32Status)
    {
        goto BNT_BAILOUT;
    }

    dwBufferSize = sizeof(dwHelpItems);

    lWin32Status = RegQueryValueEx(hKeyValue,LastHelp,RESERVED,&dwValueType,(LPBYTE)&dwHelpItems,&dwBufferSize);
    if( (ERROR_SUCCESS != lWin32Status) || (REG_DWORD != dwValueType) )
    {
        goto BNT_BAILOUT;
    }

    dwBufferSize = sizeof(dwCounterItems);
    lWin32Status = RegQueryValueEx(hKeyValue,LastCounter,RESERVED,&dwValueType,(LPBYTE)&dwCounterItems,&dwBufferSize);
    if((ERROR_SUCCESS != lWin32Status) || (REG_DWORD != dwValueType))
    {
        goto BNT_BAILOUT;
    }

    if(dwHelpItems >= dwCounterItems)
    {
        dwLastId = dwHelpItems;
    }
    else
    {
        dwLastId = dwCounterItems;
    }

    dwArraySize = (dwLastId + 1) * sizeof(LPTSTR);

    lpValueNameString = (LPTSTR)GRS_CALLOC(lstrlen(NamesKey)*sizeof(TCHAR)
        + lstrlen(Slash)* sizeof(TCHAR)
        + lstrlen(lpszLangId) * sizeof(TCHAR)
        + sizeof(TCHAR));

    if(!lpValueNameString)
    {
        lWin32Status = ERROR_OUTOFMEMORY;
        goto BNT_BAILOUT;
    }

    lstrcpy(lpValueNameString,NamesKey);
    lstrcat(lpValueNameString,Slash);
    lstrcat(lpValueNameString,lpszLangId);

    lWin32Status = RegOpenKeyEx(hKeyRegistry,lpValueNameString,RESERVED,KEY_READ,&hKeyNames);
    if( ERROR_SUCCESS != lWin32Status )
    {
        goto BNT_BAILOUT;
    }

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx(hKeyNames,Counters,RESERVED,&dwValueType,NULL,&dwBufferSize);
    if(ERROR_SUCCESS != lWin32Status)
    {
        goto BNT_BAILOUT;
    }

    dwCounterSize = dwBufferSize;
    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx(hKeyNames,Help,RESERVED,&dwValueType,NULL,&dwBufferSize);
    if(ERROR_SUCCESS != lWin32Status)
    {
        goto BNT_BAILOUT;
    }

    dwHelpSize = dwBufferSize;

    lpReturnValue = (LPTSTR*)GRS_CALLOC(dwArraySize + dwCounterSize + dwHelpSize);

    if( !lpReturnValue )
    {
        lWin32Status = ERROR_OUTOFMEMORY;
        goto BNT_BAILOUT;
    }

    lpCounterId = lpReturnValue;
    lpCounterNames = (LPTSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText = (LPTSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueEx(hKeyNames,Counters,RESERVED,&dwValueType
        ,(LPBYTE)lpCounterNames,&dwBufferSize);
    if(ERROR_SUCCESS != lWin32Status)
    {
        goto BNT_BAILOUT;
    }

    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueEx(hKeyNames,Help,RESERVED
        ,&dwValueType,(LPBYTE)lpHelpText,&dwBufferSize);
    if(ERROR_SUCCESS != lWin32Status)
    {
        goto BNT_BAILOUT;
    }

    for(lpThisName = lpCounterNames;*lpThisName;lpThisName += (lstrlen(lpThisName)+1) )
    {
        bStatus = StringToInt(lpThisName,&dwThisCounter);
        if(!bStatus)
        {
            goto BNT_BAILOUT;
        }
        lpThisName += (lstrlen(lpThisName) + 1);
        lpCounterId[dwThisCounter] = lpThisName;
    }

    for(lpThisName = lpHelpText;*lpThisName;lpThisName +=(lstrlen(lpThisName)+1))
    {
        bStatus = StringToInt(lpThisName,&dwThisCounter);
        if(!bStatus)
        {
            goto BNT_BAILOUT;
        }
        lpThisName += (lstrlen(lpThisName)+1);
        lpCounterId[dwThisCounter] = lpThisName;
    }

    if(pdwLastItem)
    {
        *pdwLastItem = dwLastId;
    }
   
    GRS_SAFEFREE(lpValueNameString);
  
    RegCloseKey(hKeyValue);
    RegCloseKey(hKeyNames);

    return lpReturnValue;
BNT_BAILOUT:
    if(lWin32Status != ERROR_SUCCESS)
    {
        SetLastError(lWin32Status);
    }
    GRS_SAFEFREE(lpValueNameString);
    GRS_SAFEFREE(lpReturnValue);

    if(hKeyValue)
    {
        RegCloseKey(hKeyValue);
    }
    if(hKeyNames)
    {
        RegCloseKey(hKeyNames);
    }
    return NULL;
}

DWORD GetSystemPerfData(HKEY hKeySystem,LPTSTR pValue,PPERF_DATA_BLOCK*pPerfData)
{
    LONG lError = 0;
    DWORD Size = 0;
    DWORD Type = 0;

    if(NULL == *pPerfData)
    {
        *pPerfData = (PPERF_DATA_BLOCK)GRS_CALLOC(INITIAL_SIZE);
        if( NULL == *pPerfData )
        {
            return ERROR_OUTOFMEMORY;
        }
    }

    while(TRUE)
    {
        Size = GRS_MSIZE(*pPerfData);
        lError = RegQueryValueEx(hKeySystem,pValue,RESERVED,&Type,(LPBYTE)*pPerfData,&Size);

        if((!lError)
            && (Size > 0)
            && (*pPerfData)->Signature[0] == _T('P')
            && (*pPerfData)->Signature[0] == _T('E')
            && (*pPerfData)->Signature[0] == _T('R')
            && (*pPerfData)->Signature[0] == _T('F'))
        {
            return ERROR_SUCCESS;
        }
        if(lError == ERROR_MORE_DATA)
        {
            *pPerfData = (PPERF_DATA_BLOCK)GRS_REALLOC(*pPerfData,GRS_MSIZE(*pPerfData) + EXTEND_SIZE);
            if(!*pPerfData)
            {
                return (lError);
            }
        }
        else
        {
            return lError;
        }
    }
}

PPERF_OBJECT_TYPE FirstObject(PPERF_DATA_BLOCK pPerfData)
{
    return ((PPERF_OBJECT_TYPE)((PBYTE)pPerfData + pPerfData->HeaderLength));
}

PPERF_OBJECT_TYPE NextObject(PPERF_OBJECT_TYPE pObject)
{
    return ((PPERF_OBJECT_TYPE) ((PBYTE)pObject + pObject->TotalByteLength));
}

PERF_COUNTER_DEFINITION* FirstCounter(PPERF_OBJECT_TYPE pObjectDef)
{
    return (PERF_COUNTER_DEFINITION*)((PCHAR)pObjectDef + pObjectDef->HeaderLength);
}

PERF_COUNTER_DEFINITION* NextCounter(PERF_COUNTER_DEFINITION* pCounterDef)
{
    return (PERF_COUNTER_DEFINITION*)((PCHAR)pCounterDef + pCounterDef->ByteLength);
}

PERF_INSTANCE_DEFINITION* FirstInstance(PPERF_OBJECT_TYPE pObject)
{
    return (PERF_INSTANCE_DEFINITION*)((PBYTE)pObject + pObject->DefinitionLength);
}

PERF_INSTANCE_DEFINITION* NextInstance(PERF_INSTANCE_DEFINITION* pInstance)
{
    PERF_COUNTER_BLOCK* pCtrBlk = NULL;

    pCtrBlk = (PERF_COUNTER_BLOCK*)((PBYTE)pInstance + pInstance->ByteLength);

    return (PERF_INSTANCE_DEFINITION*)((PBYTE)pInstance + pInstance->ByteLength + pCtrBlk->ByteLength);
}

LONG PrintExplainText(DWORD Indent,LPTSTR szTextString)
{
    GRS_USEPRINTF();
    LPTSTR szThisChar = NULL;
    BOOL bNewLine = FALSE;

    DWORD dwThisPos = 0;
    DWORD dwLinesUsed = 0;

    szThisChar = szTextString;
    
    if(!szTextString)
    {
        return dwLinesUsed;
    }

    if(Indent > WRAP_POINT)
    {
        return dwLinesUsed;
    }

    bNewLine = TRUE;

    while(*szThisChar)
    {
        if(bNewLine)
        {
            for(dwThisPos = 0;dwThisPos < Indent; dwThisPos ++)
            {
                GRS_PRINTF(_T(" "));
            }
            bNewLine = FALSE;
        }
        if((*szThisChar == ' ') && (dwThisPos >= WRAP_POINT))
        {
            GRS_PRINTF(_T("\n"));
            bNewLine = TRUE;
            while(*szThisChar <= ' ')
            {
                szThisChar ++;
            }
            dwLinesUsed++;
        }
        else
        {
            GRS_PRINTF(_T("%c"),*szThisChar);
            szThisChar ++;
        }
        dwThisPos ++;
    }
    GRS_PRINTF(_T("\n"));
    bNewLine = TRUE;
    dwLinesUsed ++;
    return dwLinesUsed;
}

void DisplayObjectNames(PPERF_DATA_BLOCK pPerf,LPTSTR *pNames)
{
    GRS_USEPRINTF();
    DWORD dwThisObject = 0;
    PPERF_OBJECT_TYPE pThisObject = NULL;

    GRS_PRINTF(_T("\nThe available performance counter objects are:"));
    for(dwThisObject = 0,pThisObject = FirstObject(pPerf)
        ;dwThisObject < pPerf->NumObjectTypes
        ;dwThisObject++,pThisObject = NextObject(pThisObject))
    {
        GRS_PRINTF(_T("\n\"%ws\"\n")
            ,pNames[pThisObject->ObjectNameTitleIndex]);
        PrintExplainText(OBJECT_TEXT_INDENT,pNames[pThisObject->ObjectHelpTitleIndex]);
    }
    GRS_PRINTF(_T("\n"));
}

BOOL GetValue(LPBYTE pPerfData,PERF_OBJECT_TYPE* pObject, 
              PERF_COUNTER_DEFINITION* pCounter, 
              PERF_COUNTER_BLOCK* pCounterDataBlock, 
              PRAW_DATA pRawData)
{
    PVOID pData = NULL;
    UNALIGNED ULONGLONG* pullData = NULL;
    PERF_COUNTER_DEFINITION* pBaseCounter = NULL;
    BOOL fSuccess = TRUE;

    //Point to the raw counter data.
    pData = (PVOID)((LPBYTE)pCounterDataBlock + pCounter->CounterOffset);

    //Now use the PERF_COUNTER_DEFINITION.CounterType value to figure out what
    //other information you need to calculate a displayable value.
    pRawData->CounterType = pCounter->CounterType;
    switch (pCounter->CounterType) 
    {

    case PERF_COUNTER_COUNTER:
    case PERF_COUNTER_QUEUELEN_TYPE:
    case PERF_SAMPLE_COUNTER:
        pRawData->Data = (ULONGLONG)(*(DWORD*)pData);
        pRawData->Time = ((PERF_DATA_BLOCK*)pPerfData)->PerfTime.QuadPart;
        if (PERF_COUNTER_COUNTER == pCounter->CounterType || PERF_SAMPLE_COUNTER == pCounter->CounterType)
        {
            pRawData->Frequency = ((PERF_DATA_BLOCK*)pPerfData)->PerfFreq.QuadPart;
        }
        break;

    case PERF_OBJ_TIME_TIMER:
        pRawData->Data = (ULONGLONG)(*(DWORD*)pData);
        pRawData->Time = pObject->PerfTime.QuadPart;
        break;

    case PERF_COUNTER_100NS_QUEUELEN_TYPE:
        pRawData->Data = *(UNALIGNED ULONGLONG *)pData;
        pRawData->Time = ((PERF_DATA_BLOCK*)pPerfData)->PerfTime100nSec.QuadPart;
        break;

    case PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE:
        pRawData->Data = *(UNALIGNED ULONGLONG *)pData;
        pRawData->Time = pObject->PerfTime.QuadPart;
        break;

    case PERF_COUNTER_TIMER:
    case PERF_COUNTER_TIMER_INV:
    case PERF_COUNTER_BULK_COUNT:
    case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
        pullData = (UNALIGNED ULONGLONG *)pData;
        pRawData->Data = *pullData;
        pRawData->Time = ((PERF_DATA_BLOCK*)pPerfData)->PerfTime.QuadPart;
        if (pCounter->CounterType == PERF_COUNTER_BULK_COUNT)
        {
            pRawData->Frequency = ((PERF_DATA_BLOCK*)pPerfData)->PerfFreq.QuadPart;
        }
        break;

    case PERF_COUNTER_MULTI_TIMER:
    case PERF_COUNTER_MULTI_TIMER_INV:
        pullData = (UNALIGNED ULONGLONG *)pData;
        pRawData->Data = *pullData;
        pRawData->Frequency = ((PERF_DATA_BLOCK*)pPerfData)->PerfFreq.QuadPart;
        pRawData->Time = ((PERF_DATA_BLOCK*)pPerfData)->PerfTime.QuadPart;

        //These counter types have a second counter value that is adjacent to
        //this counter value in the counter data block. The value is needed for
        //the calculation.
        if ((pCounter->CounterType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER)
        {
            ++pullData;
            pRawData->MultiCounterData = *(DWORD*)pullData;
        }
        break;

        //These counters do not use any time reference.
    case PERF_COUNTER_RAWCOUNT:
    case PERF_COUNTER_RAWCOUNT_HEX:
    case PERF_COUNTER_DELTA:
        pRawData->Data = (ULONGLONG)(*(DWORD*)pData);
        pRawData->Time = 0;
        break;

    case PERF_COUNTER_LARGE_RAWCOUNT:
    case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
    case PERF_COUNTER_LARGE_DELTA:
        pRawData->Data = *(UNALIGNED ULONGLONG*)pData;
        pRawData->Time = 0;
        break;

        //These counters use the 100ns time base in their calculation.
    case PERF_100NSEC_TIMER:
    case PERF_100NSEC_TIMER_INV:
    case PERF_100NSEC_MULTI_TIMER:
    case PERF_100NSEC_MULTI_TIMER_INV:
        pullData = (UNALIGNED ULONGLONG*)pData;
        pRawData->Data = *pullData;
        pRawData->Time = ((PERF_DATA_BLOCK*)pPerfData)->PerfTime100nSec.QuadPart;

        //These counter types have a second counter value that is adjacent to
        //this counter value in the counter data block. The value is needed for
        //the calculation.
        if ((pCounter->CounterType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER)
        {
            ++pullData;
            pRawData->MultiCounterData = *(DWORD*)pullData;
        }
        break;

        //These counters use two data points, this value and one from this counter's
        //base counter. The base counter should be the next counter in the object's 
        //list of counters.
    case PERF_SAMPLE_FRACTION:
    case PERF_RAW_FRACTION:
        pRawData->Data = (ULONGLONG)(*(DWORD*)pData);
        pBaseCounter = pCounter+1;  //Get base counter
        if ((pBaseCounter->CounterType & PERF_COUNTER_BASE) == PERF_COUNTER_BASE)
        {
            pData = (PVOID)((LPBYTE)pCounterDataBlock + pBaseCounter->CounterOffset);
            pRawData->Time = (LONGLONG)(*(DWORD*)pData);
        }
        else
        {
            fSuccess = FALSE;
        }
        break;

    case PERF_LARGE_RAW_FRACTION:
        pRawData->Data = *(UNALIGNED ULONGLONG*)pData;
        pBaseCounter = pCounter+1;
        if ((pBaseCounter->CounterType & PERF_COUNTER_BASE) == PERF_COUNTER_BASE)
        {
            pData = (PVOID)((LPBYTE)pCounterDataBlock + pBaseCounter->CounterOffset);
            pRawData->Time = *(LONGLONG*)pData;
        }
        else
        {
            fSuccess = FALSE;
        }
        break;

    case PERF_PRECISION_SYSTEM_TIMER:
    case PERF_PRECISION_100NS_TIMER:
    case PERF_PRECISION_OBJECT_TIMER:
        pRawData->Data = *(UNALIGNED ULONGLONG*)pData;
        pBaseCounter = pCounter+1;
        if ((pBaseCounter->CounterType & PERF_COUNTER_BASE) == PERF_COUNTER_BASE)
        {
            pData = (PVOID)((LPBYTE)pCounterDataBlock + pBaseCounter->CounterOffset);
            pRawData->Time = *(LONGLONG*)pData;
        }
        else
        {
            fSuccess = FALSE;
        }
        break;

    case PERF_AVERAGE_TIMER:
    case PERF_AVERAGE_BULK:
        pRawData->Data = *(UNALIGNED ULONGLONG*)pData;
        pBaseCounter = pCounter+1;
        if ((pBaseCounter->CounterType & PERF_COUNTER_BASE) == PERF_COUNTER_BASE)
        {
            pData = (PVOID)((LPBYTE)pCounterDataBlock + pBaseCounter->CounterOffset);
            pRawData->Time = *(DWORD*)pData;
        }
        else
        {
            fSuccess = FALSE;
        }

        if (pCounter->CounterType == PERF_AVERAGE_TIMER)
        {
            pRawData->Frequency = ((PERF_DATA_BLOCK*)pPerfData)->PerfFreq.QuadPart;
        }
        break;

        //These are base counters and are used in calculations for other counters.
        //This case should never be entered.
    case PERF_SAMPLE_BASE:
    case PERF_AVERAGE_BASE:
    case PERF_COUNTER_MULTI_BASE:
    case PERF_RAW_BASE:
    case PERF_LARGE_RAW_BASE:
        pRawData->Data = 0;
        pRawData->Time = 0;
        fSuccess = FALSE;
        break;

    case PERF_ELAPSED_TIME:
        pRawData->Data = *(UNALIGNED ULONGLONG*)pData;
        pRawData->Time = pObject->PerfTime.QuadPart;
        pRawData->Frequency = pObject->PerfFreq.QuadPart;
        break;

        //These counters are currently not supported.
    case PERF_COUNTER_TEXT:
    case PERF_COUNTER_NODATA:
    case PERF_COUNTER_HISTOGRAM_TYPE:
        pRawData->Data = 0;
        pRawData->Time = 0;
        fSuccess = FALSE;
        break;

        //Encountered an unidentified counter.
    default:
        pRawData->Data = 0;
        pRawData->Time = 0;
        fSuccess = FALSE;
        break;
    }

    return fSuccess;
}

VOID DisplayCounterData(PPERF_DATA_BLOCK pPerf,LPTSTR *pNames)
{
    GRS_USEPRINTF();
    DWORD dwThisObject = 0;
    PPERF_OBJECT_TYPE pThisObject = NULL;

    DWORD dwThisCounter = 0;
    PPERF_COUNTER_DEFINITION pThisCounter = NULL;

    LONG  lThisInstance = 0;
    PPERF_INSTANCE_DEFINITION pThisInstance = NULL;

    PERF_COUNTER_BLOCK* pBlock = NULL;
    RAW_DATA CounterData = {};

    for(dwThisObject = 0,pThisObject = FirstObject(pPerf)
        ;dwThisObject < pPerf->NumObjectTypes;
        dwThisObject++,pThisObject=NextObject(pThisObject))
    {
        GRS_PRINTF(_T("\n\nThe Object\"%ws\","),pNames[pThisObject->ObjectNameTitleIndex]);

        if(pThisObject->NumInstances != PERF_NO_INSTANCES)
        {
            GRS_PRINTF(_T("which has the following instances:"));

            for(lThisInstance = 0,pThisInstance = FirstInstance(pThisObject);
                lThisInstance < pThisObject->NumInstances;
                lThisInstance++,pThisInstance = NextInstance(pThisInstance))
            {
                GRS_PRINTF(_T("\n\t\"%ws\""),(LPWSTR)((PBYTE)pThisInstance + pThisInstance->NameOffset));
            }
            GRS_PRINTF(_T("\n"));
        }
        else
        {
            GRS_PRINTF(_T(" which has only one occurance."));
        }

        GRS_PRINTF(_T("\nand has the following counter:"));
        for(dwThisCounter=0,pThisCounter = FirstCounter(pThisObject)
            ; dwThisCounter < pThisObject->NumCounters
            ; dwThisCounter++,pThisCounter = NextCounter(pThisCounter))
        {
            GRS_PRINTF(_T("\n\t\"%ws\""),pNames[pThisCounter->CounterNameTitleIndex]);

            if (0 == pThisObject->NumInstances || PERF_NO_INSTANCES == pThisObject->NumInstances) 
            {                                
                pBlock = (PERF_COUNTER_BLOCK*)((LPBYTE)pThisObject + pThisObject->DefinitionLength);
            }
            else if (pThisObject->NumInstances)  //Find the instance. The block follows the instance
            {                                                 //structure and instance name.

                pBlock = (PERF_COUNTER_BLOCK*)((LPBYTE)pThisInstance + pThisInstance->ByteLength);

            }
            ZeroMemory(&CounterData,sizeof(RAW_DATA));
            if(GetValue((LPBYTE)pPerf,pThisObject,pThisCounter,pBlock,&CounterData))
            {
                GRS_PRINTF(_T(":\t%llu"),CounterData.Data);
            }
        }
    }

    GRS_PRINTF(_T("\n"));

}




int _tmain(int argc,TCHAR* argv[])
{
    GRS_USEPRINTF();
    LPTSTR* lpCounterText = NULL;
    LPTSTR  lpCommandLine = NULL;
    LPTSTR  lpValueString = NULL;

    DWORD dwLastElement = 0;

    PPERF_DATA_BLOCK pDataBlock = NULL;

    DWORD dwStatus = 0;

    DWORD dwAction = COMMAND_ERROR;

    lpCounterText = BuildNameTable(HKEY_LOCAL_MACHINE,DefaultLangId,&dwLastElement);
    if(!lpCounterText)
    {
        GRS_PRINTF(_T("ERROR: Unable to read counter names from registry."));
        _tsystem(_T("PAUSE"));
        return 0;
    }

    //lpCommandLine = GetCommandLine();
    //dwAction = ProcessCommandLine(lpCommandLine,lpCounterText,dwLastElement,&lpValueString);

    if(argc < 2)
    {
        dwAction = COMMAND_ERROR;
    }
    
    if(0 == lstrcmpi(argv[1],_T("-?")))
    {
        lpValueString = Global;
        dwAction = COMMAND_HELP;
    }
    else
    {
        
        for(DWORD dwElem = 0;dwElem <= dwLastElement; dwElem++)
        {
            if((lstrcmpi(lpCounterText[dwElem],argv[1])) == 0 )
            {
                _stprintf((LPTSTR)&cValueName[0],_T("%d"),dwElem);
                lpValueString = (LPTSTR)&cValueName[0];
                dwAction = COMMAND_COUNTER;
                break;
            }
        }
    }

    if(dwAction == COMMAND_ERROR)
    {
        DisplayCommandHelp();
        GRS_SAFEFREE(lpCounterText);
        _tsystem(_T("PAUSE"));
        return -1;
    }
    else
    {
        dwStatus = GetSystemPerfData(HKEY_PERFORMANCE_DATA,lpValueString,&pDataBlock);
        if(ERROR_SUCCESS != dwStatus)
        {
            GRS_PRINTF(_T("\nERROR: Unable to obtain data for that counter"));
            GRS_SAFEFREE(lpCounterText);
            _tsystem(_T("PAUSE"));
            return -1;
        }
        else
        {
            if(COMMAND_HELP == dwAction)
            {
                DisplayObjectNames(pDataBlock,lpCounterText);
            }
            else
            {
                DisplayCounterData(pDataBlock,lpCounterText);
            }
        }
        GRS_SAFEFREE(lpCounterText);
        _tsystem(_T("PAUSE"));
        return 0;
    }
    _tsystem(_T("PAUSE"));
    return 0;
}

