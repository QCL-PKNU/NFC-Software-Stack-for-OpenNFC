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

  Implementation of the Linux client porting.

*******************************************************************************/
#define P_MODULE  P_MODULE_DEC( CLIENT )

#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "porting_os.h"
#include "open_nfc.h"
#include "porting_client.h"

// YOUNGSUN
#include "KnfcHalImpl.h"
#include "KnfcHalUart.h"

typedef struct __tClientInstance
{
   /* socket used for communication with the server side */
   int nConnectionSocket;

   /* socket used to force call to WBasicPumpEvent() */
   int nWBasicPumpEventSocket;

   /* boolean used to request termination of the event receive loop */
   bool_t bRequestStop;

   /* mutex used to avoid reentrancy of CUserCallFunction */
   pthread_mutex_t hMutex;

	/* socket pair used to wake up thread */
	int aSockets[2];

} tClientInstance;

/* function call request descriptor */
typedef struct __tDFCCLientServerMessageHeader
{
   /* input parameter size */
   uint32_t nSizeIn;

   /* output parameter size */
   uint32_t nSizeOut;

   /* function identifier */
   uint8_t nCode;

} tDFCCLientServerMessageHeader;

/* client buffer synchronization request descriptor */
typedef struct __tDFCCLientServerSyncOutputData
{
   /* address of the client buffer to be synchronized */
   void* pClientBuffer;

   /* length of the client buffer */
   uint32_t nClientBufferLength;

} tDFCCLientServerSyncOutputData;


struct timeval g_timeStart;

char * g_pSocketName;

bool_t PUserInitialize(const char * pConfig)
{
	/* 
	 * Save the time of launch, to have a time display starting at zero.
	 * NOTE: for P2P experiments, it can be interesting to initialize it to 0, 
	 * so that the 2 P2P servers can be easily synchronized.
	 */
	gettimeofday(&g_timeStart, NULL);

	g_pSocketName = strdup(pConfig);

#ifdef USE_KNFC_HAL
	// Added to initialize the KHAL
	Khal_Initialize();
#endif
	
	return W_TRUE;
}


/* See porting guide */
void* CUserOpen(void)
{
   struct sockaddr_in clientService;
   tClientInstance * pInstance;

#if 0
   char data[32], control[32];
   struct msghdr   msg;
   struct cmsghdr  *cmsg;
   struct iovec    iov;
   struct ucred *  credptr;
#endif

   pInstance = CMemoryAlloc(sizeof(tClientInstance));

   if (pInstance == null)
   {
      PDebugError("CUserOpen : Cannot create the user context");
      return null;
   }

   pInstance->nConnectionSocket = -1;
   pInstance->nWBasicPumpEventSocket = -1;
   pInstance->bRequestStop = W_FALSE;

#if 0
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, pInstance->aSockets))
	{
		PDebugError("CUserOpen : socketpair() failed");
		goto return_error;
	}
#endif

//YOUNGSUN - CHKME
#ifdef USE_KNFC_UART
   pInstance->nConnectionSocket = Khal_UartOpen("/dev/ttyS2");
#else
   pInstance->nConnectionSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (pInstance->nConnectionSocket == -1)
   {
      PDebugError("CUserOpen: Cannot create the socket object");
      goto return_error;
   }

   memset(&clientService, 0, sizeof(clientService));

   clientService.sin_family = AF_INET;
   clientService.sin_addr.s_addr = inet_addr(g_pSocketName);
   clientService.sin_port = htons(8888);


   if (connect( pInstance->nConnectionSocket, (struct sockaddr *) &clientService, sizeof(clientService)))
   {
      PDebugError("CUserOpen: Cannot connect to the server - %s", g_pSocketName);
      goto return_error;
   }
   
   /* use SCM_CREDENTIALS to send client credential to the server */
#if 0
   memset(&msg, 0, sizeof(msg));

   iov.iov_base = "OPENNFC";
   iov.iov_len  = 8;
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;
   msg.msg_control = control;
   msg.msg_controllen = sizeof(control);

   cmsg = CMSG_FIRSTHDR(&msg);
   cmsg->cmsg_level = SOL_SOCKET;
   cmsg->cmsg_type = SCM_CREDENTIALS;
   cmsg->cmsg_len = CMSG_LEN(sizeof(struct ucred));
   credptr = (struct ucred *) CMSG_DATA(cmsg);    
   credptr->uid = geteuid();
   credptr->gid = getegid();
   credptr->pid = getpid();

   msg.msg_controllen = cmsg->cmsg_len;

   if (sendmsg(pInstance->nConnectionSocket, &msg, 0) < 0)
   {

      PDebugError("CUserOpen: sendmsg() failed");
      goto return_error;
   }

   /* get the event socket created by the server */

   memset(&msg, 0, sizeof(msg));
   iov.iov_base   = data;
   iov.iov_len    = sizeof(data)-1;
   msg.msg_iov    = &iov;
   msg.msg_iovlen = 1;
   msg.msg_control = control;
   msg.msg_controllen = sizeof(control);

   if (recvmsg(pInstance->nConnectionSocket, &msg, 0) < 0)
   {
      PDebugError("CUserOpen: Cannot receive event fd");
      goto return_error;
   }

   if (strcmp(data, "OPENNFC"))
   {
      PDebugError("CUserOpen : unexpected message format %s", data);
      goto return_error;
   }

   /* Loop over all control messages */
   cmsg = CMSG_FIRSTHDR(&msg);

   while (cmsg != NULL)
   {
      if ((cmsg->cmsg_level == SOL_SOCKET)  && (cmsg->cmsg_type  == SCM_RIGHTS))
      {
         int * fdptr = (int *) CMSG_DATA(cmsg);

         memcpy(&pInstance->nWBasicPumpEventSocket, fdptr, sizeof(int));

         PDebugTrace("CUserOpen: nWBasicPumpEventSocket %d", pInstance->nWBasicPumpEventSocket);
         break;
      }
   }
#endif
#endif

   if (pthread_mutex_init(&pInstance->hMutex, NULL))
   {
      PDebugError("CUserOpen: unable to initialize the mutex");
      goto return_error;
   }

   return (void*)pInstance;

return_error:

	close(pInstance->aSockets[0]);
	close(pInstance->aSockets[1]);

   if (pInstance->nWBasicPumpEventSocket != -1)
   {
      shutdown(pInstance->nWBasicPumpEventSocket, SHUT_RDWR);
      close(pInstance->nWBasicPumpEventSocket);
      pInstance->nWBasicPumpEventSocket = -1;
   }

   if(pInstance->nConnectionSocket != -1)
   {
      shutdown(pInstance->nConnectionSocket, SHUT_RDWR);
      close(pInstance->nConnectionSocket);
      pInstance->nConnectionSocket = -1;
   }

   CMemoryFree(pInstance);
   return null;
}

/* See porting guide */
void CUserClose(
            void* pUserInstance)
{
   tClientInstance * pInstance = pUserInstance;

   if(pInstance != null)
   {
	close(pInstance->aSockets[0]);
	close(pInstance->aSockets[1]);

      pthread_mutex_destroy(&pInstance->hMutex);

      /* close the event socket */
      if (pInstance->nWBasicPumpEventSocket != -1)
      {
         shutdown(pInstance->nConnectionSocket, SHUT_RDWR);
         close(pInstance->nConnectionSocket);
      }

      if(pInstance->nConnectionSocket != -1)
      {
         /* terminate the connection */
         shutdown(pInstance->nConnectionSocket, SHUT_RDWR);
         close(pInstance->nConnectionSocket);
      }
   }

   CMemoryFree(pInstance);

   free(g_pSocketName);
   g_pSocketName = NULL;

}

/* See porting guide */
W_ERROR PUserRead(
            void* pUserInstance,
            void* pBuffer,
            uint32_t nBufferLength)
{
   int socket = ((tClientInstance *)pUserInstance)->nConnectionSocket;

#ifdef USE_KNFC_UART
   return Khal_UartRead(socket, pBuffer, nBufferLength);
#else
   int32_t i;
   int32_t nOffset = 0;
   int32_t nLength;

   while(nBufferLength != 0)
   {
      nLength = recv(socket, (char *)pBuffer + nOffset, nBufferLength, MSG_NOSIGNAL);

      if(nLength < 0)
      {
         PDebugError("PUserRead : recv() failed  %d", errno);
         return W_ERROR_CLIENT_SERVER_COMMUNICATION;
      }
      else if(nLength == 0)
      {
         /* Socket gracefully shutdown */
         PDebugError("PUserRead : recv() : connection has been shut down\n");
         return W_ERROR_CLIENT_SERVER_COMMUNICATION;
      }

#if 0
	for(i = 0; i < nLength; i++) {
		KNFC_DEBUG("Read Byte [%d] = %X", i+nOffset, ((uint8_t*)pBuffer)[i+nOffset]);
	}
#endif

      nOffset += nLength;
      nBufferLength -= nLength;
   }

	return W_SUCCESS;
#endif   
}

/* See porting guide */
W_ERROR PUserWrite(
            void* pUserInstance,
            const void* pBuffer,
            uint32_t nBufferLength)
{
   int socket = ((tClientInstance *)pUserInstance)->nConnectionSocket;

#ifdef USE_KNFC_UART
   return Khal_UartWrite(socket, pBuffer, nBufferLength);
#else
   int32_t i;
   int32_t nLength;
   int32_t nOffset = 0;

   while(nBufferLength != 0)
   {
      nLength = send(socket, (char *) pBuffer + nOffset, nBufferLength, MSG_NOSIGNAL);

      if(nLength < 0)
      {
         PDebugError("PUserWrite : send() failed %d", errno);
         return W_ERROR_CLIENT_SERVER_COMMUNICATION;
      }

#if 0
	for(i = 0; i < nLength; i++) {
		KNFC_DEBUG("Write Byte [%d] = %X", i+nOffset, ((uint8_t*)pBuffer)[i+nOffset]);
	}
#endif

      nOffset += nLength;
      nBufferLength -= nLength;
   }

   return W_SUCCESS;
#endif   
}

/* See porting guide */
W_ERROR CUserCallFunction(
            void * pUserInstance,
            uint8_t nCode,
            void* pParamInOut,
            uint32_t nSizeIn,
            const void* pBuffer1,
            uint32_t nBuffer1Length,
            const void* pBuffer2,
            uint32_t nBuffer2Length,
            uint32_t nSizeOut)
{
   tClientInstance * pInstance = (tClientInstance *) pUserInstance;
   tDFCCLientServerMessageHeader header;
   W_ERROR nError;

   CMemoryFill(&header, 0xCA, sizeof(header));

   /* avoid reentrancy to ensure commands are not interleaved */

   if (pthread_mutex_lock(&pInstance->hMutex))
   {
      PDebugError("CUserCallFunction : pthread_mutex_lock failed");
      return W_ERROR_CLIENT_SERVER_COMMUNICATION;
   }

   header.nCode = nCode;
   header.nSizeIn = nSizeIn;
   header.nSizeOut = nSizeOut;

   nError = PUserWrite(pInstance, &header, sizeof(tDFCCLientServerMessageHeader));

   if(nError != W_SUCCESS)
   {
      PDebugError("CUserCallFunction: Error %d returned by PUserWrite()", nError);
      return nError;
   }

   if(nSizeIn != 0)
   {
      nError = PUserWrite(pInstance, pParamInOut, nSizeIn);
      if(nError != W_SUCCESS)
      {
         PDebugError("CUserCallFunction: Error %d returned by PUserWrite()", nError);
         return nError;
      }
   }

   if(nBuffer1Length != 0)
   {
      nError = PUserWrite(pInstance, pBuffer1, nBuffer1Length);
      if(nError != W_SUCCESS)
      {
         PDebugError("CUserCallFunction: Error %d returned by PUserWrite()", nError);
         return nError;
      }
   }

   if(nBuffer2Length != 0)
   {
      nError = PUserWrite(pInstance, pBuffer2, nBuffer2Length);
      if(nError != W_SUCCESS)
      {
         PDebugError("CUserCallFunction: Error %d returned by PUserWrite()", nError);
         return nError;
      }
   }
    /* Retreive every buffer synchronization until the function result is received */
    for(;;)
    {
       uint8_t temp;
       nError = PUserRead(pInstance, &temp, 1);
	   
       if(nError != W_SUCCESS)
       {
          PDebugError("CUserCallFunction: Error %d returned by PUserRead()", nError);
          break;
       }

#ifdef USE_KNFC_HAL
		if(temp == P_Idenfier_KhalLazyEvent || temp == P_Idenfier_KhalNormalEvent) {

			nError = PUserRead(pInstance, &temp, 1);

			if(nError != W_SUCCESS)
			{
				PDebugError("CUserCallFunction: Error %d returned by PUserRead()", nError);
				break;
			}
		}
#endif

       if(temp == nCode)
       {
         /* the code received is identical to the function identifier :
            we're receiving the answer */

         if(nSizeOut != 0)
         {
            /* process the answer of the function */
            nError = PUserRead(pInstance, pParamInOut, nSizeOut);
            if(nError != W_SUCCESS)
            {
               PDebugError("CUserCallFunction: Error %d returned by PUserRead()", nError);
            }
         }

         /* the answer has been received, function call is now complete */
         break;
       }
       else if (temp == (uint8_t) P_Identifier_PSyncOutputData)
       {
          /* process output data synchronization request */

          tDFCCLientServerSyncOutputData sSyncOutpuData;

          nError = PUserRead(pInstance, &sSyncOutpuData, sizeof(tDFCCLientServerSyncOutputData));
          if(nError != W_SUCCESS)
          {
             PDebugError("CUserCallFunction: Error %d returned by PUserRead()", nError);
             return nError;
          }
          if((sSyncOutpuData.pClientBuffer == null) || (sSyncOutpuData.nClientBufferLength == 0))
          {
             PDebugError("CUserCallFunction: Error wrong buffer message format from server");
             nError = W_ERROR_CLIENT_SERVER_PROTOCOL;
             break;
          }
          nError = PUserRead(pInstance, sSyncOutpuData.pClientBuffer, sSyncOutpuData.nClientBufferLength);
          if (nError != W_SUCCESS)
          {
             PDebugError("CUserCallFunction: Error %d returned by PUserRead()", nError);
             return nError;
          }
       }
       else
       {
          PDebugError("CUserCallFunction: Error wrong message format from server");
          nError = W_ERROR_CLIENT_SERVER_PROTOCOL;
          break;
       }
    }

   if (pthread_mutex_unlock(&pInstance->hMutex))
   {
      PDebugError("CUserCallFunction : pthread_mutex_unlock failed");
      return W_ERROR_CLIENT_SERVER_COMMUNICATION;
   }

   return nError;
}

/* See porting guide */
void CUserExecuteEventLoop(
   void * pUserInstance)
{
	tClientInstance * pInstance = (tClientInstance *) pUserInstance;
	int ret;
	char buffer[1];

	for (;;)
	{
		fd_set readfds;
		int nfds;

		FD_ZERO(&readfds);

		FD_SET(pInstance->nWBasicPumpEventSocket, &readfds);
		FD_SET(pInstance->aSockets[1], &readfds);

		if (pInstance->nWBasicPumpEventSocket > pInstance->aSockets[1])
		{
			nfds = pInstance->nWBasicPumpEventSocket + 1;
		}
		else
		{
			nfds = pInstance->aSockets[1] + 1;
		}

		ret = select(nfds, &readfds, NULL, NULL, NULL);

		if (ret < 0)
		{
			PDebugError("CUserExecuteEventLoop : select failed %d", errno);
			return;
		}

		if (pInstance->bRequestStop != W_FALSE)
		{
			PDebugTrace("shutdown of the event loop has been requested");
			return;
		}

		if (FD_ISSET(pInstance->nWBasicPumpEventSocket, & readfds))
		{
			ret = recv(pInstance->nWBasicPumpEventSocket,  buffer, 1, 0);

			if (ret != 1)
			{
				PDebugError("CUserExecuteEventLoop : recv failed %d", errno);
				return;
			}

			PDebugTrace("calling WBasicPumpEvent");
			WBasicPumpEvent(W_FALSE);
		}
	}
}

/* See porting guide */
void CUserStopEventLoop(
   void * pUserInstance)
{
   tClientInstance * pInstance = (tClientInstance *) pUserInstance;
	ssize_t nResult;
	
   pInstance->bRequestStop = W_TRUE;

	nResult = write(pInstance->aSockets[0], "X", 1);
	if (nResult <= 0)
	{
		PDebugError("%s: write error %zd", __FUNCTION__, nResult);
	}
}

/* see porting guide*/
bool_t CUserWaitForServerEvent(void * pUserInstance, bool_t * bWaitInServer)
{
   tClientInstance * pInstance = (tClientInstance *) pUserInstance;
   char buffer[1];
   
   /* the wait is done on the client side, we must not wait in the server side */
   * bWaitInServer = W_FALSE;
   
   for (;;)
   {
		fd_set readfds;
		int nfds;
      int ret;

		FD_ZERO(&readfds);

		FD_SET(pInstance->nWBasicPumpEventSocket, &readfds);
		FD_SET(pInstance->aSockets[1], &readfds);

		if (pInstance->nWBasicPumpEventSocket > pInstance->aSockets[1])
		{
			nfds = pInstance->nWBasicPumpEventSocket + 1;
		}
		else
		{
			nfds = pInstance->aSockets[1] + 1;
		}

		ret = select(nfds, &readfds, NULL, NULL, NULL);

      if (ret < 0)
		{
			PDebugError("CUserWaitForServerEvent : select failed %d", errno);
         return W_TRUE;
		}

		if (pInstance->bRequestStop != W_FALSE)
      {
         PDebugTrace("shutdown of the event loop has been requested");
         return W_TRUE;
      }

		if (FD_ISSET(pInstance->nWBasicPumpEventSocket, & readfds))
		{
			ret = recv(pInstance->nWBasicPumpEventSocket,  buffer, 1, 0);

			if (ret != 1)
			{
				PDebugError("CUserWaitForServerEvent : recv failed %d", errno);	
            return W_TRUE;
			}      
        
         return W_FALSE;
      
		}
	}      

   return W_TRUE;
}

/* EOF */
