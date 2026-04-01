/*
 * cmd.h
 *
 *  Created on: 2024年8月25日
 *      Author: Huang
 */

#ifndef SRC_IO_CMD_H_
#define SRC_IO_CMD_H_

#include <stdio.h>

#define CMD_HW_ID           0x01
#define CMD_WHL_VELMAX      0x02
#define CMD_WHL_Polarity    0x03

#define CMD_WHL_DRV   		0x04
#define CMD_WHL_BRK   		0x05
#define CMD_WHL_TURN_DIST 	0x06
#define CMD_WHL_MOVE_TO_ANGLE 0x07

#define CMD_WHL_DATA_RPT     0x10	// report speed,current,position
#define CMD_WHL_RPT_SPEED    0x11
#define CMD_WHL_RPT_CURRENT  0x12
#define CMD_WHL_RPT_POSITION 0x13
#define CMD_WHL_SENSOR_RST   0x14
#define CMD_CTL_DUR          0x15

#define CMD_WHL_VEL_PID      0x20    // velocity pid set

#define CMD_WHL_PWM_Prescaler 0x30
#define CMD_WHL_PWM_Period    0x31

/*Motor PWM duty = duty start + (duty step * speed)*/
#define CMD_WHL_PWM_DutySet   0x32
#define CMD_WHL_PWM_DutyStep  0x33

#define CMD_WHL_POS_PID       0x34  // position pid set
#define CMD_WHL_ACCEDECE      0x35

#define CMD_SENS_STRENGTH     0x40
#define CMD_SENS_TOUCHED      0x41
#define CMD_SENS_STATIONANG   0x42

#define CMD_SENS_SUPPORTANG   0x43

#define CMD_SENS_AXISANG      0x44
#define CMD_SENS_DISTANCE     0x45  // 距离传感器数据

#define CMD_ENCODER_RESET     0x46  // 编码器复位命令 (十进制70)

#endif /* SRC_IO_CMD_H_ */
