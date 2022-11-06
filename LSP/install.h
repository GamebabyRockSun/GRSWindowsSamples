// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.
//
// Module Name: provider.h
//
// Description:
//
//    This sample illustrates how to develop a layered service provider that is
//    capable of counting all bytes transmitted through a TCP/IP socket.
//
//    This file contains all datatypes and function prototypes used
//    throughout this project.
//
#ifndef _INSTALL_H_
#define _INSTALL_H_ 

#include <ws2spi.h>

//
// GUID corresponding to our provider
//
extern GUID           ProviderGuid;

//
// Our LSP DLL name
// //
#define PROVIDER_PATH L"lsp.dll"

//
// Provider.cpp prototypes
//
LPWSAPROTOCOL_INFOW GetProviders(LPINT TotalProtocols);
void                FreeProviders(LPWSAPROTOCOL_INFOW ProtocolInfo);


#endif
