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

#ifndef OPC_CLIENT_H
#define OPC_CLIENT_H

IOPCServer *InstantiateServer(wchar_t ServerName[]);
void AddGroup(wchar_t* groupName, IOPCServer* pIOPCServer, IOPCItemMgt* &pIOPCItemMgt,
	OPCHANDLE& hServerGroup);
void AddItem(wchar_t* itemName, VARTYPE itemType, IOPCItemMgt* pIOPCItemMgt, OPCHANDLE &hServerItem, OPCHANDLE clientHandle);
void ReadItem(IUnknown* pGroupIUnknown, OPCHANDLE hServerItem, VARIANT& varValue);
void RemoveItem(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE hServerItem);
void RemoveGroup(IOPCServer* pIOPCServer, OPCHANDLE hServerGroup);
void WriteItem(IUnknown* pGroupIUnknown, OPCHANDLE hServerItem, VARIANT varValue, IOPCServer* pIOPCServer);


#endif // OPC_CLIENT_H not defined
