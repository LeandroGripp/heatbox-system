#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#include "Orchestrator.h"

DWORD WINAPI opcClientFactory(LPVOID dataForThreads);
DWORD WINAPI socketServerFactory(LPVOID dataForThreads);

DataForThreads dataForThreads;

int main(void)
{
	HANDLE socketServerFactoryHandle, opcClientFactoryHandle;
	DWORD socketServerFactoryThreadID, opcClientFactoryThreadID;

	// Initialize the data structures used for communication between threads.
	dataForThreads.ghMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL               // unnamed mutex
	);
	if (dataForThreads.ghMutex == NULL)
	{
		printf("CreateMutex error: %d\n", GetLastError());
		return 1;
	}
	
	// Create thread that runs the socketServerFactory function.
	socketServerFactoryHandle = CreateThread(
		NULL,						   // default security attributes
		0,							   // default stack size
		(LPTHREAD_START_ROUTINE)socketServerFactory,
		&dataForThreads,			   // thread function arguments
		0,							   // default creation flags
		&socketServerFactoryThreadID   // receive thread identifier
	);
	// Create thread that runs the opcClientFactory function.
	opcClientFactoryHandle = CreateThread(
		NULL,						   // default security attributes
		0,							   // default stack size
		(LPTHREAD_START_ROUTINE)opcClientFactory,
		&dataForThreads,			   // thread function arguments
		0,							   // default creation flags
		&opcClientFactoryThreadID      // receive thread identifier
	);
	// Wait for both threads to finish.
	HANDLE threadHandles[2] = { socketServerFactoryHandle, opcClientFactoryHandle };
	WaitForMultipleObjects(2, threadHandles, TRUE, INFINITE);
	// Free memory resources.
	CloseHandle(socketServerFactoryHandle);
	CloseHandle(opcClientFactoryHandle);
	//CloseHandle(ghMutex);
}

DWORD WINAPI socketServerFactory(LPVOID dataForThreads)
{
	DataForThreads *data;
	data = (DataForThreads*)dataForThreads;
	short i = 0;
	while (1) {
		printf("Thread 1 \n");
		Sleep(1000);
		printf("Thread 1 \n");
		data->hotboxData.hotboxIdentifier = i++;
		Sleep(1000);
		// Create the thread that runs the socket server
		// Wait for the thread to exit. 
		// When it does, it loops over and creates a new one, making sure we recover from failure.
	}
}

DWORD WINAPI opcClientFactory(LPVOID dataForThreads)
{
	DataForThreads *data;
	data = (DataForThreads*)dataForThreads;
	while (1) {
		printf("Thread 2 \n");
		printf("%d \n", data->hotboxData.hotboxIdentifier);
		Sleep(2000);
		// Create the thread that runs the OPC Client
		// Wait for the thread to exit. 
		// When it does, it loops over and creates a new one, making sure we recover from failure.
	}
}