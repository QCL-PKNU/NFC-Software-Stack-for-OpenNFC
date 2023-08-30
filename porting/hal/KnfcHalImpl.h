/*
 * KnfcHalImpl.h
 *
 *  Created on: Jun 24, 2013
 *      Author: youngsun
 */

#ifndef _KNFC_HAL_IMPL_H_
#define _KNFC_HAL_IMPL_H_

#include "wme_context.h"
#include "KnfcHalEvent.h"

//////////////////////////////////////////////////
// Data Structure Declaration
//////////////////////////////////////////////////

#define USE_KNFC_HAL

//#define USE_KNFC_UART

//#define USE_ANDROID_NDK

//////////////////////////////////////////////////
// Data Structure Declaration
//////////////////////////////////////////////////

/**
 * Message Format between the protocol stack and firmware
 */
typedef struct {
	// function identifier
	uint8_t nCode;

	// the number of bytes of the response
	uint32_t nLength;

} KnfcRespHeader_t;

typedef struct {
	// message header
	KnfcRespHeader_t header;

	// message body
	uint8_t *pPayload;

} KnfcRespMessage_t;

//////////////////////////////////////////////////
// KNFC Parameter Data Structure
//////////////////////////////////////////////////

typedef struct {
	// This must be either P_Idenfier_KhalParameter0 or P_Idenfier_KhalParameter1.
	uint8_t nCode;
	
	// the number of bytes of the parameter
	uint32_t nLength;
	
} KhstParamHeader_t;

typedef struct {

	// Parameter header
	KhstParamHeader_t header;

	// Parameter body
	uint8_t *pPayload;
	
} KhstParamMessage_t;

//////////////////////////////////////////////////
// HAL interface
//////////////////////////////////////////////////

// To prevent from waiting the lazy response in LazyEventHandlerThread
extern pthread_mutex_t g_hWaitResponseMutex;

// Interface functions
W_ERROR Khal_Initialize();

W_ERROR Khal_WaitAndProcessResponse(uint8_t nCode, tContext *pContext, void *pParam);

W_ERROR Khal_LazyWaitAndProcessResponse(uint8_t nCode, tContext *pContext, void *pParam);

#endif /* _KNFC_HAL_IMPL_H_ */
