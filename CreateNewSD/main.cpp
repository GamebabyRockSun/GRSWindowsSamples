#include <tchar.h>
#include <windows.h>
#include <aclapi.h>
#include <strsafe.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_REALLOC(p,sz)	HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#define GRS_FREE(p)			HeapFree(GetProcessHeap(),0,p)
#define GRS_MSIZE(p)		HeapSize(GetProcessHeap(),0,p)
#define GRS_MVALID(p)		HeapValidate(GetProcessHeap(),0,p)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);
int _tmain()
{
	GRS_USEPRINTF();

	DWORD dwRes, dwDisposition;
	PSID pEveryoneSID = NULL, pAdminSID = NULL;
	PACL pACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	EXPLICIT_ACCESS ea[2] = {};
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	SECURITY_ATTRIBUTES sa = {};
	LONG lRes;
	HKEY hkSub = NULL;

	//代表Everyone用户的SID
	if(!AllocateAndInitializeSid(&SIDAuthWorld,1,SECURITY_WORLD_RID,0, 0, 0, 0, 0, 0, 0,&pEveryoneSID))
	{
		GRS_PRINTF(_T("AllocateAndInitializeSid Error %u\n"), GetLastError());
		goto Cleanup;
	}
	ea[0].grfAccessPermissions	= KEY_READ;
	ea[0].grfAccessMode			= SET_ACCESS;
	ea[0].grfInheritance		= NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm	= TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType	= TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName		= (LPTSTR) pEveryoneSID;

	//代表Administrator的SID
	if(! AllocateAndInitializeSid(&SIDAuthNT, 2,SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,0, 0, 0, 0, 0, 0,&pAdminSID)) 
	{
		GRS_PRINTF(_T("AllocateAndInitializeSid Error %u\n"), GetLastError());
		goto Cleanup; 
	}
	ea[1].grfAccessPermissions	= KEY_ALL_ACCESS;
	ea[1].grfAccessMode			= SET_ACCESS;
	ea[1].grfInheritance		= NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm	= TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType	= TRUSTEE_IS_GROUP;
	ea[1].Trustee.ptstrName		= (LPTSTR) pAdminSID;

	//将ACE装入ACL
	dwRes = SetEntriesInAcl(2, ea, NULL, &pACL);
	if (ERROR_SUCCESS != dwRes) 
	{
		GRS_PRINTF(_T("SetEntriesInAcl Error %u\n"), GetLastError());
		goto Cleanup;
	}

	//分配安全描述符
	pSD = (PSECURITY_DESCRIPTOR) GRS_CALLOC(SECURITY_DESCRIPTOR_MIN_LENGTH); 
	if (NULL == pSD) 
	{ 
		GRS_PRINTF(_T("HeapAlloc Error %u\n"), GetLastError());
		goto Cleanup; 
	} 

	//初始化一个绝对的安全描述符
	if (!InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION)) 
	{  
		GRS_PRINTF(_T("InitializeSecurityDescriptor Error %u\n"),GetLastError());
		goto Cleanup; 
	} 

	//将ACL装入安全描述符
	if (!SetSecurityDescriptorDacl(pSD,TRUE,pACL,FALSE))
	{  
		GRS_PRINTF(_T("SetSecurityDescriptorDacl Error %u\n"),GetLastError());
		goto Cleanup; 
	} 

	//初始化一个安全属性结构用于创建对象时传入，其中放入刚才创建的安全描述符
	sa.nLength = sizeof (SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

	//利用这个安全属性结构创建一个对象，这里创建的是一个注册表键对象
	lRes = RegCreateKeyEx(HKEY_CURRENT_USER, _T("mykey"), 0, _T(""), 0,	KEY_READ | KEY_WRITE, &sa, &hkSub, &dwDisposition); 
	GRS_PRINTF(_T("RegCreateKeyEx result %u\n"), lRes );

	_tsystem(_T("PAUSE"));
Cleanup:
	if (pEveryoneSID) 
	{
		FreeSid(pEveryoneSID);
	}
	if (pAdminSID) 
	{
		FreeSid(pAdminSID);
	}
	if (pACL) 
	{
		LocalFree(pACL);
	}
	GRS_SAFEFREE(pSD);
	if (hkSub)
	{
		RegCloseKey(hkSub);
	}
	return 0;

}

