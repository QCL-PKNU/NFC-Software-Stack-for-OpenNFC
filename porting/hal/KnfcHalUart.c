/*
 * KnfcHalUart.c
 *
 *  Created on: Jul 16, 2013
 *      Author: youngsun
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>

#include "KnfcHalUart.h"
#include "KnfcDebug.h"

//#define BAUDRATE B115200
//#define BAUDRATE B38400
//#define BAUDRATE B19200
#define BAUDRATE B9600

int
Khal_UartOpen(const char *sComPort) {

	int fd;
	struct termios oldtio,newtio;

	KNFC_DEBUG("Khal_UartOpen: %s", sComPort);

	fd = open(sComPort, O_RDWR | O_NOCTTY );

	if (fd <0) {
		perror(sComPort);
		return(-1);
	}

	tcgetattr(fd,&oldtio); /* save current port settings */

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
	newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);
	return fd;
}

void
Khal_UartClose(int fd) {
	close(fd);
}

W_ERROR
Khal_UartRead(int fd, void *pBuffer, uint32_t nBufferLength) {

	int32_t i;
	int32_t nOffset = 0;
	int32_t nLength = 0;

	while(nBufferLength != 0)
	{
		nLength = read(fd, (char *)pBuffer + nOffset, nBufferLength);

		if(nLength <= 0)
		{
			PDebugError("Khal_UartRead : read() failed  %d", errno);
			return W_ERROR_UART_COMMUNICATION;
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
}

W_ERROR
Khal_UartWrite(int fd, const void *pBuffer, uint32_t nBufferLength) {

	int32_t i;
	int32_t nLength = 0;
	int32_t nOffset = 0;

	while(nBufferLength != 0)
	{
		nLength = write(fd, (char *)pBuffer + nOffset, nBufferLength);

		if(nLength <= 0)
		{
			PDebugError("Khal_UartWrite : write() failed %d", errno);
			return W_ERROR_UART_COMMUNICATION;
		}
#if 0
		for(i = 0; i < nLength; i++) {
			KNFC_DEBUG("Write Byte [%d] = %X", i, ((uint8_t*)pBuffer)[i]);
		}
#endif
		nOffset += nLength;
		nBufferLength -= nLength;
	}

	return W_SUCCESS;
}
