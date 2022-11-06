#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <aclapi.h>

#define GRS_USEPRINTF() TCHAR pConsoleOutBuf[1024] = {}
#define GRS_PRINTF(...) \
    StringCchPrintf(pConsoleOutBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pConsoleOutBuf,lstrlen(pConsoleOutBuf),NULL,NULL);

TCHAR szCommand[10] = {};
TCHAR szSvcName[80] = {};

SC_HANDLE schSCManager = NULL;
SC_HANDLE schService = NULL;

VOID DisplayUsage(void);

VOID DoStartSvc(void);
VOID DoUpdateSvcDacl(void);
VOID DoStopSvc(void);

BOOL StopDependentServices(void);

void _tmain(int argc, TCHAR *argv[])
{
    GRS_USEPRINTF();

    GRS_PRINTF(_T("\n"));
    
    if( argc != 3 )
    {
        GRS_PRINTF(_T("ERROR: Incorrect number of arguments\n\n"));
        DisplayUsage();
        return;
    }

    StringCchCopy(szCommand, 10, argv[1]);
    StringCchCopy(szSvcName, 80, argv[2]);

    if (lstrcmpi( szCommand, _T("start")) == 0 )
    {
        DoStartSvc();
    }
    else if (lstrcmpi( szCommand, _T("dacl")) == 0 )
    {
        DoUpdateSvcDacl();
    }
    else if (lstrcmpi( szCommand, _T("stop")) == 0 )
        DoStopSvc();
    else 
    {
        _tprintf(_T("Unknown command (%s)\n\n"), szCommand);
        DisplayUsage();
    }
}

VOID DisplayUsage()
{
    GRS_USEPRINTF();

    GRS_PRINTF(_T("Description:\n"));
    GRS_PRINTF(_T("\tCommand-line tool that controls a service.\n\n"));
    GRS_PRINTF(_T("Usage:\n"));
    GRS_PRINTF(_T("\tservicecontrol [command] [service_name]\n\n"));
    GRS_PRINTF(_T("\t[command]\n"));
    GRS_PRINTF(_T("\t  start\n"));
    GRS_PRINTF(_T("\t  dacl\n"));
    GRS_PRINTF(_T("\t  stop\n"));
}

VOID DoStartSvc()
{
    GRS_USEPRINTF();

    SERVICE_STATUS_PROCESS ssStatus; 
    DWORD dwOldCheckPoint; 
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwBytesNeeded;

    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // servicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager) 
    {
        GRS_PRINTF(_T("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }

    // Get a handle to the service.

    schService = OpenService( 
        schSCManager,         // SCM database 
        szSvcName,            // name of service 
        SERVICE_ALL_ACCESS);  // full access 

    if (schService == NULL)
    { 
        GRS_PRINTF(_T("OpenService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }    

    // Check the status in case the service is not stopped. 

    if (!QueryServiceStatusEx( 
        schService,                     // handle to service 
        SC_STATUS_PROCESS_INFO,         // information level
        (LPBYTE) &ssStatus,             // address of structure
        sizeof(SERVICE_STATUS_PROCESS), // size of structure
        &dwBytesNeeded ) )              // size needed if buffer is too small
    {
        GRS_PRINTF(_T("QueryServiceStatusEx failed (%d)\n"), GetLastError());
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
        return; 
    }

    // Check if the service is already running. It would be possible 
    // to stop the service here, but for simplicity this example just returns. 

    if(ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
    {
        GRS_PRINTF(_T("Cannot start the service because it is already running\n"));
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
        return; 
    }

    // Wait for the service to stop before attempting to start it.

    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        // Save the tick count and initial checkpoint.

        dwStartTickCount = GetTickCount();
        dwOldCheckPoint = ssStatus.dwCheckPoint;

        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth of the wait hint but not less than 1 second  
        // and not more than 10 seconds. 

        dwWaitTime = ssStatus.dwWaitHint / 10;

        if( dwWaitTime < 1000 )
            dwWaitTime = 1000;
        else if ( dwWaitTime > 10000 )
            dwWaitTime = 10000;

        Sleep( dwWaitTime );

        // Check the status until the service is no longer stop pending. 

        if (!QueryServiceStatusEx( 
            schService,                     // handle to service 
            SC_STATUS_PROCESS_INFO,         // information level
            (LPBYTE) &ssStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded ) )              // size needed if buffer is too small
        {
            GRS_PRINTF(_T("QueryServiceStatusEx failed (%d)\n"), GetLastError());
            CloseServiceHandle(schService); 
            CloseServiceHandle(schSCManager);
            return; 
        }

        if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
        {
            // Continue to wait and check.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
            {
                GRS_PRINTF(_T("Timeout waiting for service to stop\n"));
                CloseServiceHandle(schService); 
                CloseServiceHandle(schSCManager);
                return; 
            }
        }
    }

    if (!StartService(
        schService,  // handle to service 
        0,           // number of arguments 
        NULL) )      // no arguments 
    {
        GRS_PRINTF(_T("StartService failed (%d)\n"), GetLastError());
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
        return; 
    }
    else 
    {
        GRS_PRINTF(_T("Service start pending...\n"));
    }

    // Check the status until the service is no longer start pending. 

    if (!QueryServiceStatusEx( 
        schService,                     // handle to service 
        SC_STATUS_PROCESS_INFO,         // info level
        (LPBYTE) &ssStatus,             // address of structure
        sizeof(SERVICE_STATUS_PROCESS), // size of structure
        &dwBytesNeeded ) )              // if buffer too small
    {
        GRS_PRINTF(_T("QueryServiceStatusEx failed (%d)\n"), GetLastError());
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
        return; 
    }

    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING) 
    { 
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if( dwWaitTime < 1000 )
        {
            dwWaitTime = 1000;
        }
        else if ( dwWaitTime > 10000 )
        {
            dwWaitTime = 10000;
        }

        Sleep( dwWaitTime );

        if (!QueryServiceStatusEx( 
            schService,             // handle to service 
            SC_STATUS_PROCESS_INFO, // info level
            (LPBYTE) &ssStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded ) )              // if buffer too small
        {
            GRS_PRINTF(_T("QueryServiceStatusEx failed (%d)\n"), GetLastError());
            break; 
        }

        if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
        {
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
            {
                // No progress made within the wait hint.
                break;
            }
        }
    } 

    // Determine whether the service is running.

    if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
    {
        GRS_PRINTF(_T("Service started successfully.\n")); 
    }
    else 
    { 
        GRS_PRINTF(_T("Service not started. \n"));
        GRS_PRINTF(_T("  Current State: %d\n"), ssStatus.dwCurrentState); 
        GRS_PRINTF(_T("  Exit Code: %d\n"), ssStatus.dwWin32ExitCode); 
        GRS_PRINTF(_T("  Check Point: %d\n"), ssStatus.dwCheckPoint); 
        GRS_PRINTF(_T("  Wait Hint: %d\n"), ssStatus.dwWaitHint); 
    } 

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

VOID DoUpdateSvcDacl()
{
    GRS_USEPRINTF();

    EXPLICIT_ACCESS      ea;
    SECURITY_DESCRIPTOR  sd;
    PSECURITY_DESCRIPTOR psd            = NULL;
    PACL                 pacl           = NULL;
    PACL                 pNewAcl        = NULL;
    BOOL                 bDaclPresent   = FALSE;
    BOOL                 bDaclDefaulted = FALSE;
    DWORD                dwError        = 0;
    DWORD                dwSize         = 0;
    DWORD                dwBytesNeeded  = 0;

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

    // Get a handle to the service

    schService = OpenService( 
        schSCManager,              // SCManager database 
        szSvcName,                 // name of service 
        READ_CONTROL | WRITE_DAC); // access

    if (schService == NULL)
    { 
        GRS_PRINTF(_T("OpenService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }    

    // Get the current security descriptor.

    if (!QueryServiceObjectSecurity(schService,
        DACL_SECURITY_INFORMATION, 
        &psd,           // using NULL does not work on all versions
        0, 
        &dwBytesNeeded))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            dwSize = dwBytesNeeded;
            psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY, dwSize);
            if (psd == NULL)
            {
                // Note: HeapAlloc does not support GetLastError.
                GRS_PRINTF(_T("HeapAlloc failed\n"));
                goto dacl_cleanup;
            }

            if (!QueryServiceObjectSecurity(schService,
                DACL_SECURITY_INFORMATION, psd, dwSize, &dwBytesNeeded))
            {
                GRS_PRINTF(_T("QueryServiceObjectSecurity failed (%d)\n"), GetLastError());
                goto dacl_cleanup;
            }
        }
        else 
        {
            GRS_PRINTF(_T("QueryServiceObjectSecurity failed (%d)\n"), GetLastError());
            goto dacl_cleanup;
        }
    }

    if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl,
        &bDaclDefaulted))
    {
        GRS_PRINTF(_T("GetSecurityDescriptorDacl failed(%d)\n"), GetLastError());
        goto dacl_cleanup;
    }

    // Build the ACE.
    BuildExplicitAccessWithName(&ea, _T("GUEST"),
        SERVICE_START | SERVICE_STOP | READ_CONTROL | DELETE,
        SET_ACCESS, NO_INHERITANCE);

    dwError = SetEntriesInAcl(1, &ea, pacl, &pNewAcl);
    if (dwError != ERROR_SUCCESS)
    {
        GRS_PRINTF(_T("SetEntriesInAcl failed(%d)\n"), dwError);
        goto dacl_cleanup;
    }

    // Initialize a new security descriptor.
    if (!InitializeSecurityDescriptor(&sd, 
        SECURITY_DESCRIPTOR_REVISION))
    {
        GRS_PRINTF(_T("InitializeSecurityDescriptor failed(%d)\n"), GetLastError());
        goto dacl_cleanup;
    }

    // Set the new DACL in the security descriptor.

    if (!SetSecurityDescriptorDacl(&sd, TRUE, pNewAcl, FALSE))
    {
        GRS_PRINTF(_T("SetSecurityDescriptorDacl failed(%d)\n"), GetLastError());
        goto dacl_cleanup;
    }

    // Set the new DACL for the service object.
    if (!SetServiceObjectSecurity(schService, 
        DACL_SECURITY_INFORMATION, &sd))
    {
        GRS_PRINTF(_T("SetServiceObjectSecurity failed(%d)\n"), GetLastError());
        goto dacl_cleanup;
    }
    else 
    {
        GRS_PRINTF(_T("Service DACL updated successfully\n"));
    }

dacl_cleanup:
    CloseServiceHandle(schSCManager);
    CloseServiceHandle(schService);

    if(NULL != pNewAcl)
    {
        LocalFree((HLOCAL)pNewAcl);
    }
    if(NULL != psd)
    {
        HeapFree(GetProcessHeap(), 0, (LPVOID)psd);
    }
}

VOID DoStopSvc()
{
    GRS_USEPRINTF();

    SERVICE_STATUS_PROCESS ssp;
    DWORD dwStartTime = GetTickCount();
    DWORD dwBytesNeeded;
    DWORD dwTimeout = 30000; // 30-second time-out

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
        schSCManager,         // SCM database 
        szSvcName,            // name of service 
        SERVICE_STOP | 
        SERVICE_QUERY_STATUS | 
        SERVICE_ENUMERATE_DEPENDENTS);  

    if (schService == NULL)
    { 
        GRS_PRINTF(_T("OpenService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }    

    // Make sure the service is not already stopped.

    if ( !QueryServiceStatusEx( 
        schService, 
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&ssp, 
        sizeof(SERVICE_STATUS_PROCESS),
        &dwBytesNeeded ) )
    {
        GRS_PRINTF(_T("QueryServiceStatusEx failed (%d)\n"), GetLastError()); 
        goto stop_cleanup;
    }

    if ( ssp.dwCurrentState == SERVICE_STOPPED )
    {
        GRS_PRINTF(_T("Service is already stopped.\n"));
        goto stop_cleanup;
    }

    // If a stop is pending, wait for it.

    while ( ssp.dwCurrentState == SERVICE_STOP_PENDING ) 
    {
        GRS_PRINTF(_T("Service stop pending...\n"));

        Sleep( ssp.dwWaitHint );
        
        if ( !QueryServiceStatusEx( 
            schService, 
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp, 
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded ) )
        {
            GRS_PRINTF(_T("QueryServiceStatusEx failed (%d)\n"), GetLastError()); 
            goto stop_cleanup;
        }

        if ( ssp.dwCurrentState == SERVICE_STOPPED )
        {
            GRS_PRINTF(_T("Service stopped successfully.\n"));
            goto stop_cleanup;
        }

        if ( GetTickCount() - dwStartTime > dwTimeout )
        {
            GRS_PRINTF(_T("Service stop timed out.\n"));
            goto stop_cleanup;
        }
    }

    // If the service is running, dependencies must be stopped first.

    StopDependentServices();

    // Send a stop code to the service.

    if ( !ControlService( 
        schService, 
        SERVICE_CONTROL_STOP, 
        (LPSERVICE_STATUS) &ssp ) )
    {
        GRS_PRINTF( _T("ControlService failed (%d)\n"), GetLastError() );
        goto stop_cleanup;
    }

    // Wait for the service to stop.

    while ( ssp.dwCurrentState != SERVICE_STOPPED ) 
    {
        Sleep( ssp.dwWaitHint );
        if ( !QueryServiceStatusEx( 
            schService, 
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp, 
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded ) )
        {
            GRS_PRINTF( _T("QueryServiceStatusEx failed (%d)\n"), GetLastError() );
            goto stop_cleanup;
        }

        if ( ssp.dwCurrentState == SERVICE_STOPPED )
            break;

        if ( GetTickCount() - dwStartTime > dwTimeout )
        {
            GRS_PRINTF( _T("Wait timed out\n" ));
            goto stop_cleanup;
        }
    }
    GRS_PRINTF(_T("Service stopped successfully\n"));

stop_cleanup:
    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

BOOL StopDependentServices()
{
    GRS_USEPRINTF();

    DWORD i;
    DWORD dwBytesNeeded;
    DWORD dwCount;

    LPENUM_SERVICE_STATUS   lpDependencies = NULL;
    ENUM_SERVICE_STATUS     ess;
    SC_HANDLE               hDepService;
    SERVICE_STATUS_PROCESS  ssp;

    DWORD dwStartTime = GetTickCount();
    DWORD dwTimeout = 30000; // 30-second time-out

    // Pass a zero-length buffer to get the required buffer size.
    if ( EnumDependentServices( schService, SERVICE_ACTIVE, 
        lpDependencies, 0, &dwBytesNeeded, &dwCount ) ) 
    {
        // If the Enum call succeeds, then there are no dependent
        // services, so do nothing.
        return TRUE;
    } 
    else 
    {
        if ( GetLastError() != ERROR_MORE_DATA )
        {
            return FALSE; // Unexpected error
        }

        // Allocate a buffer for the dependencies.
        lpDependencies = (LPENUM_SERVICE_STATUS) HeapAlloc( 
            GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded );

        if ( !lpDependencies )
        {
            return FALSE;
        }

        __try 
        {
            // Enumerate the dependencies.
            if ( !EnumDependentServices( schService, SERVICE_ACTIVE, 
                lpDependencies, dwBytesNeeded, &dwBytesNeeded,
                &dwCount ) )
                return FALSE;

            for ( i = 0; i < dwCount; i++ ) 
            {
                ess = *(lpDependencies + i);
                // Open the service.
                hDepService = OpenService( schSCManager, 
                    ess.lpServiceName, 
                    SERVICE_STOP | SERVICE_QUERY_STATUS );

                if ( !hDepService )
                    return FALSE;

                __try 
                {
                    // Send a stop code.
                    if ( !ControlService( hDepService, 
                        SERVICE_CONTROL_STOP,
                        (LPSERVICE_STATUS) &ssp ) )
                        return FALSE;

                    // Wait for the service to stop.
                    while ( ssp.dwCurrentState != SERVICE_STOPPED ) 
                    {
                        Sleep( ssp.dwWaitHint );
                        if ( !QueryServiceStatusEx( 
                            hDepService, 
                            SC_STATUS_PROCESS_INFO,
                            (LPBYTE)&ssp, 
                            sizeof(SERVICE_STATUS_PROCESS),
                            &dwBytesNeeded ) )
                            return FALSE;

                        if ( ssp.dwCurrentState == SERVICE_STOPPED )
                            break;

                        if ( GetTickCount() - dwStartTime > dwTimeout )
                            return FALSE;
                    }
                } 
                __finally 
                {
                    // Always release the service handle.
                    CloseServiceHandle( hDepService );
                }
            }
        } 
        __finally 
        {
            // Always free the enumeration buffer.
            HeapFree( GetProcessHeap(), 0, lpDependencies );
        }
    } 
    return TRUE;
}

