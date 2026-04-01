/*
 * modbus.h
 *
 *  Created on: 2025年8月13日
 *      Author: Huang
 */

#ifndef SRC_MOTION_MODBUS_H_
#define SRC_MOTION_MODBUS_H_

#include <stdint.h>

/* 角度传感器数据 (ID 1-5) */
extern volatile float   deviceAngle[5];
extern volatile float   deviceSpeed[5];

void Modbus_init(void);
void Modbus_proc();
uint16_t modbus_crc16(const uint8_t *data, uint16_t length);

/* 编码器复位函数 */
int Modbus_resetEncoder(uint8_t device_id, uint16_t reset_type);
int Modbus_restartEncoder(uint8_t device_id);

uint16_t modbus_crc16(const uint8_t *data, uint16_t length);

#endif /* SRC_MOTION_MODBUS_H_ */
