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

  Memory Functions.

*******************************************************************************/

#define P_MODULE  P_MODULE_DEC( MEM )

#include "porting_os.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Dynamic memory limited to 256 KB */
#define P_MEMORY_LIMIT (256*1024)

static uint32_t g_nCurrentMemory = 0;
static uint32_t g_nPeakMemory = 0;

void CMemoryGetStatistics(
               uint32_t* pnCurrentAllocation,
               uint32_t* pnPeakAllocation )
{
   *pnCurrentAllocation = g_nCurrentMemory;
   *pnPeakAllocation = g_nPeakMemory;
}

void CMemoryResetStatistics( void )
{
   g_nPeakMemory = g_nCurrentMemory;
}

void* CMemoryAlloc( uint32_t nSize )
{
   void* pBuffer;

// YOUNGSUN - CHKME
#if 0
   if((nSize > P_MEMORY_LIMIT)
   || ((g_nCurrentMemory + nSize) > P_MEMORY_LIMIT))
   {
      printf("Memory allocation error: limit of %d bytes is reached\n",  P_MEMORY_LIMIT);
      return null;
   }
#endif

   /* Store the size at the beginning of the block to compute the memory consumption
    * in PMemoryFree().
    */
   pBuffer = malloc( (size_t)(nSize + sizeof(uint32_t)) );
   if(pBuffer != null)
   {
      *((uint32_t*)pBuffer) = nSize;
      pBuffer = (void*)( ((uint8_t*)pBuffer) + sizeof(uint32_t));

      g_nCurrentMemory += nSize;
      if(g_nCurrentMemory > g_nPeakMemory)
      {
         g_nPeakMemory = g_nCurrentMemory;
      }
   }

   return pBuffer;
}

void CMemoryFree( void* pBuffer )
{
   if( pBuffer != null )
   {
      uint32_t nSize;

      pBuffer = (void*)( ((uint8_t*)pBuffer) - sizeof(uint32_t));
      nSize = *((uint32_t*)pBuffer);
      free ( pBuffer );

      CDebugAssert(g_nCurrentMemory >= nSize);
      g_nCurrentMemory -= nSize;
   }
}

/* EOF */
