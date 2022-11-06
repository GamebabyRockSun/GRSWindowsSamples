#include "stdafx.h"
#include "ZvLib.h"
#include <windows.h>

CZvLib::CZvLib()
{

}

CZvLib::~CZvLib()
{

}

string CZvLib::GetAppPath()
{
    char buf[MAX_PATH] = {};

	GetModuleFileName(NULL, buf ,MAX_PATH);
	strcpy(strrchr(buf,'\\') + 1,"");

	return buf;
}

string CZvLib::GetExeName()
{
    char buf[MAX_PATH] = {};

	GetModuleFileName(NULL, buf ,MAX_PATH);
	strcpy(buf, strrchr(buf,'\\') + 1);

	return buf;
}

void CZvLib::SplitString(const char * szStr, const char * szSplitter, vector<string> & vtStr, const char * szEndStr)
{
	string str = szStr;
	string strSplitter = szSplitter;

	size_t i,j;
	size_t l=strSplitter.size();

	i = j = 0;
	string strSub;
	size_t k = szEndStr==NULL?str.npos:str.find(szEndStr);

	while(1)
	{
		j = str.find(szSplitter, i);
		if(j>k)
			j=k;
		strSub = str.substr(i,j-i);
		if(strSub.size()>0)
			vtStr.push_back(strSub);
		i=j+l;
		if(j==str.npos || j>=k)
			break;
	}
}

bool CZvLib::StringContainChars(string & szStr, string & szChars)
{
	int len = szChars.size();

	for(int i=0; i<len; i++)
	{
		if(szStr.find(szChars[i])!=szChars.npos)
			return true;
	}
	return false;
}

bool CZvLib::StringIsChar(string & szStr)
{
	int len = szStr.size();

	for(int i=0; i<len; i++)
	{
		if(!((szStr[i]>='0'&&szStr[i]<='9')||
			(szStr[i]>='a'&&szStr[i]<='z')||
			(szStr[i]>='A'&&szStr[i]<='Z')))
			return false;
	}
	return true;
}