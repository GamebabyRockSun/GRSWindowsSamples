// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.
//
// Module Name: instlsp.cpp
//
// Description:
//
//    This sample illustrates how to develop a layered service provider that is
//    capable of counting all bytes transmitted through a TCP/IP socket.
//
//    This file contains an installation program to insert the layered sample
//    into the Winsock catalog of providers.
//    
//
// Compile:
//
//    Compile with the Makefile:
//      nmake /f Makefile
//
// Execute:
//
//    This project produces a executable file instlsp.exe. The installation app
//    allows you to install the LSP over any provider. Note however that if you
//    choose to install over a single provider, you should install over all 
//    providers of that address family (e.g. if you install over UDP, install
//    over TCP and RAW providers as well). The arguments are:
//
//       -i             Install the LSP
//       -r             Remove the LSP
//       -o  CatID      Layer over the given provider (indicated by the catalog id)
//       -a             Install over all providers
//       -p             Print the Winsock catalog (and catalog ids)
//       -l             Print the layered entries only
//       -n "String"    Name of the layered provider (catalog name, not dll name)
//       -f             Remove all layered providers (last ditch recovery)
//
//    For example, first print out the catalog:
//       instlsp.exe -p
//        1001 - MSAFD ATM AAL5
//        1002 - MSAFD Tcpip [TCP/IP]
//        1003 - MSAFD Tcpip [UDP/IP]
//        1004 - MSAFD Tcpip [RAW/IP]
//        1005 - RSVP UDP Service Provider
//        1006 - RSVP TCP Service Provider
//        1019 - MSAFD AppleTalk [ADSP]
//        1020 - MSAFD AppleTalk [ADSP] [Pseudo Stream]
//        1021 - MSAFD AppleTalk [PAP]
//        1022 - MSAFD AppleTalk [RTMP]
//        1023 - MSAFD AppleTalk [ZIP]
//
//    To install over AppleTalk
//       instlsp.exe -i -o 1019 -o 1020 -o 1021 -o 1022 -o 1023 -n "Foobar LSP"
//
//    To remove an LSP:
//       instlsp.exe -r 
//
#include <winsock2.h>
#include <ws2spi.h>

#include <rpc.h>
#include <rpcdce.h>

#include <sporder.h>
#include <winnt.h>
#include <windows.h>

#include "install.h"

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_PATH_LEN        128
#define MAX_PROVIDERS           256     

#pragma comment( lib, "Ws2_32.lib" )
#pragma comment( lib, "sporder.lib" )
#pragma comment( lib, "rpcrt4.lib")

//
// Function prototypes
//
void PrintProviders(BOOL bLayeredOnly);
int  InstallProvider(void);
void RemoveProvider(void);
void usage(char *progname);
BOOL IsIdinChain(WSAPROTOCOL_INFOW *pinfo, DWORD Id);
void RemoveAllLayeredEntries();

//
// Global variables
//
static LPWSAPROTOCOL_INFOW ProtocolInfo = NULL;
static DWORD               ProtocolInfoSize = 0;
static INT                 TotalProtocols = 0;
static DWORD               CatalogIdArray[MAX_PROVIDERS],
                           CatalogIdArrayCount = 0;       // How many to install over
static BOOL                bInstall = TRUE;
static WCHAR               wszLSPName[64];

//
// Function: main
//
// Description:
//    Parse the command line arguments and call either the install or remove
//    routine.
//
void _cdecl main(int argc, char *argv[])
{
    WSADATA     wsd;
    BOOL        bOpSpecified = FALSE;
    int         i, j;

    if (WSAStartup(MAKEWORD(2,2), &wsd) != 0)
    {
        fprintf(stderr, "Unable to load Winsock: %d\n", GetLastError());
        return;
    }

    for(i=1; i < argc ;i++)
    {
        if ( (strlen(argv[i]) != 2) &&
             ( (argv[i][0] != '-')    ||
               (argv[i][0] != '/') ) )
        {
            usage(argv[0]);
        }
        switch (tolower(argv[i][1]))
        {
            case 'i':               // install
                bOpSpecified = TRUE;
                bInstall = TRUE;
                break;
            case 'r':               // remove
                bOpSpecified = TRUE;
                bInstall = FALSE;
                break;
            case 'o':               // catalog id (to install over)
                if (i+1 >= argc)
                    usage(argv[0]);
                CatalogIdArray[CatalogIdArrayCount++] = atoi(argv[i+1]);
                i++;
                break;
            case 'p':               // print the catalog
                PrintProviders(FALSE);
                ExitProcess(-1);
                break;
            case 'l':               // print the layered providers only
                PrintProviders(TRUE);
                ExitProcess(0);
                break;
            case 'n':               // name of the LSP to install (not the DLL name)
                if (i+1 >= argc)
                    usage(argv[0]);
                MultiByteToWideChar(CP_ACP, 0, argv[i+1], strlen(argv[i+1]), wszLSPName, 64);
                i++;
                break;
            case 'a':
                ProtocolInfo = GetProviders(&TotalProtocols);
                if (!ProtocolInfo)
                {
                    printf("Unable to enumerate providers!\n");
                    ExitProcess(-1);
                }
                for(j=0; j < TotalProtocols ;j++)
                {
                    CatalogIdArray[CatalogIdArrayCount++] = ProtocolInfo[j].dwCatalogEntryId;
                }
                FreeProviders(ProtocolInfo);
                break;
            case 'f':
                RemoveAllLayeredEntries();
                ExitProcess(0);
            default:
                usage(argv[0]);
                break;
        }
    }
    if (!bOpSpecified)
        usage(argv[0]);

    if (bInstall)
    {
        printf("LSP name is '%S'\n", wszLSPName);
        InstallProvider();
    }
    else
        RemoveProvider();

    WSACleanup();
    return;
}

//
// Function: usage
//
// Description:
//    Prints usage information.
//
void usage(char *progname)
{
    printf("usage: %s -i -r [CatId] -o [CatId] -p\n", progname);
    printf("       -i       Install LSP\n"
           "       -r       Remove LSP\n"
           "       -o CatId Install over specified LSP\n"
           "                This option may be specified multiple times\n"
           "       -a       Install over all providers (base or layered)\n"
           "       -p       Print all layers and their catalog IDs\n"
           "       -l       Print layered providers only\n"
           "       -n Str   Name of LSP\n"
           "       -f       Remove all layered entries\n");
    ExitProcess(-1);
}

//
// Function: PrintProviders
//
// Description: 
//    This function prints out each entry in the Winsock catalog and its
//    catalog ID if the parameter, bLayeredOnly, is FALSE. If TRUE then
//    print only those layered catalog entries.
//
void PrintProviders(BOOL bLayeredOnly)
{

	ProtocolInfo = GetProviders(&TotalProtocols);
    if (!ProtocolInfo)
    {
        return;
    }
    for(int i=0; i < TotalProtocols ;i++)
    {
        if (!bLayeredOnly)
            printf("%04d - %S\n", ProtocolInfo[i].dwCatalogEntryId,
                                  ProtocolInfo[i].szProtocol);
        else if (ProtocolInfo[i].ProtocolChain.ChainLen == LAYERED_PROTOCOL)
            printf("%04d - %S\n", ProtocolInfo[i].dwCatalogEntryId,
                                  ProtocolInfo[i].szProtocol);
    }
    FreeProviders(ProtocolInfo);

    return;
}

//
// Function: InstallProvider
//
// Description:
//   This function installs the provider over the given catalogs.
//
int  InstallProvider(void)
{
    WSAPROTOCOL_INFOW  *OriginalProtocolInfo,
                        DummyProtocolInfo;
    GUID                ProviderChainGuid;
    WCHAR               ChainName[WSAPROTOCOL_LEN+1];
    DWORD               LayeredCatalogId,
                        OriginalCatalogId,
                       *CatalogEntries=NULL,
                        i;
    INT                 j,
                        k,
                        idx,
                        CatIndex,
                        ErrorCode;


	ProtocolInfo = GetProviders(&TotalProtocols);
    //
    // Allocate protocol info structures for each entry we want to layer over
    //
    OriginalProtocolInfo = (WSAPROTOCOL_INFOW *)HeapAlloc(
                                GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                sizeof(WSAPROTOCOL_INFOW) * CatalogIdArrayCount);
    if (!OriginalProtocolInfo)
    {
        printf("InstallProviders: HeapAlloc() failed: %d\n", GetLastError());
        FreeProviders(ProtocolInfo);
        return -1;
    }
    //
    // Go through the catalog, and save off the WSAPROTOCOL_INFOW structures for
    // those entries we're layering over.
    //
    idx = 0;
    for(i=0; i < CatalogIdArrayCount ;i++)
    {
        for(j=0; j < TotalProtocols ;j++)
        {
            // See if this is a protocol we're layering over
            //
            if (ProtocolInfo[j].dwCatalogEntryId == CatalogIdArray[i])
            {
                memcpy(&OriginalProtocolInfo[idx],
                       &ProtocolInfo[j],
                        sizeof(WSAPROTOCOL_INFOW));
                //
                // Make our provider a non-IFS handle provider
                //
                OriginalProtocolInfo[idx].dwServiceFlags1 = 
                    ProtocolInfo[j].dwServiceFlags1 & (~XP1_IFS_HANDLES); 
                idx++;

                break;
            }
        }
    }
    // Prepare our dummy (hidden) entry for our LSP
    //  Note that the data contained within this structure is ignored by Winsock.
    //
    memcpy(&DummyProtocolInfo, &OriginalProtocolInfo[0], sizeof(WSAPROTOCOL_INFOW));

    wcsncpy(DummyProtocolInfo.szProtocol, wszLSPName, WSAPROTOCOL_LEN+1);
    DummyProtocolInfo.ProtocolChain.ChainLen = LAYERED_PROTOCOL;
    DummyProtocolInfo.dwProviderFlags |= PFL_HIDDEN;
    //
    // Install the dummy entry 
    //
    if (WSCInstallProvider(&ProviderGuid, PROVIDER_PATH, &DummyProtocolInfo, 1, &ErrorCode) == SOCKET_ERROR)
    {
        printf("WSCInstallProvider() failed: %d\n", ErrorCode);
        return -1;
    }
    //
    // Get the Winsock catalog again so we can find our catalog entry ID.
    // The whole purpose to installing the dummy entry is to obtain a catalog ID
    //  which we'll use in the protocol chains of those entries we're layering over.
    //
    FreeProviders(ProtocolInfo);
    ProtocolInfo = GetProviders(&TotalProtocols);

    for(j=0; j < TotalProtocols ;j++)
    {
        if (!memcmp(&ProtocolInfo[j].ProviderId, &ProviderGuid, sizeof(GUID)))
        {
            LayeredCatalogId = ProtocolInfo[j].dwCatalogEntryId;
            break;
        }
    }
    //
    // Find each entry that we're layering over and fix the protocols chains
    //  to reference our LSP
    //
    for(i=0; i < CatalogIdArrayCount ;i++)
    {
        OriginalCatalogId = OriginalProtocolInfo[i].dwCatalogEntryId;
        //
        // Name our layered entry
        //
        swprintf(ChainName, L"%s over [%s]", wszLSPName, OriginalProtocolInfo[i].szProtocol);
        ChainName[WSAPROTOCOL_LEN] = 0;     // Make sure not to overrun the buffer
        wcsncpy(OriginalProtocolInfo[i].szProtocol, ChainName, WSAPROTOCOL_LEN+1);

        if (OriginalProtocolInfo[i].ProtocolChain.ChainLen == BASE_PROTOCOL)
        {
            // Setup a new protocol chain
            OriginalProtocolInfo[i].ProtocolChain.ChainEntries[1] = OriginalProtocolInfo[i].dwCatalogEntryId;
        }
        else
        {
            // Push protocol entries down the protocol chain
            for (k = OriginalProtocolInfo[i].ProtocolChain.ChainLen; k > 0; k--)
            {
                OriginalProtocolInfo[i].ProtocolChain.ChainEntries[k] = OriginalProtocolInfo[i].ProtocolChain.ChainEntries[k-1];
            }
        }
        // Insert our layered provider into the catalog and increment the count
        //
        OriginalProtocolInfo[i].ProtocolChain.ChainLen++;
        OriginalProtocolInfo[i].ProtocolChain.ChainEntries[0] = LayeredCatalogId;
    }
    //
    // For each entry that we're layering over, we need a GUID so create one and
    //  install the layer.
    //
    for(i=0; i < CatalogIdArrayCount ;i++)
    {
        if (UuidCreate(&ProviderChainGuid) != RPC_S_OK)
        {
            printf("UuidCreate() failed: %d\n", GetLastError());
        }

        if (WSCInstallProvider(&ProviderChainGuid, 
                                PROVIDER_PATH, 
                                &OriginalProtocolInfo[i], 
                                1, 
                               &ErrorCode) == SOCKET_ERROR)
        {
            printf("WSCInstallProvider for protocol chain failed %d\n", ErrorCode);
            return -1;
        }
        else
            printf("Installing layer: %S\n", OriginalProtocolInfo[i].szProtocol);
    }
	FreeProviders(ProtocolInfo);

	ProtocolInfo = GetProviders(&TotalProtocols);
    //
    // Set the provider order
    //
	if ((CatalogEntries = (LPDWORD) GlobalAlloc(GPTR, TotalProtocols * sizeof(DWORD))) == NULL)
	{
		printf("GlobalAlloc failed %d\n", GetLastError());
		return -1;
	}
    printf("Reordering catalog...\n");
    //
	// Find our provider entries and put them first in the list
    //
	CatIndex = 0;
	for (j = 0; j < TotalProtocols; j++)
    {
        if (IsIdinChain(&ProtocolInfo[j], LayeredCatalogId))
        {
			CatalogEntries[CatIndex++] = ProtocolInfo[j].dwCatalogEntryId;
        }
    }
	// Put all other entries at the end
    //
	for (j = 0; j < TotalProtocols; j++)
    {
        if (!IsIdinChain(&ProtocolInfo[j], LayeredCatalogId))
        {
			CatalogEntries[CatIndex++] = ProtocolInfo[j].dwCatalogEntryId;
        }
    }

	if ((ErrorCode = WSCWriteProviderOrder(CatalogEntries, TotalProtocols)) != ERROR_SUCCESS)
	{
		printf("WSCWriteProviderOrder failed %d\n", ErrorCode);
		return -1;
	}

	FreeProviders(ProtocolInfo);

    return 0;
}

//
// Function: RemoveIdFromChain
//
// Description:
//    This function removes the given CatalogId from the protocol chain 
//    for pinfo.
//
int RemoveIdFromChain(DWORD CatalogId, WSAPROTOCOL_INFOW *pinfo)
{
    int     i, j;

    for(i=0; i < pinfo->ProtocolChain.ChainLen ;i++)
    {
        if (pinfo->ProtocolChain.ChainEntries[i] == CatalogId)
        {
            for(j=i; j < pinfo->ProtocolChain.ChainLen-1 ; j++)
            {
                pinfo->ProtocolChain.ChainEntries[j] = pinfo->ProtocolChain.ChainEntries[j+1];
            }
            pinfo->ProtocolChain.ChainLen--;
            return 0;
        }
    }
    return 1;
}

//
// Function: IsIdinChain
//
// Description:
//    This function determines whether the given catalog id is referenced
//    in the protocol chain of pinfo.
//
BOOL IsIdinChain(WSAPROTOCOL_INFOW *pinfo, DWORD Id)
{
    for(int i=0; i < pinfo->ProtocolChain.ChainLen ;i++)
    {
        if (pinfo->ProtocolChain.ChainEntries[i] == Id)
            return TRUE;
    }
    return FALSE;
}

//
// Function: RemoveProvider
//
// Description:
//    This function removes a layered provider. Things can get tricky if
//    we're removing a layered provider which has been layered over by 
//    another provider.
//
void RemoveProvider(void)
{
    WSAPROTOCOL_INFOW *CleanupProtoInfo;
    WCHAR              wszProviderPath[DEFAULT_PATH_LEN];
    GUID              *GuidOrder=NULL;
	DWORD             *ProtoOrder,
                       LayeredCatalogId,
                       CleanupCount;
	INT                wszProviderPathLen,
                       ErrorCode,
                       idx,
                       i, j;

    ProtocolInfo = GetProviders(&TotalProtocols);
    //
    // Find our provider's catalog ID
    //
    for(i=0; i < (INT)TotalProtocols ;i++)
    {
		if (memcmp (&ProtocolInfo[i].ProviderId, &ProviderGuid, sizeof (GUID))==0)
		{
			LayeredCatalogId = ProtocolInfo[i].dwCatalogEntryId;
			break;
		}
    }
    //
    // Remove our layered entries in which we are the first entries in the
    //  protocol chain.
    //
    for(i=0; i < TotalProtocols ;i++)
    {
        if ((ProtocolInfo[i].ProtocolChain.ChainLen > 1) && 
            (ProtocolInfo[i].ProtocolChain.ChainEntries[0] == LayeredCatalogId))
        {
            if (WSCDeinstallProvider(&ProtocolInfo[i].ProviderId, &ErrorCode) == SOCKET_ERROR)
            {
                printf("RemoveProvider: WSCDeinstallProvider() failed [%d] on %S\n",
                    ErrorCode,
                    ProtocolInfo[i].szProtocol);
            }
        }
    }
    //
    // Remove our dummy (hidden) provider.
    //
	if (WSCDeinstallProvider(&ProviderGuid, &ErrorCode) == SOCKET_ERROR)
	{
		printf("WSCDeistallProvider for Layer failed %d\n", ErrorCode);
	}

    FreeProviders(ProtocolInfo);
    ProtocolInfo = GetProviders(&TotalProtocols);

    CleanupCount = 0;
    GuidOrder = (GUID *)GlobalAlloc(GPTR, sizeof(GUID) * TotalProtocols);
    if (!GuidOrder)
    {
        printf("RemoveProvider: GlobalAlloc() failed: %d\n", GetLastError());
        return;
    }
    // Save off the ordering of the catalog
    //
    for(i=0; i < TotalProtocols ;i++)
    {
        memcpy(&GuidOrder[i], &ProtocolInfo[i].ProviderId, sizeof(GUID));
        //
        // Count how many protocol entries reference the just removed
        //  provider (these are layered providers layered over our layer).
        //  
        for(j=0; j < ProtocolInfo[i].ProtocolChain.ChainLen ;j++)
        {
            if (ProtocolInfo[i].ProtocolChain.ChainEntries[j] == LayeredCatalogId)
            {
                CleanupCount++;
                break;
            }
        }
    }
    //
    // If this is greater than zero then we have other layered providers
    //  that reference our layer. We need to fix their protocol chains
    //  so they no longer reference us.
    //
    if (CleanupCount > 0)
    {
        printf("%d protocol entries layered over the removed provider\n", CleanupCount);

        CleanupProtoInfo = (WSAPROTOCOL_INFOW *)GlobalAlloc(GPTR, CleanupCount * sizeof(WSAPROTOCOL_INFOW));
        if (!CleanupProtoInfo)
        {
            printf("RemoveProvider: GlobalAlloc() failed: %d\n", GetLastError());
            return;
        }
        ProtoOrder = (DWORD *)GlobalAlloc(GPTR, TotalProtocols * sizeof(DWORD));
        if (!ProtoOrder)
        {
            printf("RemoveProviders: GlobalAlloc() failed: %d\n", GetLastError());
            return;
        }
        //
        // Copy off each entry which references the just removed layered provider
        //  and fix its protocol chain so it no longer references the removed
        //  provider.
        //
        idx = 0;
        for(i=0; i < TotalProtocols ;i++)
        {
            for(j=0; j < ProtocolInfo[i].ProtocolChain.ChainLen ;j++)
            {
                if (ProtocolInfo[i].ProtocolChain.ChainEntries[j] == LayeredCatalogId)
                {
                    memcpy(&CleanupProtoInfo[idx], &ProtocolInfo[i], sizeof(WSAPROTOCOL_INFOW));
                    RemoveIdFromChain(LayeredCatalogId, &CleanupProtoInfo[idx]);
                    idx++;
                    break;        
                }
            }
        }
        // Now we need to uninstall each of these entries and re-install with
        //  our copies (which no longer reference the removed provider)
        //
        for(i=0; i < idx ;i++)
        {
            // First we need to obtain the provider path since we'll need this
            // info when we re-install.
            //
            wszProviderPathLen = DEFAULT_PATH_LEN;
            if (WSCGetProviderPath(&CleanupProtoInfo[i].ProviderId,
                                   wszProviderPath,
                                   &wszProviderPathLen,
                                   &ErrorCode) == SOCKET_ERROR)
            {
                printf("WSCGetPathProvider() failed: %d\n", ErrorCode);
            }
            // Remove the provider
            //
            if (WSCDeinstallProvider(&CleanupProtoInfo[i].ProviderId, &ErrorCode) == SOCKET_ERROR)
            {
                printf("WSCDeinstallProvider() failed: %d\n", ErrorCode);
            }
            // Reinstall it
            //
            if (WSCInstallProvider(&CleanupProtoInfo[i].ProviderId,
                                    wszProviderPath,
                                   &CleanupProtoInfo[i],
                                    1,
                                   &ErrorCode) == SOCKET_ERROR)
            {
                printf("WSCInstallProvider() failed: %d\n", ErrorCode);
            }
        }

        FreeProviders(ProtocolInfo);

        GlobalFree(CleanupProtoInfo);

        ProtocolInfo = GetProviders(&TotalProtocols);
        //
        // Reorder the catalog back to the original way
        //
        for(i=0; i < TotalProtocols ;i++)
        {
            for(j=0; j < TotalProtocols ;j++)
            {
                if (!memcmp(&ProtocolInfo[j].ProviderId, &GuidOrder[i], sizeof(GUID)))
                {
                    ProtoOrder[i] = ProtocolInfo[j].dwCatalogEntryId;
                    memset(&ProtocolInfo[j].ProviderId, 0, sizeof(GUID));
                    break;
                }
            }
        }
        if ((ErrorCode = WSCWriteProviderOrder(ProtoOrder, TotalProtocols)) != ERROR_SUCCESS)
        {
            printf("WSCWriteProviderOrder() failed: %d\n", ErrorCode);
        }

        GlobalFree(ProtoOrder);
    }

    FreeProviders(ProtocolInfo);

    GlobalFree(GuidOrder);

    return;
}

//
// Function: RemoveAllLayeredEntries
//
// Description:
//    In the event that the layered entries become totally hosed up. This
//    function will remove any non base provider.
//
void RemoveAllLayeredEntries()
{
    BOOL    bLayer;
    int     ErrorCode,
            i;

    while (1)
    {
        bLayer = FALSE;
        ProtocolInfo = GetProviders(&TotalProtocols);
        if (!ProtocolInfo)
        {
            printf("Unable to enumerate Winsock catalog!\n");
            return;
        }
        for(i=0; i < TotalProtocols ;i++)
        {
            if (ProtocolInfo[i].ProtocolChain.ChainLen != BASE_PROTOCOL)
            {
                bLayer = TRUE;
                printf("Removing '%S'\n", ProtocolInfo[i].szProtocol);
                if (WSCDeinstallProvider(&ProtocolInfo[i].ProviderId, &ErrorCode) == SOCKET_ERROR)
                {
                    printf("Failed to remove [%s]: Error %d\n", ProtocolInfo[i].szProtocol, ErrorCode);
                }
                break;
            }
        }
        FreeProviders(ProtocolInfo);
        if (bLayer == FALSE)
        {
            break;
        }
    }
    return;
}
