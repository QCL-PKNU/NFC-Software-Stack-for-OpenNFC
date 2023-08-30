/*
 * KnfcDemo.c
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
#include "KnfcDemoEmul.h"

///////////////////////////////////////////////////////
// Static Function Declaration
///////////////////////////////////////////////////////

///////////////////////////////////////////////////////
// Utility
///////////////////////////////////////////////////////

/**
 * Display a byte array in hexadecimal form.
 */
void
DumpHexaBytes(uint8_t *pBytes, uint32_t nLength, bool_t bLineWrap, bool_t bAsciiDisplay) {

	uint32_t i;
	uint8_t * pAsciiDisplay = null, * pStartAscii = null;

	printf("\n DumpHexaBytes: %x, length: %d\n", pBytes, nLength);

	if(bAsciiDisplay) {
		pStartAscii = pAsciiDisplay = (W_TRUE == bLineWrap) ? malloc(16+1) : malloc(nLength+1);
	}

	for(i=0 ; i < nLength ; i++) {
		if (bAsciiDisplay) {
			*pAsciiDisplay++ = isalnum(*pBytes) ? *pBytes : '.';
		}

		printf ("%02X ", *pBytes);
		pBytes++;

		if( bLineWrap) {
			if ((i % 16) == 15) {
				if (bAsciiDisplay) {
					*pAsciiDisplay = '\0';
					printf(" %s", pStartAscii);
					pAsciiDisplay = pStartAscii;
				}
				printf("\n");
			}
		}
	}

	if(bAsciiDisplay) {
		if((bLineWrap) || (nLength < 16)) {
			int nBlanks;

			for(nBlanks=16-(i%16) ; nBlanks-- ; ) {
				printf ("   ");
			}
		}

		*pAsciiDisplay = '\0';
		printf(" %s\n", pStartAscii);
		free(pStartAscii);
	}

	printf("\n DumpHexaBytes completed... \n");
}

///////////////////////////////////////////////////////
// Demo
///////////////////////////////////////////////////////

static bool_t g_LazyEventFlag = W_FALSE;

/* The callback thread entry point */
static void *
KnfcLazyEventCallbackThread(void *pParam) {

	g_LazyEventFlag = W_TRUE;

	while(g_LazyEventFlag == W_TRUE) {
		WBasicPumpEvent(W_FALSE);
		usleep(100);
	}
	return 0;
}

bool_t
KnfcDemoInitialize(void) {

	W_ERROR error;
	uint32_t mode;
	pthread_t threadHandle;
#ifdef USE_ANDROID_NDK
	const char *address = "10.0.2.2";	/* For Android */
#else
	const char *address = "127.0.0.1";	/* For Not Android */
#endif
	const char16_t version[] = {'4', '.', '2', 0};

	// Start a thread to pump the lazy event to be processed.
	if(pthread_create(&threadHandle, NULL, KnfcLazyEventCallbackThread, NULL) == -1) {
		return W_FALSE;
	}

	if(PUserInitialize(address) == W_FALSE) {

		KNFC_DEBUG("PUserInitialize failed\n");
		goto KDI_ERROR_RETURN;
	}

	error = WBasicInit(version);

	if(error != W_SUCCESS) {

		KNFC_DEBUG("Error: Cannot initialize the library (%d).\n", error);
		goto KDI_ERROR_RETURN;
	}

	/* Check the NFC Controller mode */
	if((mode = WNFCControllerGetMode()) != W_NFCC_MODE_ACTIVE)
	{
		char16_t versionString[32];
		uint32_t length;
		bool_t fatalError = W_TRUE;

		switch(mode)
		{
			case W_NFCC_MODE_BOOT_PENDING:
				KNFC_DEBUG("Error: Boot still pending.\n");
				break;
			case W_NFCC_MODE_MAINTENANCE:
				fatalError = W_FALSE;
				KNFC_DEBUG("Warning: The NFC Controller is in maintenance mode.\n");
				break;
			case W_NFCC_MODE_NO_FIRMWARE:
				fatalError = W_FALSE;
				KNFC_DEBUG("Warning: No firmware detected.\n");
				break;
			case W_NFCC_MODE_FIRMWARE_NOT_SUPPORTED:
				fatalError = W_FALSE;
				WNFCControllerGetProperty(W_NFCC_PROP_FIRMWARE_VERSION, versionString, 32, &length);
			break;
			case W_NFCC_MODE_NOT_RESPONDING:
				KNFC_DEBUG("Error: The NFC Controller is not responding.\n");
				break;
			case W_NFCC_MODE_LOADER_NOT_SUPPORTED:
				WNFCControllerGetProperty(W_NFCC_PROP_LOADER_VERSION, versionString, 32, &length);
				break;
			case W_NFCC_MODE_SWITCH_TO_STANDBY:
			case W_NFCC_MODE_STANDBY:
			case W_NFCC_MODE_SWITCH_TO_ACTIVE:
				fatalError = W_FALSE;
				KNFC_DEBUG("Warning: The NFC Controller is in StandBy mode.\n");
				break;
			default:
				fatalError = W_FALSE;
				KNFC_DEBUG("Error: Unknown mode.\n");
				break;
		}

		if(W_TRUE == fatalError) {
			goto KDI_ERROR_RETURN;
		}
	}

	return W_TRUE;

KDI_ERROR_RETURN:
	g_LazyEventFlag = W_FALSE;
	return W_FALSE;
}

void
KnfcDemoFinalize(void) {

	g_LazyEventFlag = W_FALSE;
	WBasicTerminate();
}

///////////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////////

int
main(int argc, char** argv) {

	bool_t isReader = W_TRUE;
	int i = 0;

	KNFC_DEBUG("Demo Start...");

	if(KnfcDemoInitialize() == W_FALSE) {
		return -1;
	}

	if(isReader) {
		KnfcReaderLaunchTest();

		//KnfcReaderLaunchTest3();

#if 1
		while(1) {
			uint32_t currentAllocation;
			uint32_t peakAllocation;
			CMemoryGetStatistics(&currentAllocation, &peakAllocation);

			KNFC_DEBUG("Test2: %d", i++);
			KNFC_DEBUG("Memory: C(%d), P(%d)", currentAllocation, peakAllocation);
			KnfcReaderLaunchTest2();

			KNFC_DEBUG("Ready to Write");
			//getchar();
		}
#endif
	}
	else {
		KnfcEmulLaunchTest();
	}

	//KNFC_DEBUG("Demo Finish...");
	//KnfcDemoFinalize();

	getchar();

	return 0;
}
