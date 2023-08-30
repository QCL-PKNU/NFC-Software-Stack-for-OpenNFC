/*
 * Copyright (c) 2007-2010 Inside Secure, All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*******************************************************************************

  Trace Functions

*******************************************************************************/

#include "porting_os.h"

#ifdef P_TRACE_ACTIVE

#ifdef USE_CCCLIENT

#include "ccclient.h"

void CDebugPrintTrace(const char* pTag,uint32_t nTraceLevel, const char* pMessage, va_list list)
{
   CCTracePrint(pTag, nTraceLevel, pMessage, list);
}


void CDebugPrintTraceBuffer(const char* pTag, uint32_t nTraceLevel, const uint8_t* pDataBuffer, uint32_t nLength)
{
   CCTracePrintBuffer(pTag, nTraceLevel, pDataBuffer, nLength);

}
#else

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>


#define COUNTER_LENGTH     5
#define LEVEL_POSITION     6
#define TAG_POSITION       12
#define TEXT_POSITION      20
#define TRACE_MAX_SIZE     500
#define TRACE_TEMPLATE     "00000 TRACE       : "

extern struct timeval g_timeStart;

static char g_aTraceTemplate[TEXT_POSITION];
static char g_aTraceBuffer[TRACE_MAX_SIZE];
static bool_t b_UseColors;
static void DebugInitCheck(void);


static char* static_PrepareTraceBuffer(
            const char* pTag,
            uint32_t nTraceLevel)
{

   uint32_t nPos = COUNTER_LENGTH;

   /* Small overhead: ensure init was done on each call */
   DebugInitCheck();

   do
   {
      char nValue = g_aTraceTemplate[nPos-1];
      if(nValue != '9')
      {
         g_aTraceTemplate[nPos-1] = nValue + 1;
         break;
      }
      g_aTraceTemplate[--nPos] = '0';
   } while (nPos > 0);

   memcpy(g_aTraceBuffer, g_aTraceTemplate, TEXT_POSITION);

   switch(nTraceLevel)
   {
   case P_TRACE_TRACE:
      break;
   case P_TRACE_LOG:
      memcpy(g_aTraceBuffer + LEVEL_POSITION, "LOG  ", 5);
      break;
   case P_TRACE_WARNING:
      memcpy(g_aTraceBuffer + LEVEL_POSITION, "WARN ", 5);
      break;
   case P_TRACE_ERROR:
      memcpy(g_aTraceBuffer + LEVEL_POSITION, "ERROR", 5);
      break;
   default:
      memcpy(g_aTraceBuffer + LEVEL_POSITION, "?????", 5);
      break;
   }

   memcpy(g_aTraceBuffer + TAG_POSITION, pTag, strlen(pTag));

   return g_aTraceBuffer;
}

static long static_getRunTime(void)
{
   struct timeval currentTime;
   long msec;

   gettimeofday(&currentTime, NULL);

   msec = currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
   msec -= g_timeStart.tv_sec * 1000 + g_timeStart.tv_usec / 1000;

   return msec;
}

/* See the Functional Specifications Document */
void CDebugPrintTrace(
            const char* pTag,
            uint32_t nTraceLevel,
            const char* pMessage,
            va_list list)
{
   char* pBuffer = static_PrepareTraceBuffer(pTag, nTraceLevel);

   int nPos = vsnprintf( pBuffer + TEXT_POSITION, TRACE_MAX_SIZE - TEXT_POSITION - 1, pMessage, list);
   if (nPos >=  (TRACE_MAX_SIZE - TEXT_POSITION - 2)) {
       nPos = TRACE_MAX_SIZE - 2;
   } else {
       nPos += TEXT_POSITION;
   }

   pBuffer[nPos++] = '\n';
   pBuffer[nPos] = 0;

   if (b_UseColors)
   {
      switch (nTraceLevel)
      {
         default:
         case P_TRACE_TRACE :
            printf("\e[0;37m");
            break;

         case P_TRACE_LOG :
            printf("\e[1;37m");
            break;

         case P_TRACE_ERROR:
            printf("\e[0;31m");
            break;
      }
   }
   printf("NFC [t=%08ld] %s", static_getRunTime(), pBuffer);

   if (b_UseColors)
      printf("\e[00m");
}


/* See the Functional Specifications Document */
void CDebugPrintTraceBuffer(
            const char* pTag,
            uint32_t nTraceLevel,
            const uint8_t* pDataBuffer,
            uint32_t nLength)
{
   uint32_t nIndex;
   char* pBuffer = static_PrepareTraceBuffer(pTag, nTraceLevel);

   if (b_UseColors)
   {
      switch (nTraceLevel)
      {
         default:
         case P_TRACE_TRACE :
            printf("\e[0;37m");
            break;

         case P_TRACE_LOG :
            printf("\e[1;37m");
            break;

         case P_TRACE_ERROR:
            printf("\e[0;31m");
            break;
      }
   }

   printf("             ");

   int nPos = snprintf ( pBuffer + TEXT_POSITION, TRACE_MAX_SIZE-TEXT_POSITION-1, "   buffer[%02d] = { 0x", nLength);
   if (nPos >=  (TRACE_MAX_SIZE - TEXT_POSITION - 2)) {
       nPos = TRACE_MAX_SIZE - 2;
   } else {
       nPos += TEXT_POSITION;
   }

   for(nIndex = 0; nIndex < nLength; nIndex++)
   {
      uint8_t nValue = pDataBuffer[nIndex];
      uint8_t nSubValue;

      if (nPos > TRACE_MAX_SIZE - 5 /* space, 2 letters, \n, \0 */)
	      break;
      
      pBuffer[nPos++] = ' ';
      nSubValue = nValue >> 4;
      if(nSubValue <= 9)
      {
         pBuffer[nPos++] = nSubValue + '0';
      }
      else
      {
         pBuffer[nPos++] = nSubValue - 10 + 'A';
      }
      nSubValue = nValue & 0x0F;
      if(nSubValue <= 9)
      {
         pBuffer[nPos++] = nSubValue + '0';
      }
      else
      {
         pBuffer[nPos++] = nSubValue - 10 + 'A';
      }
   }

   if (nPos < TRACE_MAX_SIZE - 3) {
	   pBuffer[nPos++] = ' ';
	   pBuffer[nPos++] = '}';
   }
   pBuffer[nPos++] = '\n';
   pBuffer[nPos] = 0;
   printf("NFC [t=%08ld] %s", static_getRunTime(), pBuffer);

   if (b_UseColors)
      printf("\e[00m");
}

#define DEBUG_MAGIC_NUMBER 0xDEADBEEF


static void DebugInitCheck(void)
{
   static uint32_t g_DebugInitMagicNumber;

   if (g_DebugInitMagicNumber != DEBUG_MAGIC_NUMBER)
   {
      g_DebugInitMagicNumber = DEBUG_MAGIC_NUMBER;

      /* initialize the trace buffer */
      memcpy(g_aTraceTemplate, TRACE_TEMPLATE, TEXT_POSITION);

      b_UseColors = isatty(STDOUT_FILENO);
   }
}

#endif

#endif

