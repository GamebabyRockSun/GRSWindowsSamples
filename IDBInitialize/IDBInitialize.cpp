#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define COM_NO_WINDOWS_H    //如果已经包含了Windows.h或不使用其他Windows库函数时
#define OLEDBVER 0x0260     //MSDAC2.6版
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
    GRS_COM_CHECK(hr,_T("获取SQLOLEDB的CLSID失败，错误码：0x%08x\n"),hr);
    //2、使用纯代码方式连接到指定的数据源
    hr = CoCreateInstance(clDBSource, NULL, CLSCTX_INPROC_SERVER,
        IID_IDBInitialize,(void**)&pIDBInitialize);
    GRS_COM_CHECK(hr,_T("无法创建IDBInitialize接口，错误码：0x%08x\n"),hr);

    //准备连接属性
    //注意虽然后面只使用了4个属性，但是任然要定义和初始化4+1个属性
    //目的是让属性数组最后总有一个空的属性作为数组结尾
    DBPROP InitProperties[5] = {};
    DBPROPSET rgInitPropSet[1] = {};

    //指定数据库实例名，这里使用了别名local，指定本地默认实例
    InitProperties[0].dwPropertyID      = DBPROP_INIT_DATASOURCE;
    InitProperties[0].vValue.vt         = VT_BSTR;
    InitProperties[0].vValue.bstrVal    = SysAllocString(L"ASUS-PC\\SQL2008");
    InitProperties[0].dwOptions         = DBPROPOPTIONS_REQUIRED;
    InitProperties[0].colid             = DB_NULLID;
    //指定数据库名
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

    //创建一个GUID为DBPROPSET_DBINIT的属性集合，这也是初始化连接时需要的唯一一个标准属性集合
    rgInitPropSet[0].guidPropertySet    = DBPROPSET_DBINIT;
    rgInitPropSet[0].cProperties        = 4;
    rgInitPropSet[0].rgProperties       = InitProperties;

    //得到数据库初始化的属性接口
    hr = pIDBInitialize->QueryInterface(IID_IDBProperties, (void **)&pIDBProperties);
    GRS_COM_CHECK(hr,_T("获取IDBProperties接口失败，错误码：0x%08x\n"),hr);
    hr = pIDBProperties->SetProperties(1, rgInitPropSet); 
    GRS_COM_CHECK(hr,_T("设置连接属性失败，错误码：0x%08x\n"),hr);
    //属性一但设置完成，相应的接口就可以释放了
    GRS_SAFERELEASE(pIDBProperties);

    //根据指定的属性连接到数据库
    hr = pIDBInitialize->Initialize();
    GRS_COM_CHECK(hr,_T("使用属性连接到数据源失败，错误码：0x%08x\n"),hr);

    GRS_PRINTF(_T("使用属性集方式连接到数据源成功!\n"));
    //这里实际上已经拥有了一个正常的数据源对象,可以进一步进行需要的操作
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