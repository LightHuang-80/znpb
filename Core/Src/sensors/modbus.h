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

#endif /* SRC_MOTION_MODBUS_H_ */
