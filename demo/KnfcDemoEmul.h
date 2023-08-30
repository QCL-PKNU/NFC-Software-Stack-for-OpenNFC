/*
 * KnfcDemoEmul.h
 *
 *  Created on: Aug 3, 2013
 *      Author: youngsun
 */

#ifndef _KNFC_DEMO_EMUL_H_
#define _KNFC_DEMO_EMUL_H_

#include "wme_context.h"

//////////////////////////////////////////////////
// Data Structure
//////////////////////////////////////////////////

/* Max Message Length */
#define MAX_NDEF_MESSAGE_LENGTH		128

/* Emul Connection State */

#define KNFC_EMUL_STANDBY		0x00
#define KNFC_EMUL_SELECTED		0x01

typedef struct {

	// Card Connection State
	uint32_t nState;

	// Connection Handle
	W_HANDLE hConnection;

	// Update NDEF Message
	uint8_t *pUpdateBuffer;
	uint32_t nUpdateBufferLength;
	bool_t bUpdated;

} KnfcEmulConnection_t;

//////////////////////////////////////////////////
// Global Function Declaration
//////////////////////////////////////////////////

W_ERROR KnfcEmulCreateVirtualTag(uint32_t nProtocol);

W_ERROR KnfcEmulCreateNdefMessage(W_HANDLE *phMessage, uint8_t nTnf, uint16_t *pTypeString, uint8_t *pBuffer, uint32_t nBufferLength);

W_ERROR KnfcEmulWriteNdefMessage(W_HANDLE hMessage);

W_ERROR KnfcEmulReadNdefMessage(W_HANDLE *phMessage, uint8_t nTnf, uint16_t *pTypeString);

W_ERROR KnfcEmulStartVirtualTag(bool_t bReadOnly);

W_ERROR KnfcEmulStopVirtualTag();

W_ERROR KnfcEmulUpdateNdefMessage(uint8_t *pBuffer, uint32_t nBufferLength);

bool_t KnfcVirtualTagIsUpdated();

W_ERROR KnfcEmulUpdateVirtualTag(uint8_t *pBuffer, uint32_t *pnBufferLength);

void KnfcEmulLaunchTest();

#endif /* _KNFC_DEMO_EMUL_H_ */
