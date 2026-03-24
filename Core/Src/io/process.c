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

/* 缺省的报送间隔，单位: 8ms
 * 每5 * 8ms间隔内，报送运行数据*/
#define DEF_CTL_DUR   13
uint16_t control_dur = DEF_CTL_DUR;

/*接触传感器*/
extern volatile uint8_t g_touchedEvent;
extern volatile uint8_t g_touchedState[3];

/*肩台角度*/
extern volatile float   deviceAngle[5];
extern volatile float   deviceSpeed[5];

/* 报送次序，在报送间隔内，
 * 按sth进行报送*/
uint16_t report_strength_sth   = 1;
uint16_t report_touched_sth    = 4;
uint16_t report_stationAng_sth = 7;
uint16_t report_axisAng_sth    = 10;

extern int32_t hx711_eo_data;
extern NDSConf_t  nds_conf;

typedef struct _rpt_settings_ {
	uint8_t strength;
	uint8_t touched;
	uint8_t direction;
}rpt_settings_t;

rpt_settings_t rpt_settings;

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
		}break;

	case CMD_WHL_SENSOR_RST:{

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
	loop_tick ++;
}
