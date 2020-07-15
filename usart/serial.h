/*
 * Serial.h
 *
 *  Created on: Jul 4, 2020
 *      Author: olegkaliuzhnyi
 */

#ifndef SRC_SERIAL_H_
#define SRC_SERIAL_H_

#include "stm32f103xb.h"


typedef struct
{
	USART_TypeDef* USARTx;
	volatile uint8_t* RxBuffer;
	volatile uint32_t RxBufferIndex;
	uint32_t RxBufferLength;
	uint32_t RxReadIndex;
} Serial;


void Serial_Init(Serial* serial, USART_TypeDef* USARTx, uint32_t rxBufferLength);

void Serial_Deinit(Serial* serial);

void Serial_SendByte(Serial* serial, uint8_t byte);

void Serial_SendBytes(Serial* serial, uint8_t* data, uint32_t size);

void Serial_SendString(Serial* serial, char* str);

void Serial_SendLine(Serial* serial, char* str);

void Serial_HandleRxInterrupt(Serial* serial);

void Serial_HandleRxDMA(Serial* serial, DMA_TypeDef* DMAx, uint32_t LL_DMA_CHANNEL_x);

uint32_t Serial_Available(Serial* serial);

uint8_t* Serial_Read(Serial* serial, uint32_t* outLength);

char* Serial_ReadStringUntil(Serial* serial, char chr);


#endif /* SRC_SERIAL_H_ */
