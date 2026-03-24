/*
 * proto.c
 *
 *  Created on: 2023年12月16日
 *      Author: Administrator
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "main.h"
#include "usart.h"

#include <usartport.h>
#include "CO_fifo.h"

extern uint8_t usart_Received;
extern uint8_t usart_Failed;
extern uint8_t usart_Sent;

#define BUFF_SZ	128

uint8_t   usart_rcvData;
uint8_t   rcvFifo[BUFF_SZ];
CO_fifo_t rcvPort;

UART_HandleTypeDef *portHdl;

void Port_reset()
{
	CO_fifo_reset(&rcvPort);
}

HAL_StatusTypeDef Port_init(UART_HandleTypeDef *port, int16_t timeout)
{
	Port_reset();

	portHdl = port;
	while(portHdl->RxState != HAL_UART_STATE_READY && timeout-- > 0){
		HAL_Delay(1);
	}

	CO_fifo_init(&rcvPort, (char*)rcvFifo, BUFF_SZ);

	/*Start port*/
	return HAL_UART_Receive_IT(portHdl, &usart_rcvData, 1);
}

uint16_t Port_read(uint8_t* data, uint16_t cnt)
{
	CO_fifo_altBegin(&rcvPort, 0);
	size_t len = CO_fifo_altRead(&rcvPort, (char*)data, cnt);

	if (len == cnt){
		CO_fifo_altFinish(&rcvPort, NULL);
		return len;
	}

	return 0;
}

uint16_t Port_recvData_len()
{
	size_t len = CO_fifo_getOccupied(&rcvPort);
	return (uint16_t)len;
}

void Port_send(uint8_t* data, uint16_t cnt, int16_t timeout)
{
	HAL_UART_Transmit(portHdl, data, cnt, timeout);
	usart_Sent += cnt;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == portHdl->Instance){
		/*Save the receive data*/
		CO_fifo_write(&rcvPort, (const char*)&usart_rcvData, 1, NULL);

		while(HAL_UART_Receive_IT(portHdl, &usart_rcvData, 1) == HAL_BUSY);
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == portHdl->Instance){

		if ((huart->ErrorCode & HAL_UART_ERROR_ORE) ||
			(huart->ErrorCode & HAL_UART_ERROR_FE))	{
			__HAL_UART_CLEAR_OREFLAG(huart);
		}

		HAL_UART_Abort(portHdl);
		HAL_UART_Receive_IT(portHdl, &usart_rcvData, 1);
	}
}

