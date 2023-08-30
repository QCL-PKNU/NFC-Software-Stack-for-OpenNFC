/*
 * KnfcHalEvent.h
 *
 *  Created on: Jun 24, 2013
 *      Author: youngsun
 */

#ifndef _KNFC_HAL_EVENT_H_
#define _KNFC_HAL_EVENT_H_

#include "wme_context.h"

//////////////////////////////////////////////////
// Marco
//////////////////////////////////////////////////

// ID for the lazy event
#define P_Idenfier_KhalLazyEvent		0xFA
#define P_Idenfier_KhalNormalEvent	0xFB
#define P_Idenfier_KhalParameter0	0xFC
#define P_Idenfier_KhalParameter1	0xFD

//////////////////////////////////////////////////
// Data Structure Declaration
//////////////////////////////////////////////////

typedef struct {

	// basic info 
	uint8_t nEventCode;
	tContext *pContext;

	// basic parameters
	void *pCallback;
	void *pCbParam;
	void *pBuffer;
	uint32_t nBufferLength;

	// additional parameters
	void *pParam1;
	void *pParam2;
	void *pParam3;
	void *pParam4;
	void *pParam5;
	
} KnfcLazyEvent_t;

typedef struct {
	
	/* socket used for communication with the server side */
	int nConnectionSocket;

	/* socket used to force call to WBasicPumpEvent() */
	int nWBasicPumpEventSocket;

	/* boolean used to request termination of the event receive loop */
	bool_t bRequestStop;

	/* mutex used to avoid reentrancy of CUserCallFunction */
	pthread_mutex_t hMutex;

	/* socket pair used to wake up thread */
	int aSockets[2];

} KnfcConnInstance_t;

//////////////////////////////////////////////////
// Global Function Declaration
//////////////////////////////////////////////////

W_ERROR Khal_IntializeLazyEventRegistry(void);

W_ERROR Khal_FinalizeLazyEventRegistry(void);

W_ERROR Khal_RegisterLazyEvent(KnfcLazyEvent_t *pEvent);

W_ERROR Khal_UnregisterLazyEvent(KnfcLazyEvent_t *pEvent);

#endif /* _KNFC_HAL_EVENT_H_ */
