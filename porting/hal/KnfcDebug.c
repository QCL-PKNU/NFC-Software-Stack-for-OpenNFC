/*
 * KnfcDebug.c
 *
 *  Created on: Jun 24, 2013
 *      Author: youngsun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "KnfcDebug.h"

void
KnfcDebugPrintln(const char *sTag, const char *sFmt, ...) {

	char buf[256];

	va_list args;
	va_start(args, sFmt);
	vsprintf(buf, sFmt, args);
	va_end(args);

	printf("%s]: %s\n", sTag, buf);
}

