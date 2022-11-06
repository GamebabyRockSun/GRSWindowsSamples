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
        //����Ϊִ�д洢����
        pCom->CommandType = adCmdStoredProc ;
        //���ô洢�������� ��Ϊ����
        pCom->CommandText = _bstr_t(_T("Usp_InsertPeople")); 

        //׼������
        VARIANT vName = {};
        vName.vt = VT_BSTR;
        vName.bstrVal = _bstr_t(_T("C++ ADO Procedure Sample1")); 

        VARIANT vAge = {};
        vAge.vt = VT_I2;
        vAge.intVal = 10;
        //�����Ͳ���
        COleCurrency vOleSalary(5000,55);
        //����ʱ���Ͳ���
        COleDateTime vOleDOB(2012,12,22,23,59,59);
        //��Ӳ���
        pCom->Parameters->Append(pCom->CreateParameter(_bstr_t(_T("strCompanyName"))
            ,adChar,adParamInput,50,vName));
        pCom->Parameters->Append(pCom->CreateParameter(_bstr_t(_T("iAge"))
            ,adInteger,adParamInput,4,vAge));
        pCom->Parameters->Append(pCom->CreateParameter(_bstr_t(_T("dob"))
            ,adDate,adParamInput,8,_variant_t(vOleDOB)));
        pCom->Parameters->Append(pCom->CreateParameter(_bstr_t(_T("mSalary"))
            ,adCurrency,adParamInput,8,_variant_t(vOleSalary)));

        //ִ�д洢����
        pCom->Execute(NULL,NULL,adCmdStoredProc); 
       
        GRS_PRINTF(_T("�洢���̵��óɹ�!\n"));
  
        pConn->Close();

    }
    catch(_com_error & e)
    {
        GRS_PRINTF(_T("��������:\n Source : %s \n Description : %s \n")
            ,(LPCTSTR)e.Source(),(LPCTSTR)e.Description());
    }
    CoUninitialize();
    _tsystem(_T("PAUSE"));
    return 0;
}

