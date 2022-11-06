#include <windows.h>
#include <iostream>
#include <strsafe.h>

#include "../AddSource/ServiceErr.h"

#define BUFFER_SIZE 1024

__inline VOID GetAppPath(LPTSTR pszBuffer)
{
    DWORD dwLen = 0;
    if(0 == (dwLen = ::GetModuleFileName(NULL,pszBuffer,MAX_PATH)))
    {
        return;			
    }
    DWORD i = dwLen;
    for(; i > 0; i -- )
    {
        if( '\\' == pszBuffer[i] )
        {
            pszBuffer[i + 1] = '\0';
            break;
        }
    }
}

void __cdecl wmain(int argc, LPWSTR *argv)
{
    // Name of the event log.
    wchar_t *logName = L"Application";
    // Event Source name.
    wchar_t *sourceName = L"SampleEventSourceName";
    // This is the event ID that you are querying for.
    DWORD dwMessageID = SVC_ERROR;  
    // DLL that contains the event messages (descriptions).
    //消息文件(编译后为EXE,DLL等)的路径
    wchar_t dllName[MAX_PATH] = {};
    GetAppPath(dllName);
    StringCchCat(dllName,MAX_PATH,L"ServiceErr.dll");

    HANDLE h, ghResDll;
    char lpMsgBuf1[BUFFER_SIZE] = {};
    EVENTLOGRECORD *pevlr;
    BYTE bBuffer[BUFFER_SIZE] = {};
    DWORD dwRead, dwNeeded, dwThisRecord;
    LPCTSTR lpSourceName;

    // Step 1: ---------------------------------------------------------
    // Open the event log. ---------------------------------------------
    h = OpenEventLog( NULL,               // Use the local computer.
        logName);
    if (h == NULL)
    {
        std::wcout << L"Could not open the event log." << std::endl;;
        return;
    }

    // Step 2: ---------------------------------------------------------
    // Initialize the event record buffer. -----------------------------
    pevlr = (EVENTLOGRECORD *) &bBuffer;

    // Step 3: ---------------------------------------------------------
    // Load the message DLL file. --------------------------------------
    ghResDll =  LoadLibrary(dllName);

    // Step 4: ---------------------------------------------------------
    // Get the record number of the oldest event log record. -----------
    GetOldestEventLogRecord(h, &dwThisRecord);

    // Step 5: ---------------------------------------------------------
    // When the event log is opened, the position of the file pointer
    // is at the beginning of the log. Read the event log records
    // sequentially until the last record has been read.
    while (ReadEventLog(h,                // Event log handle
        EVENTLOG_FORWARDS_READ |          // Reads forward
        EVENTLOG_SEQUENTIAL_READ,         // Sequential read
        0,                                // Ignored for sequential read
        pevlr,                            // Pointer to buffer
        BUFFER_SIZE,                      // Size of buffer
        &dwRead,                          // Number of bytes read
        &dwNeeded))                       // Bytes in the next record
    {
        while (dwRead > 0)
        {
            // Get the event source name.
            lpSourceName = (LPCTSTR) ((LPBYTE) pevlr + sizeof(EVENTLOGRECORD));        

            // Print the information if the event source and the message
            // match the parameters

            if ((lstrcmp(lpSourceName,sourceName) == 0) &&
                (dwMessageID == pevlr->EventID))
            {
                // Step 6: ----------------------------------------------
                // Retrieve the message string. -------------------------
                FormatMessage(
                    FORMAT_MESSAGE_FROM_HMODULE, // Format of message
                    ghResDll,                    // Handle to the DLL file
                    pevlr->EventID,              // Event message identifier
                    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                    (LPTSTR) &lpMsgBuf1,         // Buffer that contains message
                    BUFFER_SIZE,                 // Size of buffer
                    NULL);                       // Array of insert values

                // Print the event identifier, event type, event category,
                // event source, and event message.
                std::wcout << dwThisRecord++ <<
                    L"  Event ID: " << pevlr->EventID << L" Event Type: " <<
                    std::endl;

                switch(pevlr->EventType)
                {
                case EVENTLOG_ERROR_TYPE:
                    std::wcout << L"EVENTLOG_ERROR_TYPE  " << std::endl;
                    break;
                case EVENTLOG_WARNING_TYPE:
                    std::wcout << L"EVENTLOG_WARNING_TYPE  " << std::endl;
                    break;
                case EVENTLOG_INFORMATION_TYPE:
                    std::wcout << L"EVENTLOG_INFORMATION_TYPE  " << std::endl;
                    break;
                case EVENTLOG_AUDIT_SUCCESS:
                    std::wcout << L"EVENTLOG_AUDIT_SUCCESS  " << std::endl;
                    break;
                case EVENTLOG_AUDIT_FAILURE:
                    std::wcout << L"EVENTLOG_AUDIT_FAILURE  " << std::endl;
                    break;
                default:
                    std::wcout << L"Unknown  " << std::endl;
                    break;
                }   

                std::wcout << L"  Event Category: " <<
                    pevlr->EventCategory << L" Event Source: " <<
                    lpSourceName << L" Message: " << (LPTSTR) lpMsgBuf1 <<
                    std::endl;
            }

            dwRead -= pevlr->Length;
            pevlr = (EVENTLOGRECORD *) ((LPBYTE) pevlr + pevlr->Length);
        }

        pevlr = (EVENTLOGRECORD *) &bBuffer;
    }

    // Step 7: -------------------------------------------------------------
    // Close the event log.
    CloseEventLog(h);

    _wsystem(L"PAUSE");
}            

