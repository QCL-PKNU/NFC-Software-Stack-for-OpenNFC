/*
 * KnfcDemoReader.h
 *
 *  Created on: Jul 22, 2013
 *      Author: youngsun
 */

#ifndef KNFCDEMOREADER_H_
#define KNFCDEMOREADER_H_

#include "wme_context.h"

//////////////////////////////////////////////////
// Data Structure
//////////////////////////////////////////////////

/* Reader Connection State */

#define KNFC_READER_STANDBY				0x00

// Indicate whether the detected one is a tag or a card.
// The tag will contain an NDEF-format message, but the card will not.
#define KNFC_READER_TAG_DETECTED		0x01
#define KNFC_READER_CARD_DETECTED		0x02

// Error Event Code
#define KNFC_READER_UNKNOWN_ERROR		0x03
#define KNFC_READER_COLLISION_ERROR		0x04
#define KNFC_READER_MULTIPLE_ERROR		0x05

typedef struct {

	// Reader Connection State
	uint32_t nState;

	// Connection Handle
	W_HANDLE hConnection;

	// Card Detection Event Registry
	W_HANDLE hCardEventRegistry;

	// Tag Detection Event Registry
	W_HANDLE hTagEventRegistry;
	
} KnfcReaderConnection_t;

//////////////////////////////////////////////////
// Global Function Declaration
//////////////////////////////////////////////////

W_ERROR KnfcReaderRegisterErrorEvent(void);

W_ERROR KnfcReaderListenToCardDetection(void);

W_ERROR KnfcReaderCreateNdefMessage(W_HANDLE *phMessage, uint8_t nTnf, uint16_t *pTypeString, uint8_t *pBuffer, uint32_t nBufferLength);

W_ERROR KnfcReaderReadNdefMessage(W_HANDLE *phMessage, uint8_t nTnf, const uint16_t *pTypeString);

W_ERROR KnfcReaderWriteNdefMessage(W_HANDLE hMessage);

void KnfcReaderClose(void);

void KnfcReaderLaunchTest(void);

void KnfcReaderLaunchTest2(void);

void KnfcReaderLaunchTest3(void);

#endif /* KNFCDEMOREADER_H_ */
