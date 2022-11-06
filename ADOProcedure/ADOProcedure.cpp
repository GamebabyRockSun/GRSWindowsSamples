#define _AFXDLL 
#include <Afxdisp.h>    //for COleCurrency COleDateTime
#include <tchar.h>
#include <strsafe.h>
#import "C:\Program Files\Common Files\System\ado\msado15.dll" \
    no_namespace rename("EOF", "EndOfFile")

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_COM_CHECK(hr,...) if(FAILED(hr)){GRS_PRINTF(__VA_ARGS__);goto CLEAN_UP;}

int _tmain()
{
    GRS_USEPRINTF();
    CoInitialize(NULL);
    try
    {
        _bstr_t strName;
        _bstr_t strAge;
        _bstr_t strDOB;
        _bstr_t strSalary;

        _ConnectionPtr pConn(__uuidof(Connection));
        _CommandPtr pCom(__uuidof(Command));

        _bstr_t strCon(_T("Provider=SQLOLEDB.1;Persist Security Info=False;\
                          User ID=sa;Password=999999;Initial Catalog=Study;\
                          Data Source=ASUS-PC\\SQL2008;"));

        pConn->Open(strCon,_T(""),_T(""),0);
     
        pCom->ActiveConnection = pConn;
        //设置为执行存储过程
        pCom->CommandType = adCmdStoredProc ;
        //设置存储过程名称 作为命令
        pCom->CommandText = _bstr_t(_T("Usp_InsertPeople")); 

        //准备参数
        VARIANT vName = {};
        vName.vt = VT_BSTR;
        vName.bstrVal = _bstr_t(_T("C++ ADO Procedure Sample1")); 

        VARIANT vAge = {};
        vAge.vt = VT_I2;
        vAge.intVal = 10;
        //货币型参数
        COleCurrency vOleSalary(5000,55);
        //日期时间型参数
        COleDateTime vOleDOB(2012,12,22,23,59,59);
        //添加参数
        pCom->Parameters->Append(pCom->CreateParameter(_bstr_t(_T("strCompanyName"))
            ,adChar,adParamInput,50,vName));
        pCom->Parameters->Append(pCom->CreateParameter(_bstr_t(_T("iAge"))
            ,adInteger,adParamInput,4,vAge));
        pCom->Parameters->Append(pCom->CreateParameter(_bstr_t(_T("dob"))
            ,adDate,adParamInput,8,_variant_t(vOleDOB)));
        pCom->Parameters->Append(pCom->CreateParameter(_bstr_t(_T("mSalary"))
            ,adCurrency,adParamInput,8,_variant_t(vOleSalary)));

        //执行存储过程
        pCom->Execute(NULL,NULL,adCmdStoredProc); 
       
        GRS_PRINTF(_T("存储过程调用成功!\n"));
  
        pConn->Close();

    }
    catch(_com_error & e)
    {
        GRS_PRINTF(_T("发生错误:\n Source : %s \n Description : %s \n")
            ,(LPCTSTR)e.Source(),(LPCTSTR)e.Description());
    }
    CoUninitialize();
    _tsystem(_T("PAUSE"));
    return 0;
}

