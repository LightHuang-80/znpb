/*
 * port.h
 *
 *  Created on: 2023年12月17日
 *      Author: Administrator
 */

#ifndef INC_PORT_H_
#define INC_PORT_H_


HAL_StatusTypeDef Port_init(UART_HandleTypeDef *port, int16_t timeout);

uint16_t Port_recvData_len();
uint16_t Port_read(uint8_t* data, uint16_t cnt);

void Port_send(uint8_t* data, uint16_t cnt, int16_t timeout);

#endif /* INC_PORT_H_ */
