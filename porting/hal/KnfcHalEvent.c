/*
 * KnfcHalEvent.c
 *
 *  Created on: Jun 24, 2013
 *      Author: youngsun
 */

#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "KnfcHalEvent.h"
#include "KnfcHalImpl.h"
#include "KnfcDebug.h"
#include "KnfcList.h"

//////////////////////////////////////////////////
// Static Variables
//////////////////////////////////////////////////

// A registry for the lazy events
static KnfcNode_t *g_pLazyEventRegistry = NULL;

// A handle to the lazy event handler thread
static pthread_t g_hLazyEventHandlerThread = NULL;

// if it is false, the lazy event handler will be stopped.
static bool_t g_bLazyEventHandlerFlag = W_FALSE; 

//////////////////////////////////////////////////
// Static Function Declaration
//////////////////////////////////////////////////

static bool_t CheckSameLazyEvent(KnfcLazyEvent_t *pEvent1, KnfcLazyEvent_t *pEvent2);

static KnfcLazyEvent_t *FindLazyEventRegistry(KnfcLazyEvent_t *pEvent);

static W_ERROR AddLazyEventRegistry(KnfcLazyEvent_t *pEvent);

static W_ERROR RemoveLazyEventRegistry(KnfcLazyEvent_t *pEvent);

static W_ERROR StartLazyEventHandlerThread(void *pInstance);

static W_ERROR StopLazyEventHandlerThread();

static void LazyEventHandlerThread(void *pContext);

static W_ERROR ExecuteRegisteredEventHandler(void *pContext, KnfcRespMessage_t *respMessage);

//////////////////////////////////////////////////
// Static Function Definition
//////////////////////////////////////////////////

/**
 * This function is designed to deeply compare the two lazy events: pEvent & pEvent2.
 */
 
static bool_t
CheckSameLazyEvent(KnfcLazyEvent_t *pEvent1, KnfcLazyEvent_t *pEvent2) {

#define COMPARE_LAZY_EVENT_ELEMENT(ELEM1, ELEM2)		\
if((ELEM1 != ELEM2)) { return W_FALSE; }				

	COMPARE_LAZY_EVENT_ELEMENT(pEvent1->nEventCode,	pEvent2->nEventCode);
	COMPARE_LAZY_EVENT_ELEMENT(pEvent1->pParam1, pEvent2->pParam1);
	COMPARE_LAZY_EVENT_ELEMENT(pEvent1->pParam2, pEvent2->pParam2);

	//YOUNGSUN - CHKME
	//Comparing the following elements is meaningless.
	//COMPARE_LAZY_EVENT_ELEMENT(pEvent1->pContext, pEvent2->pContext);
	//COMPARE_LAZY_EVENT_ELEMENT(pEvent1->pCallback, pEvent2->pCallback);	
	//COMPARE_LAZY_EVENT_ELEMENT(pEvent1->pCbParam, pEvent2->pCbParam);		
	//COMPARE_LAZY_EVENT_ELEMENT(pEvent1->pBuffer, pEvent2->pBuffer);	
	//COMPARE_LAZY_EVENT_ELEMENT(pEvent1->nBufferLength, pEvent2->nBufferLength);		
	//COMPARE_LAZY_EVENT_ELEMENT(pEvent1->pParam3, pEvent2->pParam3);
	//COMPARE_LAZY_EVENT_ELEMENT(pEvent1->pParam4, pEvent2->pParam4);
	//COMPARE_LAZY_EVENT_ELEMENT(pEvent1->pParam5, pEvent2->pParam5);
	return W_TRUE;
}

/**
 * This function will find the previously registered lazy event, exactly same as the given event, and return.
 */
 
static KnfcLazyEvent_t *
FindLazyEventRegistry(KnfcLazyEvent_t *pEvent) {

	uint32_t i;
	KnfcLazyEvent_t *event;
	KnfcNode_t *node = g_pLazyEventRegistry;

	for(; node != NULL; node = node->pNext) {
		if((event = (KnfcLazyEvent_t *)node->pItem) != NULL) {
			if(CheckSameLazyEvent(event, pEvent) == W_TRUE) {
			    return event;
			}
		}	
	}

	return NULL;
}

/**
 * This function is used to insert a new lazy event to the registry.
 */

static W_ERROR
AddLazyEventRegistry(KnfcLazyEvent_t *pEvent) {

	if(KnfcInsertNode(g_pLazyEventRegistry, pEvent) == NULL) {
		return W_ERROR_BAD_HANDLE;
	}

	return W_SUCCESS;
}

/**
 * This function is used to remove the given lazy event from the registry.
 */

static W_ERROR
RemoveLazyEventRegistry(KnfcLazyEvent_t *pEvent) {

	KnfcNode_t *node = KnfcDeleteNode(g_pLazyEventRegistry, pEvent);

	if(node == NULL) {
		return W_ERROR_BAD_HANDLE;
	}

	CMemoryFree(node->pItem);
	CMemoryFree(node);

	return W_SUCCESS;
}

/**
 * Start a thread for handling the currently registered lazy events
 */
 
static W_ERROR
StartLazyEventHandlerThread(void *pContext) {

	if(g_hLazyEventHandlerThread != NULL) {
		return W_SUCCESS;
	}

	g_bLazyEventHandlerFlag = W_TRUE;
	
	if(pthread_create(&g_hLazyEventHandlerThread, NULL,  &LazyEventHandlerThread, pContext) < 0) {
		KNFC_DEBUG("Creating LazyEventHandlerThread has been forced to quit."); 
		return W_ERROR_START_LAZY_EVENT_HANDLER;
	}

	return W_SUCCESS;
}

/**
 * Stop the thread for the registered lazy events
 */

static W_ERROR
StopLazyEventHandlerThread() {

	if(g_hLazyEventHandlerThread == NULL) {
		return W_SUCCESS;
	}

	g_bLazyEventHandlerFlag = W_FALSE;
	
#ifndef USE_ANDROID_NDK
	// YOUNGSUN - FIXME
	// We have to work-around the following code for Android, 
	// but currently we are going to just leave the problem due to the lack of the time. 
	// We don't need to stop the lazy event handler before turning off the NFC.
	if(pthread_cancel(g_hLazyEventHandlerThread) < 0) {
		KNFC_DEBUG("Stopping LazyEventHandlerThread has been forced to quit."); 
		return W_ERROR_STOP_LAZY_EVENT_HANDLER;
	}
#endif

	g_hLazyEventHandlerThread = NULL;
	return W_SUCCESS;
}

/*
 * Execute the registered event handler
 */
 
static W_ERROR
ExecuteRegisteredEventHandler(void *pContext, KnfcRespMessage_t *pRespMessage) {

	KnfcLazyEvent_t lazyEvent;
	KnfcLazyEvent_t *foundEvent;
	uint32_t respBufferOffset = 0;
	
	CMemoryFill(&lazyEvent, 0, sizeof(KnfcLazyEvent_t));

	lazyEvent.nEventCode = pRespMessage->header.nCode;
	lazyEvent.pContext = pContext;

	KNFC_DEBUG("ExecuteRegisteredEventHandler: %d", lazyEvent.nEventCode);

	// Preprocess the response message to find the previously registered lazy event.
	switch(lazyEvent.nEventCode) {
		case P_Idenfier_PReaderErrorEventRegister: {
			lazyEvent.pParam1 = pRespMessage->pPayload[0];
			break;
		}

		case P_Idenfier_PReaderDriverRegister: {
			
			struct DetectedConnectionInfo_s {
				uint8_t nPriority;
				uint32_t nDriverProtocol;
				uint32_t nDetectedProtocol;
				uint32_t hConnection;
				uint32_t nBufferLength;
			} connInfo;

			respBufferOffset = sizeof(struct DetectedConnectionInfo_s);
			
			CMemoryCopy(&connInfo, &pRespMessage->pPayload[0], respBufferOffset);
			
			lazyEvent.pParam1 = connInfo.nPriority;
			lazyEvent.pParam2 = connInfo.nDriverProtocol;
			lazyEvent.pParam3 = connInfo.nDetectedProtocol;
			lazyEvent.pParam4 = connInfo.hConnection;
			lazyEvent.pParam5 = connInfo.nBufferLength;
			break;
		}

		case P_Idenfier_P14P4DriverExchangeData: {
			
			struct DataExchangeInfo_s {
				uint32_t hConnection;
				uint32_t nBufferLength;				
			} xchgInfo;

			respBufferOffset = sizeof(struct DataExchangeInfo_s);
			CMemoryCopy(&xchgInfo, &pRespMessage->pPayload[0], respBufferOffset);

			lazyEvent.pParam1 = xchgInfo.hConnection;
			lazyEvent.pParam2 = 0;
			lazyEvent.pParam3 = xchgInfo.nBufferLength;
			break;
		}

		case P_Idenfier_P14P3DriverExchangeData: {
			
			struct DataExchangeInfo_s {
				uint32_t hConnection;
				uint32_t nBufferLength;				
			} xchgInfo;

			respBufferOffset = sizeof(struct DataExchangeInfo_s);
			CMemoryCopy(&xchgInfo, &pRespMessage->pPayload[0], respBufferOffset);

			lazyEvent.pParam1 = xchgInfo.hConnection;
			lazyEvent.pParam2 = 0;
			lazyEvent.pParam3 = xchgInfo.nBufferLength;
			break;
		}
		
		case P_Idenfier_PEmulOpenConnectionDriver2Ex: {
			
			struct EmulEventInfo_s {
				uint32_t hHandle;			
				uint32_t nEventCode;
			} eventInfo;

			respBufferOffset = sizeof(struct EmulEventInfo_s);
			CMemoryCopy(&eventInfo, &pRespMessage->pPayload[0], respBufferOffset);

			lazyEvent.pParam1 = eventInfo.hHandle;	
			lazyEvent.pParam2 = 0;
			lazyEvent.pParam3 = eventInfo.nEventCode;
			break;
		}

		case P_Idenfier_PEmulOpenConnectionDriver3Ex: {
			
			struct EmulCmdInfo_s {
				uint32_t hHandle;			
				uint32_t nCmdLength;
			} cmdInfo;

			respBufferOffset = sizeof(struct EmulCmdInfo_s);
			CMemoryCopy(&cmdInfo, &pRespMessage->pPayload[0], respBufferOffset);

			lazyEvent.pParam1 = cmdInfo.hHandle;	
			lazyEvent.pParam2 = 0;
			lazyEvent.pParam3 = cmdInfo.nCmdLength;
			break;
		}		
	}

	foundEvent = FindLazyEventRegistry(&lazyEvent);

	if(foundEvent == NULL) {
		return W_ERROR_EVENT_HANDLER_NOT_FOUND;
	}

	// Execute the callback function of the registered lazy event.
	switch(foundEvent->nEventCode) {
		case P_Idenfier_PReaderErrorEventRegister: {
			
			tPBasicGenericEventHandler *pCallback;
			
			pCallback = (tPBasicGenericEventHandler *)foundEvent->pCallback;
			pCallback(foundEvent->pContext, 
				foundEvent->pCbParam, 
				foundEvent->pParam1);
			break;
		}

		case P_Idenfier_PReaderDriverRegister: {
			
			tPReaderDriverRegisterCompleted *pCallback;
			uint32_t nRespBufferLength = (uint32_t)lazyEvent.pParam5;
			
			if(foundEvent->nBufferLength < nRespBufferLength) {
				return W_ERROR_BUFFER_TOO_SHORT;
			}

			CMemoryCopy(foundEvent->pBuffer, &pRespMessage->pPayload[respBufferOffset], nRespBufferLength);
			
			pCallback = (tPReaderDriverRegisterCompleted *)foundEvent->pCallback;
			pCallback(foundEvent->pContext, 
				foundEvent->pCbParam, 
				lazyEvent.pParam3,
				lazyEvent.pParam4,
				lazyEvent.pParam5,
				W_FALSE);	//<- YOUNGSUN - CHKME
			break;
		}

		case P_Idenfier_P14P4DriverExchangeData: {
			
			tPBasicGenericDataCallbackFunction *pCallback;
			uint32_t nRespBufferLength = (uint32_t)lazyEvent.pParam3;
			
			if(foundEvent->nBufferLength < nRespBufferLength) {
				return W_ERROR_BUFFER_TOO_SHORT;
			}

			CMemoryCopy(foundEvent->pBuffer, &pRespMessage->pPayload[respBufferOffset], nRespBufferLength);
			
			pCallback = (tPBasicGenericDataCallbackFunction *)foundEvent->pCallback;
			pCallback(foundEvent->pContext, 
				foundEvent->pCbParam, 
				lazyEvent.pParam3,
				W_SUCCESS);	//<- YOUNGSUN - CHKME

			// After exchanging data between PCD and PICC once, 
			// the lazy event for the operation is not needed any more.
			if(RemoveLazyEventRegistry(foundEvent) != W_SUCCESS) {
				return W_ERROR_ITEM_NOT_FOUND;
			}
			break;
		}

		case P_Idenfier_P14P3DriverExchangeData: {
			
			tPBasicGenericDataCallbackFunction *pCallback;
			uint32_t nRespBufferLength = (uint32_t)lazyEvent.pParam3;
			
			if(foundEvent->nBufferLength < nRespBufferLength) {
				return W_ERROR_BUFFER_TOO_SHORT;
			}

			CMemoryCopy(foundEvent->pBuffer, &pRespMessage->pPayload[respBufferOffset], nRespBufferLength);
			
			pCallback = (tPBasicGenericDataCallbackFunction *)foundEvent->pCallback;
			pCallback(foundEvent->pContext, 
				foundEvent->pCbParam, 
				lazyEvent.pParam3,
				W_SUCCESS);	//<- YOUNGSUN - CHKME

			// After exchanging data between PCD and PICC once, 
			// the lazy event for the operation is not needed any more.
			if(RemoveLazyEventRegistry(foundEvent) != W_SUCCESS) {
				return W_ERROR_ITEM_NOT_FOUND;
			}
			break;
		}		

		case P_Idenfier_PEmulOpenConnectionDriver2Ex: {

			tPEmulDriverEventReceived *pCallback = (tPEmulDriverEventReceived *)foundEvent->pCallback;
			pCallback(foundEvent->pContext, 
				foundEvent->pCbParam,
				lazyEvent.pParam3);	// EventCode
			break;
		}

		case P_Idenfier_PEmulOpenConnectionDriver3Ex: {

			tPEmulDriverCommandReceived *pCallback = (tPEmulDriverCommandReceived *)foundEvent->pCallback;
			pCallback(foundEvent->pContext, 
				foundEvent->pCbParam,
				lazyEvent.pParam3);	// the length of the command
			break;
		}		

		default: 
			KNFC_DEBUG("ExecuteRegisteredEventHandler - Unknown Event: %d", foundEvent->nEventCode);
			return W_ERROR_UNKNOWN_EVENT_CODE;
	}

	return W_SUCCESS;
}

//////////////////////////////////////////////////
// Global Function Definition
//////////////////////////////////////////////////

/** 
 * Initialize a Lazy Event Registry
 */
 
W_ERROR
Khal_IntializeLazyEventRegistry() {

	if(g_pLazyEventRegistry == NULL) {
		g_pLazyEventRegistry = KnfcNewList();
	}

	if(g_pLazyEventRegistry == NULL) {
		return W_ERROR_OUT_OF_RESOURCE;
	}

	g_hLazyEventHandlerThread = NULL;

	return W_SUCCESS;
}

/**
 * Register a new handler for the given lazy event to be executed when the lazy response will be received from the NFCC
 */

W_ERROR
Khal_RegisterLazyEvent(KnfcLazyEvent_t *pEvent) {

	W_ERROR error;

	if(g_pLazyEventRegistry == NULL) {	
		if((error = Khal_IntializeLazyEventRegistry()) != W_SUCCESS) {
			return error;
		}
	}
		
	// Check whether the event has been registered or not.
	if(FindLazyEventRegistry(pEvent)) {
		return W_SUCCESS;
	}

	error = AddLazyEventRegistry(pEvent);

	// Error check
	if(error != W_SUCCESS) {
		KNFC_DEBUG("Khal_RegisterLazyWaitAndProcessResponse: %d", error);
		return error;
	}

	// Start the thread for handling the lazy event
	return StartLazyEventHandlerThread(pEvent->pContext);
}

/**
 * Unregister the specified event 
 */
 
W_ERROR
Khal_UnregisterLazyEvent(KnfcLazyEvent_t *pEvent)  {

	W_ERROR error = W_SUCCESS;
	KnfcLazyEvent_t *event;

	KNFC_DEBUG("Khal_UnregisterLazyEvent called... %d", pEvent->nEventCode);

	if(pEvent == NULL) {
		error = W_ERROR_BAD_PARAMETER;
		return error;
	}
	
	event = FindLazyEventRegistry(pEvent);

	if(event != NULL) {
		KnfcNode_t *node = KnfcDeleteNode(g_pLazyEventRegistry, event);

		if(KnfcGetListSize(g_pLazyEventRegistry) == 0) {
			error = StopLazyEventHandlerThread();
		}

		CMemoryFree(node->pItem);
		CMemoryFree(node);
	}

	return error;
}

/** 
 * Finalize a Lazy Event Registry
 */
 
W_ERROR
Khal_FinalizeLazyEventRegistry() {

	if(g_pLazyEventRegistry != NULL) {
		KnfcDeleteList(g_pLazyEventRegistry);
	}

	return StopLazyEventHandlerThread();
}

//////////////////////////////////////////////////
// Thread Function Definition
//////////////////////////////////////////////////

// Wait the thread until the mutex will be unlocked.
#define PTHREAD_MUTEX_WAIT(MUTEX)	\
do{ 								\
	pthread_mutex_lock(MUTEX);  	\
	pthread_mutex_unlock(MUTEX);	\
}while(0)

/**
 * The is the thread function to handle the registered lazy events 
 * when the response messages for them are arrived from the NFCC.
 */

static void
LazyEventHandlerThread(void *pContext) {

	W_ERROR error;
	uint8_t lazyEventCode;
	uint32_t payloadLength;
	KnfcRespMessage_t respMessage;
	KnfcConnInstance_t *instance;

	KNFC_DEBUG("LazyEventHandlerThread Started...");
	
	pthread_mutex_init(&g_hWaitResponseMutex, NULL);
	instance = (KnfcConnInstance_t *)PContextGetUserInstance(pContext);

	while(g_bLazyEventHandlerFlag) {
		// Check whether the mutex has been already locked or not.
		// If locked, wait until it will be unlocked. otherwise, go through.
		PTHREAD_MUTEX_WAIT(&instance->hMutex);

		// Check if the given code is same as P_Idenfier_KhalLazyEvent.
		PUserRead(instance, &lazyEventCode, 1);

		if(lazyEventCode != P_Idenfier_KhalLazyEvent) {
			KNFC_DEBUG("Retry to read a lazy event response ... 0x%X", lazyEventCode);					
			PTHREAD_MUTEX_WAIT(&g_hWaitResponseMutex);
			continue;
		}

		// Receive the response header
		error = PUserRead(instance, &respMessage.header, sizeof(KnfcRespHeader_t));

		if(error != W_SUCCESS) {
			goto LEHT_CLEAN_AND_RETURN;
		}

		payloadLength = respMessage.header.nLength;
		respMessage.pPayload = (uint8_t*)CMemoryAlloc(payloadLength);

		if(respMessage.pPayload == NULL) {
			error = W_ERROR_OUT_OF_RESOURCE;
			goto LEHT_CLEAN_AND_RETURN;
		}

		// Receive the response body
		error = PUserRead(instance, respMessage.pPayload, payloadLength);

		if(error != W_SUCCESS) {
			goto LEHT_CLEAN_AND_RETURN;
		}

		// Execute the registered event handler
		error = ExecuteRegisteredEventHandler(pContext, &respMessage);
			
LEHT_CLEAN_AND_RETURN:

		if(error != W_SUCCESS) {
			KNFC_DEBUG("LazyEventHandlerThread Error: %d", error);
		}

		CMemoryFree(respMessage.pPayload);
	}
}

