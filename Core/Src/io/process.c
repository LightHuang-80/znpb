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

/* 编码器控制命令 */
#define ENCODER_CMD_RESET    0x01
#define ENCODER_CMD_RESTART  0x02

/* LED循环控制参数 */
typedef struct {
	uint8_t  on_sec;       // 亮的秒数 (0=停止/常灭)
	uint8_t  off_sec;      // 灭的秒数
	uint8_t  is_on;        // 当前状态: 1=亮, 0=灭
	uint32_t last_toggle;  // 上次切换时刻(ms)
} led_ctrl_t;

static led_ctrl_t led_ctrl[2] = {0};

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
		// 新的数据格式:
		// data[0] = 0x46 (msg_type，已在switch中匹配)
		// data[1] = device_id (编码器设备ID，0=全部, 1-5=指定)
		// data[2] = cmd (0x01=复位, 0x02=重启)
		// data[3] = reset_type 高字节 (仅复位需要)
		// data[4] = reset_type 低字节 (仅复位需要)
		if (rx->dlc < 3) break;
		
		uint8_t device_id = rx->data[1];
		uint8_t cmd = rx->data[2];
		
		if (cmd == ENCODER_CMD_RESET && rx->dlc >= 5) {
			// 复位命令
			uint16_t reset_type = (rx->data[3] << 8) | rx->data[4];
			
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
		}break;

	case CMD_LED_CTRL:{
		// data[1] = IN1亮秒数, data[2] = IN1灭秒数
		// data[3] = IN2亮秒数, data[4] = IN2灭秒数
		if (rx->dlc >= 5) {
			uint32_t now = HAL_GetTick();
			led_ctrl[0].on_sec  = rx->data[1];
			led_ctrl[0].off_sec = rx->data[2];
			led_ctrl[0].last_toggle = now;
			led_ctrl[1].on_sec  = rx->data[3];
			led_ctrl[1].off_sec = rx->data[4];
			led_ctrl[1].last_toggle = now;

			// 立即设置初始状态 (PHASE_A=PB3, PHASE_B=PA15)
			if (led_ctrl[0].on_sec > 0) {
				led_ctrl[0].is_on = 1;
				HAL_GPIO_WritePin(PA_GPIO_Port, PA_Pin, GPIO_PIN_SET);
			} else {
				led_ctrl[0].is_on = 0;
				HAL_GPIO_WritePin(PA_GPIO_Port, PA_Pin, GPIO_PIN_RESET);
			}
			if (led_ctrl[1].on_sec > 0) {
				led_ctrl[1].is_on = 1;
				HAL_GPIO_WritePin(PB_GPIO_Port, PB_Pin, GPIO_PIN_SET);
			} else {
				led_ctrl[1].is_on = 0;
				HAL_GPIO_WritePin(PB_GPIO_Port, PB_Pin, GPIO_PIN_RESET);
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
		Proc_handle(&rx);
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
	/* LED循环控制 */
	{
		uint32_t now = HAL_GetTick();
		// PHASE_A (PB3)
		if (led_ctrl[0].on_sec > 0) {
			uint32_t dur = led_ctrl[0].is_on
				? (uint32_t)led_ctrl[0].on_sec * 1000
				: (uint32_t)led_ctrl[0].off_sec * 1000;
			if (now - led_ctrl[0].last_toggle >= dur) {
				led_ctrl[0].is_on = !led_ctrl[0].is_on;
				led_ctrl[0].last_toggle = now;
				HAL_GPIO_WritePin(PA_GPIO_Port, PA_Pin,
						led_ctrl[0].is_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
			}
		}
		// PHASE_B (PA15)
		if (led_ctrl[1].on_sec > 0) {
			uint32_t dur = led_ctrl[1].is_on
				? (uint32_t)led_ctrl[1].on_sec * 1000
				: (uint32_t)led_ctrl[1].off_sec * 1000;
			if (now - led_ctrl[1].last_toggle >= dur) {
				led_ctrl[1].is_on = !led_ctrl[1].is_on;
				led_ctrl[1].last_toggle = now;
				HAL_GPIO_WritePin(PB_GPIO_Port, PB_Pin,
						led_ctrl[1].is_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
			}
		}
	}

	loop_tick ++;
}
