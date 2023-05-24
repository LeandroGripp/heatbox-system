// Simple OPC Client
//
// This is a modified version of the "Simple OPC Client" originally
// developed by Philippe Gras (CERN) for demonstrating the basic techniques
// involved in the development of an OPC DA client.
//
// The modifications are the introduction of two C++ classes to allow the
// the client to ask for callback notifications from the OPC server, and
// the corresponding introduction of a message comsumption loop in the
// main program to allow the client to process those notifications. The
// C++ classes implement the OPC DA 1.0 IAdviseSink and the OPC DA 2.0
// IOPCDataCallback client interfaces, and in turn were adapted from the
// KEPWARE´s  OPC client sample code. A few wrapper functions to initiate
// and to cancel the notifications were also developed.
//
// The original Simple OPC Client code can still be found (as of this date)
// in
//        http://pgras.home.cern.ch/pgras/OPCClientTutorial/
//
//
// Luiz T. S. Mendes - DELT/UFMG - 15 Sept 2011
// luizt at cpdee.ufmg.br
//

#include <atlbase.h>    // required for using the "_T" macro
#include <iostream>
#include <ObjIdl.h>

#include "opcda.h"
#include "opcerror.h"
#include "OPCClient.h"
#include "SOCAdviseSink.h"
#include "SOCDataCallback.h"
#include "SOCWrapperFunctions.h"

using namespace std;

#define OPC_SERVER_NAME L"Matrikon.OPC.Simulation.1"

// Global variables

// The OPC DA Spec requires that some constants be registered in order to use
// them. The one below refers to the OPC DA 1.0 IDataObject interface.
UINT OPC_DATA_TIME = RegisterClipboardFormat(_T("OPCSTMFORMATDATATIME"));

wchar_t HOTBOX_DATA_GROUP_NAME[] = L"HotboxInfo";

wchar_t HOTBOX_IDENTIFIER_ID[] = L"Random.Int2";
wchar_t RAILWAY_COMPOSITION_ID[] = L"Random.String";
wchar_t TEMPERATURE_ID[] = L"Random.Real4";
wchar_t ALARM_ID[] = L"Random.Int4";
wchar_t DATETIME_ID[] = L"Random.Time";

wchar_t HEATBOX_PARAMS_GROUP_NAME[] = L"HotboxParams";

wchar_t INDICATOR_IDENTIFIER_ID[] = L"Bucket Brigade.Int1";
wchar_t INDICATOR_TYPE_ID[] = L"Bucket Brigade.Int2";
wchar_t INDICATOR_STATE_ID[] = L"Bucket Brigade.Boolean";

//////////////////////////////////////////////////////////////////////
// Read the value of an item on an OPC server. 
//
DWORD WINAPI OpcClient(LPVOID dataForThreads)
{
	DataForThreads *data;
	data = (DataForThreads*)dataForThreads;

	IOPCServer* pIOPCServer = NULL;   //pointer to IOPServer interface
	IOPCItemMgt* pDataIOPCItemMgt = NULL; //pointer to IOPCItemMgt interface of the data group
	IOPCItemMgt* pParamsIOPCItemMgt = NULL; //pointer to IOPCItemMgt interface of the params group

	OPCHANDLE hServerHeatboxDataGroup; // server handle to the group that reads

	OPCHANDLE hServerHotboxIdentifier;
	OPCHANDLE hServerRailwayComposition;
	OPCHANDLE hServerTemperature;
	OPCHANDLE hServerAlarm;
	OPCHANDLE hServerDatetime;

	OPCHANDLE hServerHeatboxParamsGroup; // server handle to the group that writes

	OPCHANDLE hServerIndicatorIdentifier;
	OPCHANDLE hServerIndicatorType;
	OPCHANDLE hServerIndicatorState;

	OPCHANDLE hServerItem;  // server handle to the item

	int i;
	char buf[100];

	// Have to be done before using microsoft COM library:
	printf("Initializing the COM environment...\n");
	CoInitialize(NULL);

	// Let's instantiante the IOPCServer interface and get a pointer of it:
	printf("Instantiating the MATRIKON OPC Server for Simulation...\n");
	pIOPCServer = InstantiateServer(OPC_SERVER_NAME);

	// Add the OPC group the OPC server and get an handle to the IOPCItemMgt
	//interface:
	printf("Adding a group in the INACTIVE state for the moment...\n");
	AddGroup(HOTBOX_DATA_GROUP_NAME, pIOPCServer, pDataIOPCItemMgt, hServerHeatboxDataGroup);

	// Add the items one by one.
	AddItem(HOTBOX_IDENTIFIER_ID, VT_I2, pDataIOPCItemMgt, hServerHotboxIdentifier, H_HOTBOX_IDENTIFIER);
	AddItem(RAILWAY_COMPOSITION_ID, VT_BSTR, pDataIOPCItemMgt, hServerRailwayComposition, H_RAILWAY_COMPOSITION);
	AddItem(TEMPERATURE_ID, VT_R4, pDataIOPCItemMgt, hServerTemperature, H_TEMPERATURE);
	AddItem(ALARM_ID, VT_I4, pDataIOPCItemMgt, hServerAlarm, H_ALARM);
	AddItem(DATETIME_ID, VT_BSTR, pDataIOPCItemMgt, hServerDatetime, H_DATETIME);

	AddGroup(HEATBOX_PARAMS_GROUP_NAME, pIOPCServer, pParamsIOPCItemMgt, hServerHeatboxParamsGroup);

	AddItem(INDICATOR_IDENTIFIER_ID, VT_I1, pParamsIOPCItemMgt, hServerItem, 6);

	// TESTE DE ESCRITA SÍNCRONA
	 VARIANT valToWrite;
	 VariantInit(&valToWrite);
	 valToWrite.iVal = 5;
	 valToWrite.vt = VT_I1;

	 WriteItem(pParamsIOPCItemMgt, hServerItem, valToWrite, pIOPCServer);

	//VARIANT varValue; //to store the read value
	//VariantInit(&varValue);
	//ReadItem(pIOPCItemMgt, hServerItem, varValue);
	//// print the read value:
	//printf("Read value: %d\n", varValue.iVal);

	//Synchronous read of the device´s item value.
	//VARIANT varValue; //to store the read value
	//VariantInit(&varValue);
	//printf("Reading synchronously during 10 seconds...\n");
	//for (i = 0; i < 10; i++) {
	//	ReadItem(pIOPCItemMgt, hServerItem, varValue);
	//	// print the read value:
	//	printf("Read value: %6.2f\n", varValue.fltVal);
	//	// wait 1 second
	//	Sleep(1000);
	//}

	//// Establish a callback asynchronous read by means of the old IAdviseSink()
	//// (OPC DA 1.0) method. We first instantiate a new SOCAdviseSink object and
	//// adjusts its reference count, and then call a wrapper function to
	//// setup the callback.
	//IDataObject* pIDataObject = NULL; //pointer to IDataObject interface
	//DWORD tkAsyncConnection = 0;
	//SOCAdviseSink* pSOCAdviseSink = new SOCAdviseSink();
	//pSOCAdviseSink->AddRef();
	//printf("Setting up the IAdviseSink callback connection...\n");
	//SetAdviseSink(pIOPCItemMgt, pSOCAdviseSink, pIDataObject, &tkAsyncConnection);

	// Change the group to the ACTIVE state so that we can receive the
	// server´s callback notification
	printf("Changing the group state to ACTIVE...\n");
	SetGroupActive(pDataIOPCItemMgt);

	// Enters a message pump in order to process the server´s callback
	// notifications. This is needed because the CoInitialize() function
	// forces the COM threading model to STA (Single Threaded Apartment),
	// in which, according to the MSDN, "all method calls to a COM object
	// (...) are synchronized with the windows message queue for the
	// single-threaded apartment's thread." So, even being a console
	// application, the OPC client must process messages (which in this case
	// are only useless WM_USER [0x0400] messages) in order to process
	// incoming callbacks from a OPC server.
	//
	// A better alternative could be to use the CoInitializeEx() function,
	// which allows one to  specifiy the desired COM threading model;
	// in particular, calling
	//        CoInitializeEx(NULL, COINIT_MULTITHREADED)
	// sets the model to MTA (MultiThreaded Apartments) in which a message
	// loop is __not required__ since objects in this model are able to
	// receive method calls from other threads at any time. However, in the
	// MTA model the user is required to handle any aspects regarding
	// concurrency, since asynchronous, multiple calls to the object methods
	// can occur.
	//
	int bRet;
	MSG msg;
	DWORD ticks1, ticks2;
	//ticks1 = GetTickCount();
	//printf("Waiting for IAdviseSink callback notifications during 10 seconds...\n");
	//do {
	//	bRet = GetMessage(&msg, NULL, 0, 0);
	//	if (!bRet) {
	//		printf("Failed to get windows message! Error code = %d\n", GetLastError());
	//		exit(0);
	//	}
	//	TranslateMessage(&msg); // This call is not really needed ...
	//	DispatchMessage(&msg);  // ... but this one is!
	//	ticks2 = GetTickCount();
	//} while ((ticks2 - ticks1) < 10000);

	//// Cancel the callback and release its reference
	//printf("Cancelling the IAdviseSink callback...\n");
	//CancelAdviseSink(pIDataObject, tkAsyncConnection);
	//pSOCAdviseSink->Release();

	// Establish a callback asynchronous read by means of the IOPCDaraCallback
	// (OPC DA 2.0) method. We first instantiate a new SOCDataCallback object and
	// adjusts its reference count, and then call a wrapper function to
	// setup the callback.
	IConnectionPoint* pIConnectionPoint = NULL; //pointer to IConnectionPoint Interface
	DWORD dwCookie = 0;
	SOCDataCallback* pSOCDataCallback = new SOCDataCallback(data);
	pSOCDataCallback->AddRef();

	printf("Setting up the IConnectionPoint callback connection...\n");
	SetDataCallback(pDataIOPCItemMgt, pSOCDataCallback, pIConnectionPoint, &dwCookie);

	// Change the group to the ACTIVE state so that we can receive the
	// server´s callback notification
	printf("Changing the group state to ACTIVE...\n");
	SetGroupActive(pDataIOPCItemMgt);

	// Enter again a message pump in order to process the server´s callback
	// notifications, for the same reason explained before.

	ticks1 = GetTickCount();
	printf("Waiting for IOPCDataCallback notifications during 10 seconds...\n");
	do {
		bRet = GetMessage(&msg, NULL, 0, 0);
		if (!bRet) {
			printf("Failed to get windows message! Error code = %d\n", GetLastError());
			exit(0);
		}
		TranslateMessage(&msg); // This call is not really needed ...
		DispatchMessage(&msg);  // ... but this one is!
		ticks2 = GetTickCount();
	} while ((ticks2 - ticks1) < 10000);

	// Cancel the callback and release its reference
	printf("Cancelling the IOPCDataCallback notifications...\n");
	CancelDataCallback(pIConnectionPoint, dwCookie);
	//pIConnectionPoint->Release();
	pSOCDataCallback->Release();

	// Remove the OPC item:
	//printf("Removing the OPC item...\n");
	//RemoveItem(pIOPCItemMgt, hServerItem);

	// Remove the OPC groups:
	printf("Removing the OPC group objects...\n");
	pDataIOPCItemMgt->Release();
	pParamsIOPCItemMgt->Release();
	RemoveGroup(pIOPCServer, hServerHeatboxDataGroup);
	RemoveGroup(pIOPCServer, hServerHeatboxParamsGroup);

	// release the interface references:
	printf("Removing the OPC server object...\n");
	pIOPCServer->Release();

	//close the COM library:
	printf("Releasing the COM environment...\n");
	CoUninitialize();
}

////////////////////////////////////////////////////////////////////
// Instantiate the IOPCServer interface of the OPCServer
// having the name ServerName. Return a pointer to this interface
//
IOPCServer* InstantiateServer(wchar_t ServerName[])
{
	CLSID CLSID_OPCServer;
	HRESULT hr;

	// get the CLSID from the OPC Server Name:
	hr = CLSIDFromString(ServerName, &CLSID_OPCServer);
	_ASSERT(!FAILED(hr));


	//queue of the class instances to create
	LONG cmq = 1; // nbr of class instance to create.
	MULTI_QI queue[1] =
	{ {&IID_IOPCServer,
	NULL,
	0} };

	//Server info:
	//COSERVERINFO CoServerInfo =
	//{
	//	/*dwReserved1*/ 0,
	//	/*pwszName*/ REMOTE_SERVER_NAME,
	//	/*COAUTHINFO*/  NULL,
	//	/*dwReserved2*/ 0
	//}; 

	// create an instance of the IOPCServer
	hr = CoCreateInstanceEx(CLSID_OPCServer, NULL, CLSCTX_SERVER,
		/*&CoServerInfo*/NULL, cmq, queue);
	_ASSERT(!hr);

	// return a pointer to the IOPCServer interface:
	return(IOPCServer*)queue[0].pItf;
}


/////////////////////////////////////////////////////////////////////
// Add group "Group1" to the Server whose IOPCServer interface
// is pointed by pIOPCServer. 
// Returns a pointer to the IOPCItemMgt interface of the added group
// and a server opc handle to the added group.
//
void AddGroup(wchar_t* groupName, IOPCServer* pIOPCServer, IOPCItemMgt* &pIOPCItemMgt,
	OPCHANDLE& hServerGroup)
{
	DWORD dwUpdateRate = 0;
	OPCHANDLE hClientGroup = 0;

	// Add an OPC group and get a pointer to the IUnknown I/F:
	HRESULT hr = pIOPCServer->AddGroup(/*szName*/ groupName,
		/*bActive*/ FALSE,
		/*dwRequestedUpdateRate*/ 1000,
		/*hClientGroup*/ hClientGroup,
		/*pTimeBias*/ 0,
		/*pPercentDeadband*/ 0,
		/*dwLCID*/0,
		/*phServerGroup*/&hServerGroup,
		&dwUpdateRate,
		/*riid*/ IID_IOPCItemMgt,
		/*ppUnk*/ (IUnknown**)&pIOPCItemMgt);
	_ASSERT(!FAILED(hr));
}



//////////////////////////////////////////////////////////////////
// Add the Item ITEM_ID to the group whose IOPCItemMgt interface
// is pointed by pIOPCItemMgt pointer. Return a server opc handle
// to the item.

void AddItem(wchar_t* itemName, VARTYPE itemType, IOPCItemMgt* pIOPCItemMgt, OPCHANDLE &hServerItem, OPCHANDLE clientHandle)
{
	HRESULT hr;

	// Add the OPC item. First we have to convert from wchar_t* to char*
	// in order to print the item name in the console.
	size_t m;
	char buf[100];
	wcstombs_s(&m, buf, 100, itemName, _TRUNCATE);
	printf("Adding the item %s to the group...\n", buf);

	// Array of items to add:
	OPCITEMDEF ItemArray[1] =
	{ {
			/*szAccessPath*/ L"",
			/*szItemID*/ itemName,
			/*bActive*/ TRUE,
			/*hClient*/ clientHandle,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ itemType,
			/*wReserved*/0
			}
	};
	

	//Add Result:
	OPCITEMRESULT* pAddResult = NULL;
	HRESULT* pErrors = NULL;

	// Add an Item to the previous Group:
	hr = pIOPCItemMgt->AddItems(1, ItemArray, &pAddResult, &pErrors);
	if (hr != S_OK) {
		printf("Failed call to AddItems function. Error code = %x\n", hr);
		exit(0);
	}

	// Server handle for the added item:
	hServerItem = pAddResult[0].hServer;

	// release memory allocated by the server:
	CoTaskMemFree(pAddResult->pBlob);

	CoTaskMemFree(pAddResult);
	pAddResult = NULL;

	CoTaskMemFree(pErrors);
	pErrors = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Read from device the value of the item having the "hServerItem" server 
// handle and belonging to the group whose one interface is pointed by
// pGroupIUnknown. The value is put in varValue. 
//
void ReadItem(IUnknown* pGroupIUnknown, OPCHANDLE hServerItem, VARIANT& varValue)
{
	// value of the item:
	OPCITEMSTATE* pValue = NULL;

	// get a pointer to the IOPCSyncIOInterface:
	IOPCSyncIO* pIOPCSyncIO;
	pGroupIUnknown->QueryInterface(__uuidof(pIOPCSyncIO), (void**)&pIOPCSyncIO);

	// read the item value from the device:
	HRESULT* pErrors = NULL; //to store error code(s)
	HRESULT hr = pIOPCSyncIO->Read(OPC_DS_DEVICE, 1, &hServerItem, &pValue, &pErrors);
	_ASSERT(!hr);
	_ASSERT(pValue != NULL);

	varValue = pValue[0].vDataValue;

	// Release memeory allocated by the OPC server:
	CoTaskMemFree(pErrors);
	pErrors = NULL;

	CoTaskMemFree(pValue);
	pValue = NULL;

	// release the reference to the IOPCSyncIO interface:
	pIOPCSyncIO->Release();
}

void WriteItem(IUnknown* pGroupIUnknown, OPCHANDLE hServerItem, VARIANT varValue, IOPCServer* pIOPCServer)
{
	// get a pointer to the IOPCSyncIOInterface:
	IOPCSyncIO* pIOPCSyncIO;
	pGroupIUnknown->QueryInterface(__uuidof(pIOPCSyncIO), (void**)&pIOPCSyncIO);

	// write the item value to the device:
	HRESULT* pErrors = NULL; //to store error code(s)
	HRESULT hr = pIOPCSyncIO->Write(1, &hServerItem, &varValue, &pErrors);

	printf("Status of hr is: ");
	switch (hr) {
		case S_OK:
			printf("S_OK \n"); break;
		case S_FALSE:
			printf("S_FALSE \n"); break;
		case E_FAIL:
			printf("E_FAIL \n"); break;
		case E_OUTOFMEMORY:
			printf("E_OUTOFMEMORY \n"); break;
		case E_INVALIDARG:
			printf("E_INVALIDARG \n"); break;
		default:
			printf("Unknown state \n");
	}

	printf("Status of pError is: ");
	switch (*pErrors) {
	case S_OK:
		printf("S_OK \n"); break;
	case E_FAIL:
		printf("E_FAIL \n"); break;
	case OPC_S_CLAMP:
		printf("OPC_S_CLAMP \n"); break;
	case OPC_E_RANGE:
		printf("OPC_E_RANGE \n"); break;
	case OPC_E_BADTYPE:
		printf("OPC_E_BADTYPE \n"); break;
	case OPC_E_BADRIGHTS:
		printf("OPC_E_BADRIGHTS \n"); break;
	case OPC_E_INVALIDHANDLE:
		printf("OPC_E_INVALIDHANDLE \n"); break;
	case OPC_E_UNKNOWNITEMID:
		printf("OPC_E_UNKNOWNITEMID \n"); break;
	default:
		printf("Unknown error \n");
		//pIOPCServer->GetErrorString()


		
	}


		
	
	_ASSERT(!hr);
	//_ASSERT(pValue != NULL);
}

///////////////////////////////////////////////////////////////////////////
// Remove the item whose server handle is hServerItem from the group
// whose IOPCItemMgt interface is pointed by pIOPCItemMgt
//
void RemoveItem(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE hServerItem)
{
	// server handle of items to remove:
	OPCHANDLE hServerArray[1];
	hServerArray[0] = hServerItem;

	//Remove the item:
	HRESULT* pErrors; // to store error code(s)
	HRESULT hr = pIOPCItemMgt->RemoveItems(1, hServerArray, &pErrors);
	_ASSERT(!hr);

	//release memory allocated by the server:
	CoTaskMemFree(pErrors);
	pErrors = NULL;
}

////////////////////////////////////////////////////////////////////////
// Remove the Group whose server handle is hServerGroup from the server
// whose IOPCServer interface is pointed by pIOPCServer
//
void RemoveGroup(IOPCServer* pIOPCServer, OPCHANDLE hServerGroup)
{
	// Remove the group:
	HRESULT hr = pIOPCServer->RemoveGroup(hServerGroup, FALSE);
	if (hr != S_OK) {
		if (hr == OPC_S_INUSE)
			printf("Failed to remove OPC group: object still has references to it.\n");
		else printf("Failed to remove OPC group. Error code = %x\n", hr);
		exit(0);
	}
}
