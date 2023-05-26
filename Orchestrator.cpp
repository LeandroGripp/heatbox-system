#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#include "Orchestrator.h"
#include "OPCClient.h"
#include "SocketServer.h"

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


/* NOTAS DO GABRIEL
* 1.E preciso testar o caso em que o mutex está sob posse de alguem e a thread encerra inesperadamente.
*	PRECISA TRATAR ESSE ERRO NA FUNCAO QUE RECBEU O MUTEX ABANDONADO! A OUTRA THREAD PODE ATRASAR E NAO ENTREGAR
*	SLIDE 42 DE MULTITHREADING I (ATR)
*	SLIDE 13 DE MULTITHREADING II (ATR)
* 2.E preciso colocar o contador de numero sequencial de mensagens
* 3.Para entregar a escrita para o OPC Client basta escrever no struct DataForThreads?
* 4.Entender melhor como o OpcCLient conseguiu identificar a estrutura DataForThreads se ela nao foi declarada
* 5.Esperar infinitamente pelo mutex depende de ngm travar na secao critica
* 6.Simplesmente colocar uma mensagem de erro indicando que a thread encerrou e tentar cria-la de novo?
* 7.Indicador de posse do mutex e apenas para depuracao?
* 8.Pode recriar o mutex direto? Provavelmente vai so renovar o handle pq o mutex nao e destruido
*	"ERROR_ALREADY_EXISTS" no retorno de CreateMutex()
*/

DWORD WINAPI socketServerFactory()
{
	// SOCKET SERVER
	// Issues periodic read and write requests to verify the OPC Client is working.
	HANDLE SocketServerHandle;
	DWORD SocketServerThreadId;
	while (1) {
		// Create the thread that runs the Socket Server
		SocketServerHandle = CreateThread(
			NULL,						   // default security attributes
			0,							   // default stack size
			(LPTHREAD_START_ROUTINE)SocketServer,
			&dataForThreads,			   // thread function arguments
			0,							   // default creation flags
			&SocketServerThreadId			   // receive thread identifier
		);
		// Wait for the thread to exit.
		WaitForSingleObject(SocketServerHandle, INFINITE);
		// Check if Mutex was corrupted and, if so, restore it
		if (dataForThreads.dataMutexOwner == SOCKET_SERVER) {
			ReleaseMutex(dataForThreads.dataMutex);
			dataForThreads.dataMutex = CreateMutex(NULL, FALSE, NULL);
			dataForThreads.dataMutexOwner = NO_OWNER;
		}
		if (dataForThreads.paramsMutexOwner == SOCKET_SERVER) {
			ReleaseMutex(dataForThreads.paramsMutex);
			dataForThreads.paramsMutex = CreateMutex(NULL, FALSE, NULL);
			dataForThreads.paramsMutexOwner = NO_OWNER;
		}
		// Loop over and create a new thread, making sure we recover from failure.
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