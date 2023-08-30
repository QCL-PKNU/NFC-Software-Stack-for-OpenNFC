/*
 * KnfcDemo.h
 *
 *  Created on: Jul 22, 2013
 *      Author: youngsun
 */

#ifndef _KNFC_DEMO_H_
#define _KNFC_DEMO_H_

#include "wme_context.h"

///////////////////////////////////////////////////////
// Utility
///////////////////////////////////////////////////////

void DumpHexaBytes(uint8_t *pBytes, uint32_t nLength, bool_t bLineWrap, bool_t bAsciiDisplay);

///////////////////////////////////////////////////////
// Reader
///////////////////////////////////////////////////////

bool_t KnfcDemoInitialize(void);

void KnfcDemoFinalize(void);

#endif /* _KNFC_DEMO_H_ */
