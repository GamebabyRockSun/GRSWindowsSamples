#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pConsoleOutBuf[1024] = {}
#define GRS_PRINTF(...) \
    StringCchPrintf(pConsoleOutBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pConsoleOutBuf,lstrlen(pConsoleOutBuf),NULL,NULL);

TCHAR szCommand[10] = {};
TCHAR szSvcName[80] = {};

VOID DisplayUsage(void);

VOID DoQuerySvc(void);
VOID DoUpdateSvcDesc(void);
VOID DoAutoSvc(void);
VOID DoDisableSvc(void);
VOID DoEnableSvc(void);
VOID DoDeleteSvc(void);


void __cdecl _tmain(int argc, TCHAR *argv[])
{
    GRS_USEPRINTF();

    GRS_PRINTF(_T("\n"));
    
    if( argc != 3 )
    {
        GRS_PRINTF(_T("ERROR:\tIncorrect number of arguments\n\n"));
        DisplayUsage();
        return;
    }

    StringCchCopy(szCommand, 10, argv[1]);
    StringCchCopy(szSvcName, 80, argv[2]);

    if (lstrcmpi( szCommand, _T("query")) == 0 )
    {
        DoQuerySvc();
    }
    else if (lstrcmpi( szCommand, _T("describe")) == 0 )
    {
        DoUpdateSvcDesc();
    }
    else if (lstrcmpi( szCommand, _T("disable")) == 0 )
    {
        DoDisableSvc();
    }
    else if (lstrcmpi( szCommand, _T("enable")) == 0 )
    {
        DoEnableSvc();
    }
    else if (lstrcmpi( szCommand, _T("auto")) == 0 )
    {
        DoAutoSvc();
    }
    else if (lstrcmpi( szCommand, _T("delete")) == 0 )
    {
        DoDeleteSvc();
    }
    else 
    {
        GRS_PRINTF(_T("Unknown command (%s)\n\n"), szCommand);
        DisplayUsage();
    }
}

VOID DisplayUsage()
{
    GRS_USEPRINTF();

    GRS_PRINTF(_T("Description:\n"));
    GRS_PRINTF(_T("\tCommand-line tool that configures a service.\n\n"));
    GRS_PRINTF(_T("Usage:\n"));
    GRS_PRINTF(_T("\tserviceconfig [command] [service_name]\n\n"));
    GRS_PRINTF(_T("\t[command]\n"));
    GRS_PRINTF(_T("\t  query\n"));
    GRS_PRINTF(_T("\t  describe\n"));
    GRS_PRINTF(_T("\t  disable\n"));
    GRS_PRINTF(_T("\t  enable\n"));
    GRS_PRINTF(_T("\t  auto\n"));
    GRS_PRINTF(_T("\t  delete\n"));
}

VOID DoQuerySvc()
{
    GRS_USEPRINTF();

    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    LPQUERY_SERVICE_CONFIG lpsc; 
    LPSERVICE_DESCRIPTION lpsd;
    DWORD dwBytesNeeded, cbBufSize, dwError; 

    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager) 
    {
        GRS_PRINTF(_T("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }

    schService = OpenService( 
        schSCManager,          // SCM database 
        szSvcName,             // name of service 
        SERVICE_QUERY_CONFIG); // need query config access 

    if (schService == NULL)
    { 
        GRS_PRINTF(_T("OpenService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }

    if( !QueryServiceConfig( 
        schService, 
        NULL, 
        0, 
        &dwBytesNeeded))
    {
        dwError = GetLastError();
        if( ERROR_INSUFFICIENT_BUFFER == dwError )
        {
            cbBufSize = dwBytesNeeded;
            lpsc = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LMEM_FIXED, cbBufSize);
        }
        else
        {
            GRS_PRINTF(_T("QueryServiceConfig failed (%d)"), dwError);
            goto cleanup; 
        }
    }

    if( !QueryServiceConfig( 
        schService, 
        lpsc, 
        cbBufSize, 
        &dwBytesNeeded) ) 
    {
        GRS_PRINTF(_T("QueryServiceConfig failed (%d)"), GetLastError());
        goto cleanup;
    }

    if( !QueryServiceConfig2( 
        schService, 
        SERVICE_CONFIG_DESCRIPTION,
        NULL, 
        0, 
        &dwBytesNeeded))
    {
        dwError = GetLastError();
        if( ERROR_INSUFFICIENT_BUFFER == dwError )
        {
            cbBufSize = dwBytesNeeded;
            lpsd = (LPSERVICE_DESCRIPTION) LocalAlloc(LMEM_FIXED, cbBufSize);
        }
        else
        {
            GRS_PRINTF(_T("QueryServiceConfig2 failed (%d)"), dwError);
            goto cleanup; 
        }
    }

    if (! QueryServiceConfig2( 
        schService, 
        SERVICE_CONFIG_DESCRIPTION,
        (LPBYTE) lpsd, 
        cbBufSize, 
        &dwBytesNeeded) ) 
    {
        GRS_PRINTF(_T("QueryServiceConfig2 failed (%d)"), GetLastError());
        goto cleanup;
    }

    GRS_PRINTF(_T("%s configuration: \n"), szSvcName);
    GRS_PRINTF(_T("  Type: 0x%x\n"), lpsc->dwServiceType);
    GRS_PRINTF(_T("  Start Type: 0x%x\n"), lpsc->dwStartType);
    GRS_PRINTF(_T("  Error Control: 0x%x\n"), lpsc->dwErrorControl);
    GRS_PRINTF(_T("  Binary path: %s\n"), lpsc->lpBinaryPathName);
    GRS_PRINTF(_T("  Account: %s\n"), lpsc->lpServiceStartName);

    if (lpsd->lpDescription != NULL && lstrcmp(lpsd->lpDescription, _T("")) != 0)
    {
        GRS_PRINTF(_T("  Description: %s\n"), lpsd->lpDescription);
    }
    if (lpsc->lpLoadOrderGroup != NULL && lstrcmp(lpsc->lpLoadOrderGroup, _T("")) != 0)
    {
        GRS_PRINTF(_T("  Load order group: %s\n"), lpsc->lpLoadOrderGroup);
    }
    if (lpsc->dwTagId != 0)
    {
        GRS_PRINTF(_T("  Tag ID: %d\n"), lpsc->dwTagId);
    }
    if (lpsc->lpDependencies != NULL && lstrcmp(lpsc->lpDependencies, _T("")) != 0)
    {
        GRS_PRINTF(_T("  Dependencies: %s\n"), lpsc->lpDependencies);
    }

    LocalFree(lpsc); 
    LocalFree(lpsd);

cleanup:
    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

VOID DoDisableSvc()
{
    GRS_USEPRINTF();

    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager) 
    {
        GRS_PRINTF(_T("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }

    schService = OpenService( 
        schSCManager,            // SCM database 
        szSvcName,               // name of service 
        SERVICE_CHANGE_CONFIG);  // need change config access 

    if (schService == NULL)
    { 
        GRS_PRINTF(_T("OpenService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }    

    if (! ChangeServiceConfig( 
        schService,        // handle of service 
        SERVICE_NO_CHANGE, // service type: no change 
        SERVICE_DISABLED,  // service start type 
        SERVICE_NO_CHANGE, // error control: no change 
        NULL,              // binary path: no change 
        NULL,              // load order group: no change 
        NULL,              // tag ID: no change 
        NULL,              // dependencies: no change 
        NULL,              // account name: no change 
        NULL,              // password: no change 
        NULL) )            // display name: no change
    {
        GRS_PRINTF(_T("ChangeServiceConfig failed (%d)\n"), GetLastError()); 
    }
    else 
    {
        GRS_PRINTF(_T("Service disabled successfully.\n")); 
    }

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

VOID DoEnableSvc()
{
    GRS_USEPRINTF();

    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    // Get a handle to the SCM database. 

    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager) 
    {
        GRS_PRINTF(_T("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }

    // Get a handle to the service.

    schService = OpenService( 
        schSCManager,            // SCM database 
        szSvcName,               // name of service 
        SERVICE_CHANGE_CONFIG);  // need change config access 

    if (schService == NULL)
    { 
        GRS_PRINTF(_T("OpenService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }    

    // Change the service start type.

    if (! ChangeServiceConfig( 
        schService,            // handle of service 
        SERVICE_NO_CHANGE,     // service type: no change 
        SERVICE_DEMAND_START,  // service start type 
        SERVICE_NO_CHANGE,     // error control: no change 
        NULL,                  // binary path: no change 
        NULL,                  // load order group: no change 
        NULL,                  // tag ID: no change 
        NULL,                  // dependencies: no change 
        NULL,                  // account name: no change 
        NULL,                  // password: no change 
        NULL) )                // display name: no change
    {
        GRS_PRINTF(_T("ChangeServiceConfig failed (%d)\n"), GetLastError()); 
    }
    else
    {
        GRS_PRINTF(_T("Service enabled successfully.\n")); 
    }

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

VOID DoAutoSvc(void)
{
    GRS_USEPRINTF();

    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    // Get a handle to the SCM database. 

    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager) 
    {
        GRS_PRINTF(_T("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }

    // Get a handle to the service.

    schService = OpenService( 
        schSCManager,            // SCM database 
        szSvcName,               // name of service 
        SERVICE_CHANGE_CONFIG);  // need change config access 

    if (schService == NULL)
    { 
        GRS_PRINTF(_T("OpenService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }    

    // Change the service start type.

    if (! ChangeServiceConfig( 
        schService,            // handle of service 
        SERVICE_NO_CHANGE,     // service type: no change 
        SERVICE_AUTO_START,  // service start type 
        SERVICE_NO_CHANGE,     // error control: no change 
        NULL,                  // binary path: no change 
        NULL,                  // load order group: no change 
        NULL,                  // tag ID: no change 
        NULL,                  // dependencies: no change 
        NULL,                  // account name: no change 
        NULL,                  // password: no change 
        NULL) )                // display name: no change
    {
        GRS_PRINTF(_T("ChangeServiceConfig failed (%d)\n"), GetLastError()); 
    }
    else
    {
        GRS_PRINTF(_T("Service Set AutoRun successfully.\n")); 
    }

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

VOID DoUpdateSvcDesc()
{
    GRS_USEPRINTF();

    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    SERVICE_DESCRIPTION sd;
    LPTSTR szDesc = _T("This is a test description");

    // Get a handle to the SCM database. 

    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager) 
    {
        GRS_PRINTF(_T("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }

    // Get a handle to the service.

    schService = OpenService( 
        schSCManager,            // SCM database 
        szSvcName,               // name of service 
        SERVICE_CHANGE_CONFIG);  // need change config access 

    if (schService == NULL)
    { 
        GRS_PRINTF(_T("OpenService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }    

    // Change the service description.

    sd.lpDescription = szDesc;

    if( !ChangeServiceConfig2(
        schService,                 // handle to service
        SERVICE_CONFIG_DESCRIPTION, // change: description
        &sd) )                      // new description
    {
        GRS_PRINTF(_T("ChangeServiceConfig2 failed\n"));
    }
    else
    {
        GRS_PRINTF(_T("Service description updated successfully.\n"));
    }

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

VOID DoDeleteSvc()
{
    GRS_USEPRINTF();

    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager) 
    {
        GRS_PRINTF(_T("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }

    // Get a handle to the service.

    schService = OpenService( 
        schSCManager,       // SCM database 
        szSvcName,          // name of service 
        DELETE);            // need delete access 

    if (schService == NULL)
    { 
        GRS_PRINTF(_T("OpenService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }

    // Delete the service.

    if (! DeleteService(schService) ) 
    {
        GRS_PRINTF(_T("DeleteService failed (%d)\n"), GetLastError()); 
    }
    else
    {
        GRS_PRINTF(_T("Service deleted successfully\n")); 
    }

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}
