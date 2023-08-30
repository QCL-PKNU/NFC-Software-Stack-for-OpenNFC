/*
 * KnfcHalUart.h
 *
 *  Created on: Jul 16, 2013
 *      Author: youngsun
 */

#ifndef _KNFC_HAL_UART_H_
#define _KNFC_HAL_UART_H_

#include "wme_context.h"

int Khal_UartOpen(const char *sComPort);

void Khal_UartClose(int fd);

W_ERROR Khal_UartRead(int fd, void *pBuffer, uint32_t nBufferLength);

W_ERROR Khal_UartWrite(int fd, const void *pBuffer, uint32_t nBufferLength);

#endif /* _KNFC_HAL_UART_H_ */
