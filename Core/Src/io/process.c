/*
 * process.c
 *
 *  Created on: 2024年8月23日
 *      Author: Huang
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "usart.h"
#include "can.h"
#include "log.h"
#include "gpio.h"
#include <usartport.h>
#include "canport.h"

#include "cmd.h"
#include "ndsconf.h"
#include "gpioexti.h"
#include "modbus.h"
#include "tof.h"

/* 缺省的报送间隔，单位: 8ms
 * 每5 * 8ms间隔内，报送运行数据*/
#define DEF_CTL_DUR   13
uint16_t control_dur = DEF_CTL_DUR;

/*接触传感器*/
extern volatile uint8_t g_touchedEvent;
extern volatile uint8_t g_touchedState[3];

/* 报送次序，在报送间隔内，
 * 按sth进行报送*/
uint16_t report_strength_sth   = 1;
uint16_t report_touched_sth    = 4;
uint16_t report_stationAng_sth = 7;
uint16_t report_axisAng_sth    = 10;
uint16_t report_distance_sth   = 12;  // 距离传感器报送时间点

extern int32_t hx711_eo_data;
extern NDSConf_t  nds_conf;

typedef struct _rpt_settings_ {
	uint8_t strength;
	uint8_t touched;
	uint8_t direction;
}rpt_settings_t;

rpt_settings_t rpt_settings;

/* 编码器控制CAN ID */
#define CAN_ID_ENCODER_CTRL  0x46  // 十进制70

/* 编码器控制命令 */
#define ENCODER_CMD_RESET    0x01
#define ENCODER_CMD_RESTART  0x02

void Proc_init()
{
	CANmodule_init(&hcan, 250);
	CANsetNormalMode(&hcan);

	g_touchedEvent = 0;
}

void Proc_report_strength()
{
	CANtx_t *tx = &canport.txArray[0];
	tx->ident = 0x01;	// can bus master
	tx->DLC   = 0x06;

	tx->data[0] = nds_conf.hwset.ID;
	tx->data[1] = CMD_SENS_STRENGTH;
	memcpy(&(tx->data[2]), (void*)&hx711_eo_data, sizeof(int32_t));

	CANsend(tx);
}

void Proc_report_touched(uint8_t event)
{
	CANtx_t *tx = &canport.txArray[0];
	tx->ident = 0x01;	// can bus master
	tx->DLC   = 0x06;

	tx->data[0] = nds_conf.hwset.ID;
	tx->data[1] = CMD_SENS_TOUCHED;

	/*触发的线路*/
	tx->data[2] = event;

	tx->data[3] = g_touchedState[0];
	tx->data[4] = g_touchedState[1];
	tx->data[5] = g_touchedState[2];

	CANsend(tx);
}

void Proc_report_stationAng()
{
	CANtx_t *tx = &canport.txArray[0];
	tx->ident = 0x01;	// can bus master
	tx->DLC   = 0x06;

	tx->data[0] = nds_conf.hwset.ID;
	tx->data[1] = CMD_SENS_STATIONANG;

	/*肩台角度*/
	int16_t angle;

	if (nds_conf.hwset.ID == 150 || nds_conf.hwset.ID == 160){
		tx->DLC   = 0x04;
		float scaled = (deviceAngle[2] * 100.0f);
		angle = (int16_t)((scaled >= 0.0f) ? (scaled + 0.5f) : (scaled - 0.5f));
		memcpy(&(tx->data[2]), (void*)&angle, sizeof(int16_t));

		/*150和160没有轮子角度*/
	}else{
		tx->DLC   = 0x06;
		float scaled = (deviceAngle[1] * 100.0f);
		angle = (int16_t)((scaled >= 0.0f) ? (scaled + 0.5f) : (scaled - 0.5f));
		memcpy(&(tx->data[2]), (void*)&angle, sizeof(int16_t));

		/*轮子角度*/
		scaled = (int16_t)(deviceAngle[0] * 100.0f);
		angle = (int16_t)((scaled >= 0.0f) ? (scaled + 0.5f) : (scaled - 0.5f));
		memcpy(&(tx->data[4]), (void*)&angle, sizeof(int16_t));
	}

	CANsend(tx);
}

void Proc_report_axisAng()
{
	CANtx_t *tx = &canport.txArray[0];
	tx->ident = 0x01;	// can bus master
	tx->DLC   = 0x06;

	tx->data[0] = nds_conf.hwset.ID;
	tx->data[1] = CMD_SENS_AXISANG;

	/*机械臂角度*/
	int16_t angle;

	if (nds_conf.hwset.ID != 150 && nds_conf.hwset.ID != 160){
		/*主臂角度*/
		float scaled = (deviceAngle[3] * 100.0f);
		angle = (int16_t)((scaled >= 0.0f) ? (scaled + 0.5f) : (scaled - 0.5f));
		memcpy(&(tx->data[2]), (void*)&angle, sizeof(int16_t));

		/*前臂角度*/
		scaled = (int16_t)(deviceAngle[4] * 100.0f);
		angle = (int16_t)((scaled >= 0.0f) ? (scaled + 0.5f) : (scaled - 0.5f));
		memcpy(&(tx->data[4]), (void*)&angle, sizeof(int16_t));

		CANsend(tx);
	}
}

void Proc_report_distance()
{
	CANtx_t *tx = &canport.txArray[0];
	tx->ident = 0x01;	// can bus master
	tx->DLC   = 0x05;

	tx->data[0] = nds_conf.hwset.ID;
	tx->data[1] = CMD_SENS_DISTANCE;

	/*距离值(mm)*/
	memcpy(&(tx->data[2]), (void*)&deviceDistance, sizeof(uint16_t));

	/*有效标志*/
	tx->data[4] = distanceValid;

	CANsend(tx);
}

/**
 * @brief 处理编码器控制消息 (CAN ID: 0x70)
 * @param rx CAN接收消息
 * 
 * 数据格式:
 *   data[0]: device_id (1-5, 0=全部)
 *   data[1]: cmd (0x01=复位, 0x02=重启)
 *   data[2]: reset_type_h (复位类型高字节)
 *   data[3]: reset_type_l (复位类型低字节)
 * 
 * 复位类型:
 *   0x8000: 位置清零
 *   0x4000: 复位圈数
 *   0x2000: 复位全部错误
 *   0x1000: 进入线性校准状态
 */
void Proc_handle_encoder_ctrl(CANrxMsg_t* rx)
{
	if (rx->dlc < 2) return;
	
	uint8_t device_id = rx->data[0];
	uint8_t cmd = rx->data[1];
	
	if (cmd == ENCODER_CMD_RESET && rx->dlc >= 4) {
		// 复位命令
		uint16_t reset_type = (rx->data[2] << 8) | rx->data[3];
		
		if (device_id == 0) {
			// 复位全部编码器 (ID 1-5)
			for (uint8_t i = 1; i <= 5; i++) {
				Modbus_resetEncoder(i, reset_type);
			}
		} else if (device_id >= 1 && device_id <= 5) {
			// 复位指定编码器
			Modbus_resetEncoder(device_id, reset_type);
		}
	}
	else if (cmd == ENCODER_CMD_RESTART) {
		// 重启命令
		if (device_id == 0) {
			// 重启全部编码器
			for (uint8_t i = 1; i <= 5; i++) {
				Modbus_restartEncoder(i);
			}
		} else if (device_id >= 1 && device_id <= 5) {
			// 重启指定编码器
			Modbus_restartEncoder(device_id);
		}
	}
}

void Proc_handle(CANrxMsg_t* rx)
{
	if (rx->ident != nds_conf.hwset.ID){
		return;
	}

	switch(rx->data[0]){
	case CMD_HW_ID:{
			if (rx->data[1] == 1){
				uint8_t hwid = rx->data[2];
				CONF_setID(hwid);
			}
		}break;

	case CMD_CTL_DUR: {
		    control_dur = rx->data[1];
		    if (control_dur == 0xFF){
		    	control_dur = DEF_CTL_DUR;
		    }

		}break;
	case CMD_WHL_DATA_RPT:{
		report_strength_sth   = rx->data[1];
		report_touched_sth    = rx->data[2];
		report_stationAng_sth = rx->data[3];
		report_axisAng_sth    = rx->data[4];
		report_distance_sth   = rx->data[5];
		}break;

	case CMD_WHL_SENSOR_RST:{

		}break;

	case CMD_ENCODER_RESET:{
		// 编码器复位命令处理
		// rx->data[1]: device_id (1-5, 0xFF=全部)
		// rx->data[2]: reset_type
		//   0x01 = 位置清零 (0x8000)
		//   0x02 = 复位圈数 (0x4000)
		//   0x03 = 复位全部错误 (0x2000)
		//   0x04 = 设备重启 (功能码0x41)
		//   0x05 = 位置清零+复位圈数 (0xC000)
		uint8_t dev_id = rx->data[1];
		uint8_t rst_type = rx->data[2];
		uint16_t modbus_reset_value = 0;

		// 转换复位类型为Modbus寄存器值
		switch(rst_type) {
			case 0x01: modbus_reset_value = 0x8000; break;  // 位置清零
			case 0x02: modbus_reset_value = 0x4000; break;  // 复位圈数
			case 0x03: modbus_reset_value = 0x2000; break;  // 复位全部错误
			case 0x04: modbus_reset_value = 0;      break;  // 设备重启(特殊处理)
			case 0x05: modbus_reset_value = 0xC000; break;  // 位置清零+复位圈数
			default: break;  // 无效类型，不执行操作
		}

		if (dev_id == 0xFF) {
			// 复位全部编码器 (ID 1-5)
			for (uint8_t i = 1; i <= 5; i++) {
				if (rst_type == 0x04) {
					Modbus_restartEncoder(i);
				} else if (modbus_reset_value != 0) {
					Modbus_resetEncoder(i, modbus_reset_value);
				}
			}
		} else if (dev_id >= 1 && dev_id <= 5) {
			// 复位指定编码器
			if (rst_type == 0x04) {
				Modbus_restartEncoder(dev_id);
			} else if (modbus_reset_value != 0) {
				Modbus_resetEncoder(dev_id, modbus_reset_value);
			}
		}
		}break;

	default:
		break;
	}
}

void Proc_loop()
{
	static uint32_t loop_tick = 0;
	static uint32_t lastEventTick;

	if (loop_tick >= control_dur){
		loop_tick = 0;
	}

	CANmodule_process();
	CANclearPendingSync();

	/*handle can msg*/
	CANrxMsg_t rx;
	uint8_t ret = CANreadMsg(&rx);
	if (ret){
		// 检查是否是编码器控制消息 (CAN ID: 0x70)
		if (rx.ident == CAN_ID_ENCODER_CTRL) {
			Proc_handle_encoder_ctrl(&rx);
		} else {
			Proc_handle(&rx);
		}
	}

	if (report_strength_sth > 0){
		if (loop_tick == report_strength_sth){
			Proc_report_strength();
		}
	}

	uint32_t tick = HAL_GetTick();
	if (report_touched_sth > 0){
		/*接触传感器等级高*/
		if (g_touchedEvent > 0){
			Proc_report_touched(g_touchedEvent);
			g_touchedEvent = 0;
			lastEventTick = tick;
		}
	}

	/*接触传感器，每隔400ms，报送一次*/
	if (tick - lastEventTick > 400){
		GPIO_updateTouchedState();
		Proc_report_touched(0);
		lastEventTick = tick;
	}

	if (report_stationAng_sth > 0){
		if (loop_tick == report_stationAng_sth){
			Proc_report_stationAng();
		}
	}

	if (report_axisAng_sth > 0){
		if (loop_tick == report_axisAng_sth){
			Proc_report_axisAng();
		}
	}

	if (report_distance_sth > 0){
		if (loop_tick == report_distance_sth){
			Proc_report_distance();
		}
	}
	loop_tick ++;
}
