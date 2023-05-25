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
// OPC Client that reads asynchrounously from an OPC Server and writes
// synchronously to it.
//
DWORD WINAPI OpcClient(LPVOID dataForThreads)
{
	// Cast the received params to the right type. Params passed to threads need to be of type
	// void pointer and need, therefore, to be cast.
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

	int i;
	char buf[100];

	// Have to be done before using microsoft COM library:
	printf("Initializing the COM environment...\n");
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	// Let's instantiante the IOPCServer interface and get a pointer of it:
	printf("Instantiating the MATRIKON OPC Server for Simulation...\n");
	pIOPCServer = InstantiateServer(OPC_SERVER_NAME);

	// Add the OPC group to the OPC server and get an handle to the IOPCItemMgt
	// interface:
	printf("Adding the group of data to be read asynchronously as active...\n");
	AddGroup(HOTBOX_DATA_GROUP_NAME, pIOPCServer, pDataIOPCItemMgt, hServerHeatboxDataGroup);
	SetGroupActive(pDataIOPCItemMgt);

	// Add the items to be read one by one.
	AddItem(HOTBOX_IDENTIFIER_ID, VT_I2, pDataIOPCItemMgt, hServerHotboxIdentifier, H_HOTBOX_IDENTIFIER);
	AddItem(RAILWAY_COMPOSITION_ID, VT_BSTR, pDataIOPCItemMgt, hServerRailwayComposition, H_RAILWAY_COMPOSITION);
	AddItem(TEMPERATURE_ID, VT_R4, pDataIOPCItemMgt, hServerTemperature, H_TEMPERATURE);
	AddItem(ALARM_ID, VT_I4, pDataIOPCItemMgt, hServerAlarm, H_ALARM);
	AddItem(DATETIME_ID, VT_BSTR, pDataIOPCItemMgt, hServerDatetime, H_DATETIME);

	// Repeat the process, but for the items to be written to.
	printf("Adding the group of data do be written to synchronously as inactive... \n");
	AddGroup(HEATBOX_PARAMS_GROUP_NAME, pIOPCServer, pParamsIOPCItemMgt, hServerHeatboxParamsGroup);

	AddItem(INDICATOR_IDENTIFIER_ID, VT_I1, pParamsIOPCItemMgt, hServerIndicatorIdentifier, 1);
	AddItem(INDICATOR_TYPE_ID, VT_I2, pParamsIOPCItemMgt, hServerIndicatorType, 1);
	AddItem(INDICATOR_STATE_ID, VT_BOOL, pParamsIOPCItemMgt, hServerIndicatorState, 1);

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

	// Loop waiting for requests from the socket server.
	// TODO: wait for escape sequence to break out of the while loop
	while (1) {
		// Check if there's a write request. If not, loop over.
		if (data->writeReqState == WRITE_REQUESTED) {
			WaitForSingleObject(data->paramsMutex, INFINITE);
			data->paramsMutexOwner = OPC_CLIENT;
			data->writeReqState = WRITING;

			printf("----------------------------------------------------------------\n");
			printf("Data written synchronously to the OPC Server\n");

			// Create variants and write the values to the OPC Server.
			HotboxParams *params = &data->hotboxParams;
			VARIANT identifier, type, state;

			VariantInit(&identifier);
			identifier.iVal = params->indicatorIdentifier;
			identifier.vt = VT_I1;
			WriteItem(pParamsIOPCItemMgt, hServerIndicatorIdentifier, identifier, pIOPCServer);
			printf("Indicator identifier: %d\n", params->indicatorIdentifier);

			VariantInit(&type);
			type.intVal = params->indicatorType;
			type.vt = VT_I2;
			WriteItem(pParamsIOPCItemMgt, hServerIndicatorType, type, pIOPCServer);
			printf("Indicator type: %d\n", params->indicatorType);

			VariantInit(&state);
			state.intVal = params->indicatorState;
			state.vt = VT_BOOL;
			WriteItem(pParamsIOPCItemMgt, hServerIndicatorState, state, pIOPCServer);
			printf("Indicator state: %s\n", params->indicatorType ? "TRUE" : "FALSE");

			printf("----------------------------------------------------------------\n");

			// Declare the writing finished and free the mutex.
			data->writeReqState = WRITE_FINISHED;
			data->paramsMutexOwner = NO_OWNER;
			ReleaseMutex(data->paramsMutex);
		}
	}

	// Cancel the callback and release its reference
	printf("Cancelling the IOPCDataCallback notifications...\n");
	CancelDataCallback(pIConnectionPoint, dwCookie);
	//pIConnectionPoint->Release();
	pSOCDataCallback->Release();

	// Remove the OPC groups with their items:
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
// Add group <groupName> to the Server whose IOPCServer interface
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

void AddItem(wchar_t* itemName, VARTYPE itemType, IOPCItemMgt* pIOPCItemMgt,
	OPCHANDLE &hServerItem, OPCHANDLE clientHandle)
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

///////////////////////////////////////////////////////////////////////////////
// Write to device the value of the item having the "hServerItem" server 
// handle and belonging to the group whose one interface is pointed by
// pGroupIUnknown.
//
void WriteItem(IUnknown* pGroupIUnknown, OPCHANDLE hServerItem, VARIANT varValue, IOPCServer* pIOPCServer)
{
	// get a pointer to the IOPCSyncIOInterface:
	IOPCSyncIO* pIOPCSyncIO;
	pGroupIUnknown->QueryInterface(__uuidof(pIOPCSyncIO), (void**)&pIOPCSyncIO);

	// write the item value to the device:
	HRESULT* pErrors = NULL; //to store error code(s)
	HRESULT hr = pIOPCSyncIO->Write(1, &hServerItem, &varValue, &pErrors);

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
