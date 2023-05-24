#include <string>

#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

typedef struct HotboxData {
	short hotboxIdentifier;
	char railwayComposition[100];
	float temperature;
	long alarm;
	char datetime[100];
} HotboxData;

typedef enum {
	SOCKET_SERVER,
	OPC_CLIENT
} MutexOwner;

typedef struct DataForThreads {
	HotboxData hotboxData;
	MutexOwner mutexOwner;
	HANDLE ghMutex;
} DataForThreads;

#endif ORCHESTRATOR_H
