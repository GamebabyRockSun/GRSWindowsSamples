#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <pdh.h>
#include <pdhmsg.h>

#pragma comment(lib, "pdh.lib")

#define SAMPLE_INTERVAL_MS  1000

long __cdecl _tmain(int argc, TCHAR **argv)
{

    HQUERY          hQuery;
    HCOUNTER        hCounter;
    PDH_STATUS      pdhStatus;
    PDH_FMT_COUNTERVALUE   fmtValue;
    DWORD           ctrType;
    SYSTEMTIME      stSampleTime;
    PDH_BROWSE_DLG_CONFIG  BrowseDlgData = {};
    TCHAR            szPathBuffer[PDH_MAX_COUNTER_PATH] = {};

    // Create a query.
    pdhStatus = PdhOpenQuery(NULL, NULL, &hQuery);
    if (ERROR_SUCCESS != pdhStatus)
    {
        _tprintf(TEXT("PdhOpenQuery failed with %ld.\n"), pdhStatus);
        goto cleanup;
    }

    ZeroMemory(&szPathBuffer, sizeof(szPathBuffer));

    // Initialize the browser dialog window settings.
    ZeroMemory(&BrowseDlgData, sizeof(PDH_BROWSE_DLG_CONFIG));
    BrowseDlgData.bIncludeInstanceIndex = FALSE;   
    BrowseDlgData.bSingleCounterPerAdd = TRUE;
    BrowseDlgData.bSingleCounterPerDialog = TRUE;  
    BrowseDlgData.bLocalCountersOnly = FALSE;      
    BrowseDlgData.bWildCardInstances = TRUE;
    BrowseDlgData.bHideDetailBox = TRUE;
    BrowseDlgData.bInitializePath = FALSE;     
    BrowseDlgData.bDisableMachineSelection = FALSE;
    BrowseDlgData.bIncludeCostlyObjects = FALSE;
    BrowseDlgData.bShowObjectBrowser = FALSE;
    BrowseDlgData.hWndOwner = NULL;   
    BrowseDlgData.szReturnPathBuffer = szPathBuffer;
    BrowseDlgData.cchReturnPathLength = PDH_MAX_COUNTER_PATH;
    BrowseDlgData.pCallBack = NULL;   
    BrowseDlgData.dwCallBackArg = 0;
    BrowseDlgData.CallBackStatus = ERROR_SUCCESS;
    BrowseDlgData.dwDefaultDetailLevel = PERF_DETAIL_WIZARD;
    BrowseDlgData.szDialogBoxCaption = TEXT("Select a counter to monitor.");

    // Display the counter browser window. The dialog is configured
    // to return a single selection from the counter list.
    pdhStatus = PdhBrowseCounters (&BrowseDlgData);
    if (ERROR_SUCCESS != pdhStatus)
    {
        if (PDH_DIALOG_CANCELLED != pdhStatus)
        {
            _tprintf(TEXT("PdhBrowseCounters failed with 0x%x.\n"), pdhStatus);
        }

        goto cleanup;
    }

    // Add the selected counter to the query.
    pdhStatus = PdhAddCounter (hQuery,
        szPathBuffer, 
        0, 
        &hCounter);
    if (ERROR_SUCCESS != pdhStatus)
    {
        _tprintf(TEXT("PdhBrowseCounters failed with 0x%x.\n"), pdhStatus);
        goto cleanup;
    }

    // Most counters require two sample values to display a formatted value.
    // PDH stores the current sample value and the previously collected
    // sample value. This call retrieves the first value that will be used
    // by PdhGetFormattedCounterValue in the first iteration of the loop
    // Note that this value is lost if the counter does not require two
    // values to compute a displayable value.
    pdhStatus = PdhCollectQueryData (hQuery);
    if (ERROR_SUCCESS != pdhStatus)
    {
        _tprintf(TEXT("PdhCollectQueryData failed with 0x%x.\n"), pdhStatus);
        goto cleanup;
    }

    // Print counter values until a key is pressed.
    while (!_kbhit()) 
    {

        // Wait one interval.
        Sleep(SAMPLE_INTERVAL_MS);

        // Get the sample time.
        GetLocalTime (&stSampleTime);

        // Get the current data value.
        pdhStatus = PdhCollectQueryData (hQuery);

        // Print the time stamp for the sample.
        _tprintf (
			TEXT("\n%s:\"%2.2d/%2.2d/%4.4d %2.2d:%2.2d:%2.2d.%3.3d\""),
			szPathBuffer,
            stSampleTime.wMonth, 
            stSampleTime.wDay, 
            stSampleTime.wYear,
            stSampleTime.wHour, 
            stSampleTime.wMinute, 
            stSampleTime.wSecond,
            stSampleTime.wMilliseconds);

        // Compute a displayable value for the counter.
        pdhStatus = PdhGetFormattedCounterValue (hCounter,
            PDH_FMT_DOUBLE,
            &ctrType,
            &fmtValue);

        if (pdhStatus == ERROR_SUCCESS)
        {
            _tprintf (TEXT(",\"%.20g\""), fmtValue.doubleValue);
        }
        else
        {
            _tprintf(TEXT("\nPdhGetFormattedCounterValue failed with 0x%x.\n"), pdhStatus);
            goto cleanup;
        }
    }

cleanup:
    // Close the query.
    if (hQuery)
        PdhCloseQuery (hQuery);

	_tsystem(_T("PAUSE"));

    return pdhStatus;
}
