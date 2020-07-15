/*
 * serial.c
 *
 *  Created on: Jul 4, 2020
 *      Author: olegkaliuzhnyi
 */

#include "stdlib.h"

#include "serial.h"
#include "stm32f1xx_ll_usart.h"
#include "stm32f1xx_ll_dma.h"



void Serial_Init(Serial* serial, USART_TypeDef* USARTx, uint32_t rxBufferLength)
{
	serial->USARTx = USARTx;
	serial->RxBufferIndex = 0;
	serial->RxReadIndex = 0;
	serial->RxBuffer = malloc(sizeof(uint8_t) * rxBufferLength);
	serial->RxBufferLength = rxBufferLength;
}

void Serial_Deinit(Serial* serial)
{
	free(serial->RxBuffer);
	serial->RxBufferLength = 0;
}

void Serial_SendByte(Serial* serial, uint8_t byte)
{
	USART_TypeDef* usart = serial->USARTx;
	while (!LL_USART_IsActiveFlag_TXE(usart)) {} // waiting for TXE to be set to SR register
		LL_USART_TransmitData8(usart, byte);
}

void Serial_SendBytes(Serial* serial, uint8_t* data, uint32_t size)
{
	for	(uint32_t i = 0; i < size; i++)
	{
		Serial_SendByte(serial, *(uint8_t*)(data + i));
	}
}

void Serial_SendString(Serial* serial, char* str)
{
	uint32_t i = 0;
	char current = *str;
	while (current != '\0')
	{
		Serial_SendByte(serial, (uint8_t)current);

		i++;
		current = *(char*)(str + i);
	}
}

void Serial_SendLine(Serial* serial, char* str)
{
	Serial_SendString(serial, str);
	Serial_SendString(serial, "\n");
}

void Serial_HandleRxInterrupt(Serial* serial)
{
	USART_TypeDef* usart = serial->USARTx;

	if(LL_USART_IsActiveFlag_RXNE(usart) && LL_USART_IsEnabledIT_RXNE(usart))
	{
		uint8_t data = LL_USART_ReceiveData8(usart);
		serial->RxBuffer[serial->RxBufferIndex] = data;

		if (serial->RxBufferIndex == serial->RxBufferLength - 1)
			serial->RxBufferIndex = 0;
		else
			serial->RxBufferIndex++;

	}
	else if(LL_USART_IsActiveFlag_ORE(USART1))
	{
	  (void) usart->DR; // read from DR to reset ORE flag (overflow error)
	}
	else if(LL_USART_IsActiveFlag_FE(USART1))
	{
	  (void) usart->DR; // read from DR to reset FE flag (frame receiving error, no stop bit)
	}
	else if(LL_USART_IsActiveFlag_NE(USART1))
	{
	  (void) usart->DR; // read from DR to reset NE flag (signal noise error)
	}
	else if(LL_USART_IsActiveFlag_PE(USART1))
	{
	  (void) usart->DR; // read from DR to reset PE flag (parity error)
	}
}

void Serial_HandleRxDMA(Serial* serial, DMA_TypeDef* DMAx, uint32_t LL_DMA_CHANNEL_x)
{
	uint32_t bufferLength = serial->RxBufferLength;
//	uint32_t rxBufferIndex = serial->RxBufferIndex;

	uint32_t currentPosition = bufferLength - LL_DMA_GetDataLength(DMAx, LL_DMA_CHANNEL_x);
//	if (currentPosition != rxBufferIndex)
//	{
//		if (currentPosition > rxBufferIndex)
//		{
//			/* We are in "linear" mode */
//			/* Process data directly by subtracting "pointers" */
//			usart_process_data(serial->RxBuffer[rxBufferIndex], currentPosition - rxBufferIndex);
//		}
//		else
//		{
//			/* We are in "overflow" mode */
//			/* First process data to the end of buffer */
//			usart_process_data(serial->RxBuffer[rxBufferIndex], bufferLength - rxBufferIndex);
//			/* Check and continue with beginning of buffer */
//			if (currentPosition > 0)
//			{
//				usart_process_data(serial->RxBuffer[0], currentPosition);
//			}
//		}
//	}
	serial->RxBufferIndex = currentPosition;

	if (serial->RxBufferIndex == bufferLength)
	{
		serial->RxBufferIndex = 0;
	}
}

uint32_t Serial_Available(Serial* serial)
{
	if (serial->RxBufferIndex == serial->RxReadIndex) /* Check change in received data */
	{
		return 0;
	}
	else if (serial->RxBufferIndex > serial->RxReadIndex) /* Current position is over previous one */
	{
		/* We are in "linear" mode, case P1, P2, P3 */
		return serial->RxBufferIndex - serial->RxReadIndex;
	}
	else
	{
		/* We are in "overflow" mode, case P4 */
		return serial->RxBufferLength - serial->RxReadIndex + serial->RxBufferIndex;
	}
}

uint8_t* Serial_Read(Serial* serial, uint32_t* outLength)
{
	uint8_t* result = 0;
	*outLength = 0;

	uint32_t rxBufferIndex = serial->RxBufferIndex;
	if (rxBufferIndex != serial->RxReadIndex) /* Check change in received data */
	{
		if (rxBufferIndex > serial->RxReadIndex) /* Current position is over previous one */
		{
			/* We are in "linear" mode, case P1, P2, P3 */

			uint32_t length = rxBufferIndex - serial->RxReadIndex;
			uint32_t size = sizeof(uint8_t) * length;

			*outLength = length;

			result = malloc(size);
			memcpy(result, &serial->RxBuffer[serial->RxReadIndex], size);
		}
		else
		{
			/* We are in "overflow" mode, case P4 */

			uint32_t length = serial->RxBufferLength - serial->RxReadIndex + rxBufferIndex;
			uint32_t uint8Size = sizeof(uint8_t);
			uint32_t size = uint8Size * length;

			*outLength = length;

			uint8_t* result = (uint8_t*)malloc(size);

			/* Copy end part of the buffer */
			uint32_t copySize1 = uint8Size * (serial->RxBufferLength - serial->RxReadIndex);
			memcpy(result, &serial->RxBuffer[serial->RxReadIndex], copySize1);

			/* Copy start part of the buffer */
			uint32_t copySize2 = uint8Size * rxBufferIndex;
			memcpy(result + copySize1 + 1, serial->RxBuffer, copySize2);
		}
	}

	serial->RxReadIndex = rxBufferIndex; /* Save current position as old */

	return result;
}

char* Serial_ReadStringUntil(Serial* serial, char chr)
{
	char* result = 0;

	uint32_t rxBufferIndex = serial->RxBufferIndex;
	if (rxBufferIndex != serial->RxReadIndex) /* Check change in received data */
	{
		uint32_t index = serial->RxReadIndex;
		uint32_t length = 0;

		char current = (char)serial->RxBuffer[index];
		while(current != chr)
		{
			if (index == serial->RxBufferLength - 1)
				index = 0;
			else
				index++;

			while (index == serial->RxBufferIndex); /* wait for new data */

			length++;
			if (length >= serial->RxBufferLength)
			{
				length = 0;
				break;
			}

			current = (char)serial->RxBuffer[index];
		}

		if (length > 0)
		{
			uint32_t uint8Size = sizeof(uint8_t);
			uint32_t size = uint8Size * length + 1;

			result = (char*)malloc(size);

			uint32_t start = serial->RxReadIndex;
			for (uint32_t i = 0; i < length; i++)
			{
				uint32_t bufferIndex = start + i;
				if (bufferIndex == serial->RxBufferLength - 1)
					start = 0;

				result[i] = (char)serial->RxBuffer[bufferIndex];
			}

			result[length] = '\0';

			serial->RxReadIndex = (serial->RxReadIndex + length + 1) % serial->RxBufferLength;
		}
		else
		{
			serial->RxReadIndex++; /* buffer has only chr */
		}
	}

	return result;
}

