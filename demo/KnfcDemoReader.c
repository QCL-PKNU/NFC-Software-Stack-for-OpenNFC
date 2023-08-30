/*
 * KnfcDemoReader.c
 *
 *  Created on: Jul 22, 2013
 *      Author: youngsun
 */

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>

#include "open_nfc.h"

// YOUNGSUN - CHKME
#include "KnfcHalImpl.h"
#include "KnfcHalUart.h"

#include "KnfcDemo.h"
#include "KnfcDemoReader.h"

#include "wme_context.h"

///////////////////////////////////////////////////////
// Static Variables
///////////////////////////////////////////////////////

static KnfcReaderConnection_t g_ReaderConnection;

///////////////////////////////////////////////////////
// Static Function Declaration
///////////////////////////////////////////////////////

static void ReaderErrorEventHandler(void *pHandlerParameter, uint32_t nErrorCode);

static void CardDetectionHandler(void* pHandlerParameter, W_HANDLE hConnection, W_ERROR nError);

static void TagDetectionHandler(void* pHandlerParameter, W_HANDLE hConnection, W_ERROR nError);

///////////////////////////////////////////////////////
// Static Function Definition
///////////////////////////////////////////////////////

/**
 * This function will be invoked when one of the following three error events occurs from the NFCC.
 *
 * - Unknown Card Detection 
 * - Collision Detection 
 * - Multiple Card Detection 
 */

static void
ReaderErrorEventHandler(void *pHandlerParameter, uint32_t nErrorCode) {

	KNFC_DEBUG("A reader error event has been detected");
	
	switch(nErrorCode) {
		case W_READER_ERROR_UNKNOWN:
			g_ReaderConnection.nState = KNFC_READER_UNKNOWN_ERROR;
			break;

		case W_READER_ERROR_COLLISION:
			g_ReaderConnection.nState = KNFC_READER_COLLISION_ERROR;
			break;

		case W_READER_MULTIPLE_DETECTION:
			g_ReaderConnection.nState = KNFC_READER_MULTIPLE_ERROR;
			break;

		default:
			break;			
	}

}

/**
 * This function will process the result of the card detection.
 */

static void
CardDetectionHandler(void* pHandlerParameter, W_HANDLE hConnection, W_ERROR nError) {

	KNFC_DEBUG("A card has been detected");

	g_ReaderConnection.hConnection = hConnection;
	g_ReaderConnection.nState = KNFC_READER_CARD_DETECTED;
}

/**
 * This function will handle the result of the tag detection.
 */
 
static void
TagDetectionHandler(void* pHandlerParameter, W_HANDLE hConnection, W_ERROR nError) {

	KNFC_DEBUG("A tag has been detected");

	g_ReaderConnection.hConnection = hConnection;
	g_ReaderConnection.nState = KNFC_READER_TAG_DETECTED;
}

///////////////////////////////////////////////////////
// Reader
///////////////////////////////////////////////////////

W_ERROR
KnfcReaderRegisterErrorEvent(void) {

	if(WReaderErrorEventRegister(ReaderErrorEventHandler, null, W_READER_ERROR_UNKNOWN, W_FALSE, null) != W_SUCCESS) {
		KNFC_DEBUG("Error: Cannot register the unknown card handler function.\n");
		/* Just Warn: this may happen when starting several instances of the program */
		/* goto return_function; */
	}

	if(WReaderErrorEventRegister(ReaderErrorEventHandler, null, W_READER_ERROR_COLLISION, W_FALSE, null) != W_SUCCESS) {
		KNFC_DEBUG("Error: Cannot register the card collision handler function.\n");
		/* Just Warn: this may happen when starting several instances of the program */
		/* goto return_function; */
	}

	if(WReaderErrorEventRegister(ReaderErrorEventHandler, null, W_READER_MULTIPLE_DETECTION, W_FALSE, null) != W_SUCCESS) {
		KNFC_DEBUG("Error: Cannot register the card collision handler function.\n");
		/* Just Warn: this may happen when starting several instances of the program */
		/* goto return_function; */
	}

	return W_SUCCESS;
}

W_ERROR
KnfcReaderListenToCardDetection(void) {

	W_ERROR error;

	uint8_t aTagConnectionProperty [] =	{
		W_PROP_NFC_TAG_TYPE_1,
		W_PROP_NFC_TAG_TYPE_2,
		W_PROP_NFC_TAG_TYPE_3,
		W_PROP_NFC_TAG_TYPE_4_A,
		W_PROP_NFC_TAG_TYPE_4_B,
	};

#if 0
	// Card detection
	error = WReaderListenToCardDetection(
				CardDetectionHandler, null, 
				W_PRIORITY_MINIMUM, 
				null, 0, 
				&g_ReaderConnection.hCardEventRegistry);

	if(error != W_SUCCESS) {
		KNFC_DEBUG("WReaderListenToCardDetection failed\n");
	}
#endif

	g_ReaderConnection.nState = KNFC_READER_STANDBY;

	// Tag detection
	error = WReaderListenToCardDetection(
				TagDetectionHandler, null, 
				W_PRIORITY_MAXIMUM, 
				aTagConnectionProperty, sizeof(aTagConnectionProperty),
				&g_ReaderConnection.hTagEventRegistry);

	if(error != W_SUCCESS) {
		KNFC_DEBUG("WReaderListenToCardDetection failed\n");
	}

	return W_SUCCESS;
}

/**
 * Indicates whether any tag or card has been detected or not.
 */
 
bool_t
KnfcReaderIsConnected(void) {

	if(g_ReaderConnection.nState == KNFC_READER_TAG_DETECTED || 
	   g_ReaderConnection.nState == KNFC_READER_CARD_DETECTED) {
	   return W_TRUE;
	}

	return W_FALSE;
}

/**
 * Returns the number of connection properties and the properties.
 */

uint8_t *
KnfcReaderGetConnectionProperties(uint32_t *pnPropertyNumber) {

	uint32_t propertyNumber = 0;
	uint8_t *connectionProperties = null;

	if(pnPropertyNumber == null) {
		goto KGCP_ERROR_RETURN;
	}

	if(g_ReaderConnection.nState != KNFC_READER_TAG_DETECTED &&
	   g_ReaderConnection.nState != KNFC_READER_CARD_DETECTED) {
		goto KGCP_ERROR_RETURN;
	}

	if(WBasicGetConnectionPropertyNumber(g_ReaderConnection.hConnection, &propertyNumber) != W_SUCCESS) {
		goto KGCP_ERROR_RETURN;
	}

	connectionProperties = (uint8_t *)malloc(propertyNumber * sizeof(uint8_t));

	if(connectionProperties == null) {
		goto KGCP_ERROR_RETURN;
	}

	if(WBasicGetConnectionProperties(g_ReaderConnection.hConnection, connectionProperties, propertyNumber) == W_SUCCESS) {
		KNFC_DEBUG("%d connection properties were found.", propertyNumber);
		*pnPropertyNumber = propertyNumber;
		return connectionProperties;
	}

KGCP_ERROR_RETURN:

	*pnPropertyNumber = 0;
	return null;
}


W_ERROR
KnfcReaderCreateNdefMessage(W_HANDLE *phMessage, uint8_t nTnf, uint16_t *pTypeString, uint8_t *pBuffer, uint32_t nBufferLength) {

	W_ERROR error;
	W_HANDLE record;

	// Build an NDEF message using the given bytes
	if((error = WNDEFCreateNewMessage(phMessage)) != W_SUCCESS) {
		return error;
	}

	// Create a NDEF Record
	error = WNDEFCreateRecord(nTnf, pTypeString, pBuffer, nBufferLength, &record);

	if(error != W_SUCCESS) {
		return error;
	}

	// Append the NDEF Record to the message
	if((error = WNDEFAppendRecord(*phMessage, record)) != W_SUCCESS) {
		WBasicCloseHandle(record);
	}

	return error;
}

/**
 * Return a NDEF Message
 */ 

W_ERROR
KnfcReaderReadNdefMessage(W_HANDLE *phMessage, uint8_t nTnf, const uint16_t *pTypeString) { 

	W_ERROR error;

	if(g_ReaderConnection.hConnection == 0) {
		return W_ERROR_INVALID_CONNECTION;
	}

	error = WNDEFReadMessageSync(g_ReaderConnection.hConnection,
								nTnf, pTypeString, phMessage);

	if(error != W_SUCCESS) {
		WBasicCloseHandle(g_ReaderConnection.hConnection);
		g_ReaderConnection.hConnection = 0;
	}

	return error;	
}

/**
 * Write the NDEF Message
 */ 

W_ERROR
KnfcReaderWriteNdefMessage(W_HANDLE hMessage) { 

	W_ERROR error;

	if(g_ReaderConnection.hConnection == 0) {
		return W_ERROR_INVALID_CONNECTION;
	}

	error = WNDEFWriteMessageSync(g_ReaderConnection.hConnection,
								hMessage, W_NDEF_ACTION_BIT_ERASE);

	if(error != W_SUCCESS) {
		WBasicCloseHandle(g_ReaderConnection.hConnection);
		g_ReaderConnection.hConnection = 0;
	}

	return error;	
}

//### The following accessor functions could be removed. ###

/**
 * Returns the name of the given connection property
 */

const char *
KnfcReaderGetConnectionPropertyName(uint8_t nPropertyIdentifier) {
	return WBasicGetConnectionPropertyName(nPropertyIdentifier);
}

/**
 * Returns the next NDEF message
 */ 

W_HANDLE
KnfcReaderGetNextNdefMessage(W_HANDLE hMessage) {
	return WNDEFGetNextMessage(hMessage);
}

extern tContext* g_pContext;

void
KnfcReaderClose() {

	g_ReaderConnection.nState = KNFC_READER_STANDBY;

	//WBasicCloseHandle(g_ReaderConnection.hCardEventRegistry);
	//WBasicCloseHandle(g_ReaderConnection.hTagEventRegistry);

	PReaderDriverSetWorkPerformedAndClose(g_pContext, NULL);
}

void
KnfcReaderLaunchTest() {

	char str[8];
	char operator;

	if(KnfcReaderRegisterErrorEvent() != W_SUCCESS) {
		return;
	}

	sleep(1);

	if(KnfcReaderListenToCardDetection() != W_SUCCESS) {
		return;
	}

	KNFC_DEBUG("Present a tag or card");

	while(1) {
		if(KnfcReaderIsConnected() == W_TRUE) {
			break;
		}

		usleep(100);
	}

	if(g_ReaderConnection.nState == KNFC_READER_TAG_DETECTED) {

		uint32_t i;

		uint8_t *propertyArray;
		uint32_t propertyNumber;

		W_HANDLE nextMessage;

		// Property Info
		propertyArray = KnfcReaderGetConnectionProperties(&propertyNumber);

		for(i = 0; i < propertyNumber; i++) {
			KNFC_DEBUG("Property[%d]: %s", i, KnfcReaderGetConnectionPropertyName(propertyArray[i]));
		}

		// Message Content
		KnfcReaderReadNdefMessage(&nextMessage, W_NDEF_TNF_ANY_TYPE, null);

		while(1) {
			uint8_t *message;
			uint32_t messageLength;
			uint32_t actualMessageLength;

			messageLength = WNDEFGetMessageLength(nextMessage);
			message = (uint8_t *)malloc(messageLength);

			if(message == null) {
				KNFC_DEBUG("Error: malloc");
				break;
			}

			WNDEFGetMessageContent(nextMessage, message, messageLength, &actualMessageLength);

			if(actualMessageLength == 0) {
				free(message);
				break;
			}

			KNFC_DEBUG("Ndef Message: %x", nextMessage);
			DumpHexaBytes(message, messageLength, W_TRUE, W_TRUE);
			free(message);

			nextMessage = KnfcReaderGetNextNdefMessage(nextMessage);

			if(nextMessage == null) {
				break;
			}
		}
	}

	KnfcReaderClose();
}

void
KnfcReaderLaunchTest2() {

	char str[8];
	char operator;

	sleep(1);

	if(KnfcReaderListenToCardDetection() != W_SUCCESS) {
		return;
	}

	KNFC_DEBUG("Present a tag or card");

	while(1) {
		if(KnfcReaderIsConnected() == W_TRUE) {
			break;
		}

		usleep(100);
	}
		
	if(g_ReaderConnection.nState == KNFC_READER_TAG_DETECTED) {

		uint32_t i;

		uint8_t buffer[4] = {0x01, 0x02, 0x03, 0x04};
		uint32_t bufferLength = 4;
		char16_t typeString[13] = {'t', 'e', 'x', 't', '/', 'x', '-', 'v', 'C', 'a', 'r', 'd', 0};
			
		W_HANDLE message;

		if(KnfcReaderCreateNdefMessage(&message, W_NDEF_TNF_MEDIA, typeString, buffer, bufferLength) == W_SUCCESS) {
			KnfcReaderWriteNdefMessage(message);	
		}
	}

	KnfcReaderClose();
}

void
KnfcReaderLaunchTest3() {

	char str[8];
	char operator;

	sleep(1);

	if(KnfcReaderListenToCardDetection() != W_SUCCESS) {
		return;
	}

	KNFC_DEBUG("Present a tag or card");

	while(1) {
		if(KnfcReaderIsConnected() == W_TRUE) {
			break;
		}

		usleep(100);
	}

	if(g_ReaderConnection.nState == KNFC_READER_TAG_DETECTED) {

		uint32_t i;

		uint8_t *propertyArray;
		uint32_t propertyNumber;

		W_HANDLE nextMessage;

		// Property Info
		propertyArray = KnfcReaderGetConnectionProperties(&propertyNumber);

		for(i = 0; i < propertyNumber; i++) {
			KNFC_DEBUG("Property[%d]: %s", i, KnfcReaderGetConnectionPropertyName(propertyArray[i]));
		}

		// Message Content
		KnfcReaderReadNdefMessage(&nextMessage, W_NDEF_TNF_ANY_TYPE, null);

		while(1) {
			uint8_t *message;
			uint32_t messageLength;
			uint32_t actualMessageLength;

			messageLength = WNDEFGetMessageLength(nextMessage);
			message = (uint8_t *)malloc(messageLength);

			if(message == null) {
				KNFC_DEBUG("Error: malloc");
				break;
			}

			WNDEFGetMessageContent(nextMessage, message, messageLength, &actualMessageLength);

			if(actualMessageLength == 0) {
				free(message);
				break;
			}

			KNFC_DEBUG("Ndef Message: %x", nextMessage);
			DumpHexaBytes(message, messageLength, W_TRUE, W_TRUE);
			free(message);

			nextMessage = KnfcReaderGetNextNdefMessage(nextMessage);

			if(nextMessage == null) {
				break;
			}
		}
	}

	KnfcReaderClose();
}

