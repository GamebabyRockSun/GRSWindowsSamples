#include <tchar.h>

#import "msxml6.dll"
using namespace MSXML2;

void AddCollectionSample();

int _tmain()
{
    ::CoInitialize(NULL);
    AddCollectionSample();
    ::CoUninitialize();
    return 0;
}

void AddCollectionSample()
{
    IXMLDOMDocument2Ptr pIXMLDOMDocument2;
    IXMLDOMSchemaCollection2Ptr pIXMLDOMSchemaCollection2Ptr;
    int nResult;

    try
    {
        // Create the DOM
        nResult = pIXMLDOMDocument2.CreateInstance(__uuidof(MSXML2::DOMDocument40));
        (nResult == 0) ? 0: throw nResult;

        // Create the Schema Collections
        nResult = pIXMLDOMSchemaCollection2Ptr.CreateInstance(__uuidof(MSXML2::XMLSchemaCache40));
        (nResult == 0) ? 0: throw nResult;

        // Add the schema to the collection
        nResult = pIXMLDOMSchemaCollection2Ptr->add(_T("x-schema:books"), _T("collection.xsd"));
        (nResult == 0) ? 0: throw nResult;

        // Attach schemas
        pIXMLDOMDocument2->schemas = pIXMLDOMSchemaCollection2Ptr.GetInterfacePtr();

        pIXMLDOMDocument2->async = false;
        pIXMLDOMDocument2->validateOnParse = true;

        // Load the document into the DOM
        nResult = pIXMLDOMDocument2->load(_T("collection.xml"));
        (nResult == -1) ? 0: throw nResult;

        ::MessageBox(NULL, pIXMLDOMDocument2->xml, _T("Loaded Document"), MB_OK);
    } catch(...)
    {
        ::MessageBox(NULL, _T("Sample Failed"), _T("Error"), MB_OK);
    }
}