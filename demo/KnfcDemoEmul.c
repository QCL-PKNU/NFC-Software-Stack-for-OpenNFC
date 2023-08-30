/*
 * KnfcDemoCardEmul.c
 *
 *  Created on: Aug 3, 2013
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
#include "KnfcDemoEmul.h"

//////////////////////////////////////////////////
// Static Variable
//////////////////////////////////////////////////

static KnfcEmulConnection_t g_hEmulConnection;

//////////////////////////////////////////////////
// Static Function Declaration
//////////////////////////////////////////////////

static void VirtualTagEventHandler(void *pParameter, uint32_t nEventCode);

//////////////////////////////////////////////////
// Static Function Definition
//////////////////////////////////////////////////

/* Receive the event of the virtual tag */
static void
VirtualTagEventHandler(void *pParameter, uint32_t nEventCode) {

	switch(nEventCode)
	{
		case W_VIRTUAL_TAG_EVENT_SELECTION:
			printf("The tag is selected by the reader.\n");
			break;
		case W_VIRTUAL_TAG_EVENT_READER_LEFT:
			printf("The reader left the tag without reading the content.\n");
			break;
		case W_VIRTUAL_TAG_EVENT_READER_READ_ONLY :
			printf("The reader read the tag.\n");
			break;
		case W_VIRTUAL_TAG_EVENT_READER_WRITE:
		default:
			printf("This event should not occur for a read-only virtual tag.\n");
			break;
	}
}

//////////////////////////////////////////////////
// Global Function Definition
//////////////////////////////////////////////////

W_ERROR
KnfcEmulCreateVirtualTag(uint32_t nProtocol) {

	uint8_t tagId[] = {0x01, 0x02, 0x03, 0x04};
	W_ERROR error = WVirtualTagCreate(nProtocol, tagId, sizeof(tagId),
										MAX_NDEF_MESSAGE_LENGTH,
										&g_hEmulConnection.hConnection);

	g_hEmulConnection.bUpdated = W_FALSE;

	KNFC_DEBUG("Create Virtual Tag Error: %d", error);
	return error;
}

W_ERROR
KnfcEmulCreateNdefMessage(W_HANDLE *phMessage, uint8_t nTnf, uint16_t *pTypeString, uint8_t *pBuffer, uint32_t nBufferLength) {

	W_ERROR error;
	W_HANDLE record;

	// Build an NDEF message using the given bytes
	if((error = WNDEFCreateNewMessage(phMessage)) != W_SUCCESS) {
		return error;
	}

	// Create a NDEF Record
	error = WNDEFCreateRecord(nTnf, pTypeString,
							pBuffer, nBufferLength, &record);

	if(error != W_SUCCESS) {
		return error;
	}

	// Append the NDEF Record to the message
	if((error = WNDEFAppendRecord(*phMessage, record)) != W_SUCCESS) {
		WBasicCloseHandle(record);
	}

	return error;
}

W_ERROR
KnfcEmulWriteNdefMessage(W_HANDLE hMessage) {

	W_ERROR error;

	if(g_hEmulConnection.hConnection == 0) {
		return W_ERROR_INVALID_CONNECTION;
	}

	error = WNDEFWriteMessageSync(g_hEmulConnection.hConnection,
								hMessage, W_NDEF_ACTION_BIT_ERASE);

	if(error != W_SUCCESS) {
		WBasicCloseHandle(g_hEmulConnection.hConnection);
		g_hEmulConnection.hConnection = 0;
	}

	return error;
}

W_ERROR
KnfcEmulReadNdefMessage(W_HANDLE *phMessage, uint8_t nTnf, uint16_t *pTypeString) {

	W_ERROR error;

	if(g_hEmulConnection.hConnection == 0) {
		return W_ERROR_INVALID_CONNECTION;
	}

	error = WNDEFReadMessageSync(g_hEmulConnection.hConnection,
								nTnf, pTypeString, phMessage);

	if(error != W_SUCCESS) {
		WBasicCloseHandle(g_hEmulConnection.hConnection);
		g_hEmulConnection.hConnection = 0;
	}

	return error;
}

W_ERROR
KnfcEmulStartVirtualTag(bool_t bReadOnly) {

	W_ERROR error;

	KNFC_DEBUG("KnfcEmulStartVirtualTag called... %d", bReadOnly);
	
	if(g_hEmulConnection.hConnection == 0) {
		return W_ERROR_INVALID_CONNECTION;
	}

	error = WVirtualTagStartSync(g_hEmulConnection.hConnection,
								VirtualTagEventHandler, null, bReadOnly);

	if(error != W_SUCCESS) {
		WBasicCloseHandle(g_hEmulConnection.hConnection);
		g_hEmulConnection.hConnection = 0;
	}

	return error;
}

W_ERROR
KnfcEmulStopVirtualTag() {

	W_ERROR error;

	KNFC_DEBUG("KnfcEmulStopVirtualTag called...");

	// If the connection is not being opened, just return.
	if(g_hEmulConnection.hConnection == 0) {
		return W_SUCCESS;
	}	

	error = WVirtualTagStopSync(g_hEmulConnection.hConnection);

	if(error == W_SUCCESS) {
	
		WBasicCloseHandle(g_hEmulConnection.hConnection);
		g_hEmulConnection.hConnection = 0;
	}

	return error;
}

W_ERROR
KnfcEmulUpdateNdefMessage(uint8_t *pBuffer, uint32_t nBufferLength) {

	W_ERROR error;

	KNFC_DEBUG("KnfcEmulUpdateNdefMessage called: %d", nBufferLength);

	if(g_hEmulConnection.hConnection == 0) {
		return W_ERROR_INVALID_CONNECTION;
	}

	g_hEmulConnection.pUpdateBuffer = (uint8_t *)malloc(nBufferLength);
	memcpy(g_hEmulConnection.pUpdateBuffer, pBuffer, nBufferLength);
	g_hEmulConnection.nUpdateBufferLength = nBufferLength;
	g_hEmulConnection.bUpdated = W_TRUE;

	return W_SUCCESS;
	}

bool_t
KnfcVirtualTagIsUpdated() {

	return g_hEmulConnection.bUpdated;
}

W_ERROR
KnfcEmulUpdateVirtualTag(uint8_t *pBuffer, uint32_t *pnBufferLength) {

	KNFC_DEBUG("KnfcEmulUpdateVirtualTag called... %d", g_hEmulConnection.bUpdated);

	if(g_hEmulConnection.hConnection == 0) {
		return W_ERROR_INVALID_CONNECTION;
	}

	if(g_hEmulConnection.bUpdated == W_FALSE) {
		return W_SUCCESS;
	}

	memcpy(pBuffer, g_hEmulConnection.pUpdateBuffer, g_hEmulConnection.nUpdateBufferLength);
	*pnBufferLength = g_hEmulConnection.nUpdateBufferLength;
	g_hEmulConnection.bUpdated = W_FALSE;

	return W_SUCCESS;
	}

void
KnfcEmulLaunchTest() {

	W_ERROR error;
	W_HANDLE message;
	uint16_t typeString[] = {
		't', 'e', 'x', 't', '/', 'x', '-', 'v', 'C', 'a', 'r', 'd'
	};
	uint8_t *payloadBuffer = "www.raon-tech.com";
	uint32_t payloadLength = strlen(payloadBuffer);

	error = KnfcEmulCreateNdefMessage(&message, W_NDEF_TNF_MEDIA, typeString,
										payloadBuffer, payloadLength);

	if(error != W_SUCCESS) {
		KNFC_DEBUG("Virtual Tag Stopping Error");
		return;
	}

	sleep(5);

#if 1

	error = KnfcEmulCreateVirtualTag(W_PROP_NFC_TAG_TYPE_4_A);

	if(error != W_SUCCESS) {
		KNFC_DEBUG("Virtual Tag Creation Error");
		return;
	}

	error = KnfcEmulWriteNdefMessage(message);

	if(error != W_SUCCESS) {
		KNFC_DEBUG("Virtual Tag Writing Error");
		return;
	}

	error = KnfcEmulStartVirtualTag(W_TRUE);

	if(error != W_SUCCESS) {
		KNFC_DEBUG("Virtual Tag Starting Error");
		return;
	}

	KNFC_DEBUG("Press any button to stop the tag.");
	getchar();

	error = KnfcEmulStopVirtualTag();

	if(error != W_SUCCESS) {
		KNFC_DEBUG("Virtual Tag Stopping Error");
		return;
	}
#endif
}

