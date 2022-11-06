#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define COM_NO_WINDOWS_H    //����Ѿ�������Windows.h��ʹ������Windows�⺯��ʱ
#define OLEDBVER 0x0260     //MSDAC2.6��
#include <oledb.h>
#include <oledberr.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_SAFERELEASE(I) if(NULL != (I)){(I)->Release();(I)=NULL;}
#define GRS_COM_CHECK(hr,...) if(FAILED(hr)){GRS_PRINTF(__VA_ARGS__);goto CLEAR_UP;}

int _tmain()
{
    GRS_USEPRINTF();

    CoInitialize(NULL);
    HRESULT                     hr                          = S_OK;
    IDBInitialize*              pIDBInitialize              = NULL;
    IDBProperties*              pIDBProperties              = NULL;
    CLSID                       clDBSource                  = {};

    hr = CLSIDFromProgID(_T("SQLOLEDB"), &clDBSource);
    GRS_COM_CHECK(hr,_T("��ȡSQLOLEDB��CLSIDʧ�ܣ������룺0x%08x\n"),hr);
    //2��ʹ�ô����뷽ʽ���ӵ�ָ��������Դ
    hr = CoCreateInstance(clDBSource, NULL, CLSCTX_INPROC_SERVER,
        IID_IDBInitialize,(void**)&pIDBInitialize);
    GRS_COM_CHECK(hr,_T("�޷�����IDBInitialize�ӿڣ������룺0x%08x\n"),hr);

    //׼����������
    //ע����Ȼ����ֻʹ����4�����ԣ�������ȻҪ����ͳ�ʼ��4+1������
    //Ŀ���������������������һ���յ�������Ϊ�����β
    DBPROP InitProperties[5] = {};
    DBPROPSET rgInitPropSet[1] = {};

    //ָ�����ݿ�ʵ����������ʹ���˱���local��ָ������Ĭ��ʵ��
    InitProperties[0].dwPropertyID      = DBPROP_INIT_DATASOURCE;
    InitProperties[0].vValue.vt         = VT_BSTR;
    InitProperties[0].vValue.bstrVal    = SysAllocString(L"ASUS-PC\\SQL2008");
    InitProperties[0].dwOptions         = DBPROPOPTIONS_REQUIRED;
    InitProperties[0].colid             = DB_NULLID;
    //ָ�����ݿ���
    InitProperties[1].dwPropertyID      = DBPROP_INIT_CATALOG;
    InitProperties[1].vValue.vt         = VT_BSTR;
    InitProperties[1].vValue.bstrVal    = SysAllocString(L"Study");
    InitProperties[1].dwOptions         = DBPROPOPTIONS_REQUIRED;
    InitProperties[1].colid             = DB_NULLID;

    // User ID
    InitProperties[2].dwPropertyID      = DBPROP_AUTH_USERID;
    InitProperties[2].vValue.vt         = VT_BSTR;
    InitProperties[2].vValue.bstrVal    = SysAllocString(OLESTR("sa"));

    // Password
    InitProperties[3].dwPropertyID      = DBPROP_AUTH_PASSWORD;
    InitProperties[3].vValue.vt         = VT_BSTR;
    InitProperties[3].vValue.bstrVal    = SysAllocString(OLESTR("999999"));

    //����һ��GUIDΪDBPROPSET_DBINIT�����Լ��ϣ���Ҳ�ǳ�ʼ������ʱ��Ҫ��Ψһһ����׼���Լ���
    rgInitPropSet[0].guidPropertySet    = DBPROPSET_DBINIT;
    rgInitPropSet[0].cProperties        = 4;
    rgInitPropSet[0].rgProperties       = InitProperties;

    //�õ����ݿ��ʼ�������Խӿ�
    hr = pIDBInitialize->QueryInterface(IID_IDBProperties, (void **)&pIDBProperties);
    GRS_COM_CHECK(hr,_T("��ȡIDBProperties�ӿ�ʧ�ܣ������룺0x%08x\n"),hr);
    hr = pIDBProperties->SetProperties(1, rgInitPropSet); 
    GRS_COM_CHECK(hr,_T("������������ʧ�ܣ������룺0x%08x\n"),hr);
    //����һ��������ɣ���Ӧ�ĽӿھͿ����ͷ���
    GRS_SAFERELEASE(pIDBProperties);

    //����ָ�����������ӵ����ݿ�
    hr = pIDBInitialize->Initialize();
    GRS_COM_CHECK(hr,_T("ʹ���������ӵ�����Դʧ�ܣ������룺0x%08x\n"),hr);

    GRS_PRINTF(_T("ʹ�����Լ���ʽ���ӵ�����Դ�ɹ�!\n"));
    //����ʵ�����Ѿ�ӵ����һ������������Դ����,���Խ�һ��������Ҫ�Ĳ���
    //..................

    pIDBInitialize->Uninitialize();
    GRS_SAFERELEASE(pIDBInitialize);
CLEAR_UP:
    GRS_SAFERELEASE(pIDBProperties);
    GRS_SAFERELEASE(pIDBInitialize);
    _tsystem(_T("PAUSE"));
    CoUninitialize();
    return 0;
}