/*
 * tof.h
 *
 *  TOF距离传感器模块 - 使用UART3独立通信（单字节中断接收）
 *  Created on: 2025年
 *      Author: Huang
 */

#ifndef SRC_SENSORS_TOF_H_
#define SRC_SENSORS_TOF_H_

#include <stdint.h>

/* 距离传感器数据 */
extern volatile uint16_t deviceDistance;  // 距离值，单位mm
extern volatile uint8_t  distanceValid;   // 距离数据有效标志

/* 调试计数器 */
extern volatile uint32_t tof_debugRxCount;    // 接收中断计数
extern volatile uint32_t tof_debugFrameCount; // 帧处理计数
extern volatile uint32_t tof_debugErrCount;   // 错误恢复计数

/* 初始化TOF模块 */
void TOF_init(void);

/* TOF处理函数，在主循环中调用 */
void TOF_proc(void);

/* UART3接收回调，在HAL_UART_RxCpltCallback中调用 */
void TOF_RxCallback(void);

#endif /* SRC_SENSORS_TOF_H_ */
