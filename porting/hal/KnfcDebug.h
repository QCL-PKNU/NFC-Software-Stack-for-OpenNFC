/*
 * KnfcDebug.h
 *
 *  Created on: Jun 24, 2013
 *      Author: youngsun
 */

#ifndef _KNFC_DEBUG_H_
#define _KNFC_DEBUG_H_

#include "wme_context.h"
#include "KnfcHalImpl.h"

//////////////////////////////////////////////////
// NFC Debug Function Declaration
//////////////////////////////////////////////////

void KnfcDebugPrintln(const char * sTag, const char * sFmt,...);

//////////////////////////////////////////////////
// Debug Interface Macros for NFC Debugging Functions
//////////////////////////////////////////////////

#define DEBUG
#define LOW_LEVEL_TRACES
#define LLC_TRACE
#define HCI_TRACE
#define LLC_DATA_BYTES

#ifndef KNFC_DEBUG
#ifdef USE_ANDROID_NDK
#include <android/log.h>

#define KNFC_DEBUG(FMT, ...)	__android_log_print(ANDROID_LOG_VERBOSE, "KNFC", FMT, ##__VA_ARGS__)
#else
#define KNFC_DEBUG(FMT, ...)	KnfcDebugPrintln("KNFC", FMT, ##__VA_ARGS__)
#endif
#else
#define KNFC_DEBUG(FMT, ...)	//
#endif

#ifndef ALOG
#define ALOG		KNFC_DEBUG
#else
#define ALOG		//
#endif

#ifndef ALOGD
#define ALOGD		ALOG
#else
#define ALOGD		//
#endif

#ifndef ALOGI
#define ALOGI		ALOG
#else
#define ALOGI		//
#endif

#ifndef ALOGW
#define ALOGW		ALOG
#else
#define ALOGW		//
#endif

#ifndef ALOGE
#define ALOGE		ALOG
#else
#define ALOGE		//
#endif

#endif /* KNFCDEBUG_H_ */
