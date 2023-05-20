#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <string>

typedef struct HotboxData {
	short hotboxIdentifier;
	std::string railwayComposition;
	float temperature;
	long alarm;
	std::string datetime;
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
