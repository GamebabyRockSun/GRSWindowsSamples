#pragma once

#include <Winsock2.h>
#include <mswsock.h>
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "mswsock")
#include <process.h>
#include <time.h>
#pragma warning(disable:4786) 
#include <vector>
#include <string>
#include <iostream>
#include <utility>
#include <map>
#include <queue>
#include <algorithm>
#include <set>
#include <list>
using namespace std;

enum IOType 
{
	IOConnected,
	IOAccepted,
	IORecved,
	IOSent,
	IOWriteCompleted,
	IOFileTransmitted,
	IOFailed,
};

#define MAX_RECV_SIZE 40960
#define FLAG_CLOSING 0x19821213

typedef map<string, string>	MIMETYPES;

#define HTTP_SERVER_NAME "HttpSvr 1.00 - gamebabyrocksun@163.com"
#define MAX_HTTP_REQUEST 40960
#define HTTP_DEFAULT_FILE "index.html"
