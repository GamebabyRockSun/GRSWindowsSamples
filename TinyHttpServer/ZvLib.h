#pragma once

#pragma warning(disable: 4786)
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class CZvLib  
{
public:
	CZvLib();
	virtual ~CZvLib();

	static string GetExeName();
	static string GetAppPath();
	static void DebugLog(const char * szLog, bool bTime=true);
	static void SplitString(const char * szStr, const char * szSplitter, vector<string> & vtStr, const char * szEndStr=NULL);
	static bool StringContainChars(string & szStr, string & szChars);
	static bool StringIsChar(string & szStr);
};