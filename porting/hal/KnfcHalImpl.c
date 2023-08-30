/*
 * KnfcHalImpl.c
 *
 *  Created on: Jun 24, 2013
 *      Author: youngsun
 */

#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "KnfcHalImpl.h"
#include "KnfcDebug.h"

//////////////////////////////////////////////////
// Global Variable Definitions
//////////////////////////////////////////////////

pthread_mutex_t g_hWaitResponseMutex;

//////////////////////////////////////////////////
// Static Function Definitions
//////////////////////////////////////////////////

/**
 * This function is used to send additional parameter data to the firmware.
 */

#ifdef INCLUDE_DEPRECATED_FUNCTIONS

W_ERROR
SendExtraParameter(void *pContext, uint8_t nParamID, uint8_t *pBuffer, uint32_t nBufferLength) {

	W_ERROR error;
	KhstParamHeader_t header;
	void *pInstance = PContextGetUserInstance(pContext);

	if(pInstance == NULL || pBuffer == NULL || nBufferLength == 0) {
		return W_ERROR_BAD_PARAMETER;
	}

	header.nCode = P_Idenfier_KhalParameter0 + nParamID;
	header.nLength = nBufferLength;

	// send the header for the parameter message
	error = PUserWrite(pInstance, &header, sizeof(KhstParamHeader_t));

	if(error != W_SUCCESS) {
		return error;
	}

	// send the message body
	return PUserWrite(pInstance, pBuffer, nBufferLength);
}

#endif

//////////////////////////////////////////////////
// Knfc Hardware Abstract Layer Interface
//////////////////////////////////////////////////

/**
 * This function is designed to perform the initialization for the KNFC hardware abtract layer.
 */
 
W_ERROR
Khal_Initialize() {
	Khal_IntializeLazyEventRegistry();
	return W_SUCCESS;
}

/**
 * This function will wait and process the reponse from the NFCC.
 */

W_ERROR
Khal_WaitAndProcessResponse(uint8_t nCode, tContext *pContext, void *pParam) {

	W_ERROR error;
	uint32_t payloadLength;	
	KnfcRespMessage_t respMessage;
	void *instance = PContextGetUserInstance(pContext);

	KNFC_DEBUG("Khal_WaitAndProcessResponse called... %d", nCode);

	// Receive the response header from the NFCC
	error = PUserRead(instance, &respMessage.header, sizeof(KnfcRespHeader_t));

	if(error != W_SUCCESS) {
		goto KWAP_CLEAN_AND_RETURN;
	}

	if(nCode != respMessage.header.nCode) {
		KNFC_DEBUG("Khal_WaitAndProcessResponse - Wrong Response: %d, %d", nCode, respMessage.header.nCode);
		error = W_ERROR_WRONG_RESPONSE;
		goto KWAP_CLEAN_AND_RETURN;
	}

	payloadLength = respMessage.header.nLength;

	respMessage.pPayload = (uint8_t*)CMemoryAlloc(payloadLength);

	if(respMessage.pPayload == NULL) {
		error = W_ERROR_OUT_OF_RESOURCE;
		goto KWAP_CLEAN_AND_RETURN;
	}

	// Receive the response body from the NFCC
	error = PUserRead(instance, respMessage.pPayload, payloadLength);

	if(error != W_SUCCESS) {
		goto KWAP_CLEAN_AND_RETURN;
	}

	switch(nCode) {
		case P_Idenfier_PBasicDriverGetVersion: {
			tMessage_in_PBasicDriverGetVersion *param;
			param = (tMessage_in_PBasicDriverGetVersion *)pParam;
			CMemoryCopy(param->pBuffer, respMessage.pPayload, payloadLength);
			param->nBufferSize = payloadLength;
			break;
		}

		case P_Idenfier_PNFCControllerDriverReadInfo: {
			tMessage_in_PNFCControllerDriverReadInfo *param;
			param = (tMessage_in_PNFCControllerDriverReadInfo *)pParam;
			CMemoryCopy(param->pBuffer, respMessage.pPayload, payloadLength);
			param->nBufferSize = payloadLength;
			break;
		}

		case P_Idenfier_PEmulOpenConnectionDriver1Ex: {
			tMessage_in_PEmulOpenConnectionDriver1Ex *param;
			param = (tMessage_in_PEmulOpenConnectionDriver1Ex *)pParam;
			CMemoryCopy((uint8_t *)param->phHandle, respMessage.pPayload, payloadLength);

			// Execute the Callback
			param->pOpenCallback(pContext, param->pOpenCallbackParameter, W_SUCCESS);
			break;
		}

		case P_Idenfier_PEmulGetMessageData: {
			tMessage_in_PEmulGetMessageData *param;
			param = (tMessage_in_PEmulGetMessageData *)pParam;

			// The validity of the data length will be checked in the firmware.
			CMemoryCopy(param->pDataBuffer, respMessage.pPayload, payloadLength);
			*(param->pnActualDataLength) = payloadLength;
			break;
		}

		case P_Idenfier_PNFCControllerGetMode: 
		case P_Idenfier_PReaderErrorEventRegister: 
			// Do Nothing
			break;			

		default: 
			KNFC_DEBUG("Unknown Command Code: %d", nCode);
			break;
	}

KWAP_CLEAN_AND_RETURN:

	pthread_mutex_unlock(&g_hWaitResponseMutex);

	if(error != W_SUCCESS) {
		KNFC_DEBUG("Khal_WaitAndProcessResponse Error: %d", error);
	}

	CMemoryFree(respMessage.pPayload);
	return error;
}

/**
 * This function will be used to register the event handlers for the responses from the NFCC.
 */
 
W_ERROR
Khal_LazyWaitAndProcessResponse(uint8_t nCode, tContext *pContext, void *pParam) {

	W_ERROR error = W_SUCCESS;
	KnfcLazyEvent_t *lazyEvent = NULL;

	KNFC_DEBUG("Khal_LazyWaitAndProcessResponse called... %d", nCode);

	lazyEvent = (KnfcLazyEvent_t *)CMemoryAlloc(sizeof(KnfcLazyEvent_t));

	if(lazyEvent == NULL) {
		return W_ERROR_OUT_OF_RESOURCE;
	}

	CMemoryFill(lazyEvent, 0, sizeof(KnfcLazyEvent_t));
	
	lazyEvent->nEventCode = nCode;
	lazyEvent->pContext = pContext;

	switch(nCode) {
		case P_Idenfier_PReaderErrorEventRegister: {
			tMessage_in_PReaderErrorEventRegister *param;
			param = (tMessage_in_PReaderErrorEventRegister *)pParam;

			lazyEvent->pCallback = param->pHandler;
			lazyEvent->pCbParam = param->pHandlerParameter;
			lazyEvent->pParam1 = param->nEventType;
			lazyEvent->pParam2 = 0;
			break;
		}
		
		case P_Idenfier_PReaderDriverRegister: {
			tMessage_in_PReaderDriverRegister *param;
			param = (tMessage_in_PReaderDriverRegister *)pParam;

			lazyEvent->pCallback = param->pCallback;
			lazyEvent->pCbParam = param->pCallbackParameter;
			lazyEvent->pBuffer = param->pBuffer;
			lazyEvent->nBufferLength = param->nBufferMaxLength;

			lazyEvent->pParam1 = param->nPriority;				//nPriority
			lazyEvent->pParam2 = param->nRequestedProtocolsBF;	//nDriverProtocol
			break;
		}
		
		case P_Idenfier_P14P4DriverExchangeData: {
			tMessage_in_P14P4DriverExchangeData *param;
			param = (tMessage_in_P14P4DriverExchangeData *)pParam;

			lazyEvent->pCallback = param->pCallback;
			lazyEvent->pCbParam = param->pCallbackParameter;
			lazyEvent->pBuffer = param->pCardToReaderBuffer;
			lazyEvent->nBufferLength = param->nCardToReaderBufferMaxLength;

			lazyEvent->pParam1 = param->hDriverConnection;
			lazyEvent->pParam2 = 0;
			break;
		}

		case P_Idenfier_P14P3DriverExchangeData: {
			tMessage_in_P14P3DriverExchangeData *param;
			param = (tMessage_in_P14P3DriverExchangeData *)pParam;

			lazyEvent->pCallback = param->pCallback;
			lazyEvent->pCbParam = param->pCallbackParameter;
			lazyEvent->pBuffer = param->pCardToReaderBuffer;
			lazyEvent->nBufferLength = param->nCardToReaderBufferMaxLength;

			lazyEvent->pParam1 = param->hDriverConnection;
			lazyEvent->pParam2 = 0;
			break;
		}		
		
		case P_Idenfier_PEmulOpenConnectionDriver2Ex: {
			tMessage_in_PEmulOpenConnectionDriver2Ex *param;
			param = (tMessage_in_PEmulOpenConnectionDriver2Ex *)pParam;

			lazyEvent->pCallback = param->pEventCallback;
			lazyEvent->pCbParam = param->pEventCallbackParameter;

			lazyEvent->pParam1 = param->hHandle;
			lazyEvent->pParam2 = 0;
			break;
		}
		
		case P_Idenfier_PEmulOpenConnectionDriver3Ex: {
			tMessage_in_PEmulOpenConnectionDriver3Ex *param;
			param = (tMessage_in_PEmulOpenConnectionDriver3Ex *)pParam;

			lazyEvent->pCallback = param->pCommandCallback;
			lazyEvent->pCbParam = param->pCommandCallbackParameter;

			lazyEvent->pParam1 = param->hHandle;
			lazyEvent->pParam2 = 0;
			break;
		}		
		
		default: 
			KNFC_DEBUG("Unknown Lazy Command Code: %d", nCode);
			break;
	}

	if(error == W_SUCCESS) {
		error = Khal_RegisterLazyEvent(lazyEvent);
	}

	if(error != W_SUCCESS) {
		KNFC_DEBUG("Register Lazy Event Error: %d", error);		
	}
		
	return error;
}

