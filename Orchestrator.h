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

typedef struct HotboxParams {
	short indicatorIdentifier;
	short indicatorType;
	bool indicatorState;
} HotboxParams;

typedef enum {
	NO_OWNER,
	SOCKET_SERVER,
	OPC_CLIENT
} MutexOwner;

typedef enum {
	NO_REQ,
	WRITE_REQUESTED,
	WRITING,
	WRITE_FINISHED
} WriteReqState;

typedef struct DataForThreads {
	HotboxData hotboxData;
	MutexOwner dataMutexOwner;
	HANDLE dataMutex;

	HotboxParams hotboxParams;
	MutexOwner paramsMutexOwner;
	HANDLE paramsMutex;
	WriteReqState writeReqState;
} DataForThreads;

#endif ORCHESTRATOR_H
