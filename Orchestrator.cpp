#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#include "Orchestrator.h"
#include "OPCClient.h"

DWORD WINAPI opcClientFactory();
DWORD WINAPI socketServerFactory();

DataForThreads dataForThreads;

int main(void)
{
	HANDLE socketServerFactoryHandle, opcClientFactoryHandle;
	DWORD socketServerFactoryThreadID, opcClientFactoryThreadID;

	// Initialize the data structures used for communication between threads.
	dataForThreads.dataMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL               // unnamed mutex
	);
	dataForThreads.paramsMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL               // unnamed mutex
	);
	if (dataForThreads.dataMutex == NULL || dataForThreads.paramsMutex == NULL)
	{
		printf("CreateMutex error: %d\n", GetLastError());
		return 1;
	}
	dataForThreads.dataMutexOwner = NO_OWNER;
	dataForThreads.paramsMutexOwner = NO_OWNER;
	dataForThreads.writeReqState = NO_REQ;
	
	// Create thread that runs the socketServerFactory function.
	socketServerFactoryHandle = CreateThread(
		NULL,						   // default security attributes
		0,							   // default stack size
		(LPTHREAD_START_ROUTINE)socketServerFactory,
		NULL,						   // thread function arguments
		0,							   // default creation flags
		&socketServerFactoryThreadID   // receive thread identifier
	);
	// Create thread that runs the opcClientFactory function.
	opcClientFactoryHandle = CreateThread(
		NULL,						   // default security attributes
		0,							   // default stack size
		(LPTHREAD_START_ROUTINE)opcClientFactory,
		NULL,						   // thread function arguments
		0,							   // default creation flags
		&opcClientFactoryThreadID      // receive thread identifier
	);
	// Wait for both threads to finish.
	HANDLE threadHandles[2] = { socketServerFactoryHandle, opcClientFactoryHandle };
	WaitForMultipleObjects(2, threadHandles, TRUE, INFINITE);
	// Free memory resources.
	CloseHandle(socketServerFactoryHandle);
	CloseHandle(opcClientFactoryHandle);
	CloseHandle(dataForThreads.dataMutex);
	CloseHandle(dataForThreads.paramsMutex);
}

DWORD WINAPI socketServerFactory()
{
	// MOCK SOCKET SERVER
	// Issues periodic read and write requests to verify the OPC Client is working.
	short i = 0;
	while (1) {
		// Part A: reads
		// Waits for data to be read to be available and claims ownership of the mutex.
		WaitForSingleObject(dataForThreads.dataMutex, INFINITE);
		dataForThreads.dataMutexOwner = SOCKET_SERVER;

		// Accesses the data, formats it and outputs it to the console.
		HotboxData data = dataForThreads.hotboxData;

		char strToPrint[100];
		strncpy(strToPrint, data.railwayComposition, 10);
		strToPrint[10] = 0;

		float floatToPrint = ((int)data.temperature) % 10000 + (data.temperature - ((int)data.temperature));

		printf("09|%06d|%03d|%-10s|%06.1f|%02d|%s\n",
			1 % 1000000,
			data.hotboxIdentifier % 1000,
			strToPrint,
			floatToPrint,
			data.alarm % 100,
			data.datetime);

		// Frees access to the data.
		dataForThreads.dataMutexOwner = NO_OWNER;
		ReleaseMutex(dataForThreads.dataMutex);

		Sleep(1000);

		// Part B: writes

		// Waits for writing data structure to be free and claims ownership of the mutex.
		WaitForSingleObject(dataForThreads.paramsMutex, INFINITE);
		dataForThreads.paramsMutexOwner = SOCKET_SERVER;

		// Updates the data structure to have the values we want to write to the server.
		dataForThreads.hotboxParams.indicatorIdentifier = ++i;
		dataForThreads.hotboxParams.indicatorType = i;
		dataForThreads.hotboxParams.indicatorState = !dataForThreads.hotboxParams.indicatorState;

		// Frees the mutex, so that the OPCClient can gain access to the data structure.
		dataForThreads.paramsMutexOwner = NO_OWNER;
		ReleaseMutex(dataForThreads.paramsMutex);

		// Makes the write request and waits for it to be finished.
		// TODO: detect when the OPC Client thread finishes in the middle of the 
		//		 process and recover.
		dataForThreads.writeReqState = WRITE_REQUESTED;
		while (dataForThreads.writeReqState != WRITE_FINISHED);

		// Finishes the request process.
		dataForThreads.writeReqState = NO_REQ;
		
		Sleep(1000);

		// Create the thread that runs the socket server
		// Wait for the thread to exit. 
		// When it does, it loops over and creates a new one, making sure we recover from failure.
	}
}

DWORD WINAPI opcClientFactory()
{
	HANDLE opcClientHandle;
	DWORD opcClientThreadId;
	while (1) {
		// Create the thread that runs the OPC Client
		opcClientHandle = CreateThread(
			NULL,						   // default security attributes
			0,							   // default stack size
			(LPTHREAD_START_ROUTINE)OpcClient,
			&dataForThreads,			   // thread function arguments
			0,							   // default creation flags
			&opcClientThreadId			   // receive thread identifier
		);
		// Wait for the thread to exit.
		WaitForSingleObject(opcClientHandle, INFINITE);
		// Check if Mutex was corrupted and, if so, restore it
		if (dataForThreads.dataMutexOwner == OPC_CLIENT) {
			ReleaseMutex(dataForThreads.dataMutex);
			dataForThreads.dataMutex = CreateMutex(NULL, FALSE, NULL);
			dataForThreads.dataMutexOwner = NO_OWNER;
		}
		if (dataForThreads.paramsMutexOwner == OPC_CLIENT) {
			ReleaseMutex(dataForThreads.paramsMutex);
			dataForThreads.paramsMutex = CreateMutex(NULL, FALSE, NULL);
			dataForThreads.paramsMutexOwner = NO_OWNER;
		}
		// Loop over and create a new thread, making sure we recover from failure.
	}
}