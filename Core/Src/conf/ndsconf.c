/*
 * ndsconf.c
 *
 *  Created on: 2023年12月16日
 *      Author: Administrator
 */




#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "main.h"
#include "tim.h"
#include "ndsconf.h"
#include "eeprom.h"
#include "stm32f1xx_hal_flash_ex.h"

/*Adc index,
 * ADC5 -- Pos reference
 * ADC7 -- Power reference
 * ADC9 -- Motor current reference*/
uint16_t   adc_Buff[3];

NDSConf_t  nds_conf = {
		{01, 10, 02, 12},
		{0x1006, 0xFD, 0x01, 0xF0}, // 0xF : 115200
		393, 0,
		200,  	// Velmax;
		1600,  	// AcceMax;
		1600,  	// DeceMax;
		500,  	// Jerk;

		3599, // TimerArr;
		372,	// Curmax;
		2613,	// Volmax;

		/*Position PID, 100x value*/
		140,	// Kp0;
		20,		// Ki0;
		42,		// Kd0;

		/*Current PID, 10x value*/
		72,	 // Kp1;
		420, // Ki1;
		1,	 // Kd1;

		/*Position PID error threshold*/
		12,	// PidErrThres0;

		/*Velocity PID error threshold*/
		12,	// PidErrThres1;

		80,	// HomeSpeed;

		/*Zero speed from PWM duty*/
		2,	// PWMDutyStart;

		/*PWM duty increased interval*/
		10,	// PWMDutyInc;

		/*Min allowed distant error*/
		4,

		/*Dead time*/
		2,

		2,	//  PWMPrescaler;
		6,	//  ADCCycles1;
		6,	//  ADCCycles2;
		1,	//  Polarity;
		10,	//  SpeedSample;
		4,  //  PID sample time
		180, //  PID max vel
		2,  //  Limit Low Pos
		250,//  Limit Up Pos
		1200 // SCurve speed coefficient
};

#define    NUM_VARS_IN_ROM 40

/*The variables virtual flag table*/
uint16_t   rom_virtaddr[NUM_VARS_IN_ROM] = {
		0, // 	ManfId;
		1, //   ProductId;
		2, //   SerialId;
		3, //   VsId;
		4, //   Firmware Ver;
		5, //   Machine ID;
		6, //   Machine group GID;
	    7, //   Buadrate;

		8,	// GearRatio_Hv;
		9,	// GearRatio_Lv;   // GearRatio = Hv + Lv * 0.01f
		10,	// Velmax;
		11,	// AcceMax;
		12,	// DeceMax;
		13,	// Jerk;
		14,	// TimerArr;
		15,	// Curmax;
		16,	// Volmax;

		/*Position PID*/
		17,	// Kp0;
		18,	// Ki0;
		19,	// Kd0;

		/*Velocity PID*/
		20,	// Kp1;
		21,	// Ki1;
		22,	// Kd1;

		/*Position PID error threshold*/
		23,	// PidErrThres0;

		/*Current PID error threshold*/
		24,	// PidErrThres1;

		25,	// HomeSpeed;

		/*Zero speed from PWM duty*/
		26,	// PWMDutyStart;

		/*PWM duty increased interval*/
		27,	// PWMDutyInc;

		/*Min allowed distant error*/
		28,

		/*Dead time*/
		29,

		30,	//  PWMPrescaler;
		31,	//  ADCCycles1;
		32,	//  ADCCycles2;
		33,	//  Polarity;
		34,	//  SpeedSample;
		35, //  PidSampleTime
		36, //  PidMaxVel
		37, //  LimitLowPos
		38, //  LimitUpPos
		39  //  SCurveSpeedCoeff
};

uint16_t* VirtAddVarTab;
uint16_t  NumbOfVar;

void CONF_init()
{
	VirtAddVarTab = rom_virtaddr;
	NumbOfVar = 	NUM_VARS_IN_ROM;

	HAL_FLASH_Unlock();
	EE_Init();
	HAL_FLASH_Lock();
}

void CONF_load()
{
	EE_ReadVariable(rom_virtaddr[0], (uint16_t*)&nds_conf.puid.ManfId);
	EE_ReadVariable(rom_virtaddr[1], (uint16_t*)&nds_conf.puid.ProductId);
	EE_ReadVariable(rom_virtaddr[2], (uint16_t*)&nds_conf.puid.SerialId);
	EE_ReadVariable(rom_virtaddr[3], (uint16_t*)&nds_conf.puid.VsId);

	EE_ReadVariable(rom_virtaddr[4], (uint16_t*)&nds_conf.hwset.Ver);
	EE_ReadVariable(rom_virtaddr[5], (uint16_t*)&nds_conf.hwset.ID);
	EE_ReadVariable(rom_virtaddr[6], (uint16_t*)&nds_conf.hwset.GID);
	EE_ReadVariable(rom_virtaddr[7], (uint16_t*)&nds_conf.hwset.Buadrate);

	EE_ReadVariable(rom_virtaddr[8], (uint16_t*)&nds_conf.GearRatio_Hv);	// GearRatio_Hv;
	EE_ReadVariable(rom_virtaddr[9], (uint16_t*)&nds_conf.GearRatio_Lv);	// GearRatio_Lv;   // GearRatio = Hv + Lv * 0.01f
	EE_ReadVariable(rom_virtaddr[10], (uint16_t*)&nds_conf.Velmax);	// Velmax;
	EE_ReadVariable(rom_virtaddr[11], (uint16_t*)&nds_conf.AcceMax);	// AcceMax;
	EE_ReadVariable(rom_virtaddr[12], (uint16_t*)&nds_conf.DeceMax);	// DeceMax;
	EE_ReadVariable(rom_virtaddr[13], (uint16_t*)&nds_conf.Jerk);	// Jerk;
	EE_ReadVariable(rom_virtaddr[14], (uint16_t*)&nds_conf.TimerArr);	// TimerArr;
	EE_ReadVariable(rom_virtaddr[15], (uint16_t*)&nds_conf.Curmax);	// Curmax;
	EE_ReadVariable(rom_virtaddr[16], (uint16_t*)&nds_conf.Volmax);	// Volmax;

	/*Position PID*/
	EE_ReadVariable(rom_virtaddr[17], (uint16_t*)&nds_conf.Kp0);	// Kp0;
	EE_ReadVariable(rom_virtaddr[18], (uint16_t*)&nds_conf.Ki0);	// Ki0;
	EE_ReadVariable(rom_virtaddr[19], (uint16_t*)&nds_conf.Kd0);	// Kd0;

	/*Current PID*/
	EE_ReadVariable(rom_virtaddr[20], (uint16_t*)&nds_conf.Kp1);	// Kp1;
	EE_ReadVariable(rom_virtaddr[21], (uint16_t*)&nds_conf.Ki1);	// Ki1;
	EE_ReadVariable(rom_virtaddr[22], (uint16_t*)&nds_conf.Kd1);	// Kd1;

	/*Position PID error threshold*/
	EE_ReadVariable(rom_virtaddr[23], (uint16_t*)&nds_conf.PidErrThres0);	// PidErrThres0;

	/*Current PID error threshold*/
	EE_ReadVariable(rom_virtaddr[24], (uint16_t*)&nds_conf.PidErrThres1);	// PidErrThres1;

	EE_ReadVariable(rom_virtaddr[25], (uint16_t*)&nds_conf.HomeSpeed);	// HomeSpeed;

	/*Zero speed from PWM duty*/
	EE_ReadVariable(rom_virtaddr[26], (uint16_t*)&nds_conf.PWMDutyStart);	// PWMDutyStart;

	EE_ReadVariable(rom_virtaddr[27], (uint16_t*)&nds_conf.PWMDutyInc);	// Deadtime;

	EE_ReadVariable(rom_virtaddr[28], (uint16_t*)&nds_conf.MinDistErr);	// Minimize distant error;

	EE_ReadVariable(rom_virtaddr[29], (uint16_t*)&nds_conf.DeadTime);	// Deadtime;

	EE_ReadVariable(rom_virtaddr[30], (uint16_t*)&nds_conf.PWMPrescaler);	//  PWMPrescaler;
	EE_ReadVariable(rom_virtaddr[31], (uint16_t*)&nds_conf.ADCCycles1);	//  ADCCycles1;
	EE_ReadVariable(rom_virtaddr[32], (uint16_t*)&nds_conf.ADCCycles2);	//  ADCCycles2;
	EE_ReadVariable(rom_virtaddr[33], (uint16_t*)&nds_conf.Polarity);	   //  Polarity;
	EE_ReadVariable(rom_virtaddr[34], (uint16_t*)&nds_conf.SpeedSample);  	   //  SpeedSample;
	EE_ReadVariable(rom_virtaddr[35], (uint16_t*)&nds_conf.PidSampleTime); //  PidSampleTime;
	EE_ReadVariable(rom_virtaddr[36], (uint16_t*)&nds_conf.PidMaxVel);     //
	EE_ReadVariable(rom_virtaddr[37], (uint16_t*)&nds_conf.LimitLowPos); //  LimitLowPos;
	EE_ReadVariable(rom_virtaddr[38], (uint16_t*)&nds_conf.LimitUpPos);     // LimitUpPos

}

/*
static void CONF_saveAll()
{
	EE_WriteVariable(rom_virtaddr[0], nds_conf.puid.ManfId);
	EE_WriteVariable(rom_virtaddr[1], nds_conf.puid.ProductId);
	EE_WriteVariable(rom_virtaddr[2], nds_conf.puid.SerialId);
	EE_WriteVariable(rom_virtaddr[3], nds_conf.puid.VsId);

	EE_WriteVariable(rom_virtaddr[4], nds_conf.hwset.Ver);
	EE_WriteVariable(rom_virtaddr[5], nds_conf.hwset.ID);
	EE_WriteVariable(rom_virtaddr[6], nds_conf.hwset.GID);
	EE_WriteVariable(rom_virtaddr[7], nds_conf.hwset.Buadrate);

	EE_WriteVariable(rom_virtaddr[8], nds_conf.GearRatio_Hv);	// GearRatio_Hv;
	EE_WriteVariable(rom_virtaddr[9], nds_conf.GearRatio_Lv);	// GearRatio_Lv;   // GearRatio = Hv + Lv * 0.01f
	EE_WriteVariable(rom_virtaddr[10], nds_conf.Velmax);	// Velmax;
	EE_WriteVariable(rom_virtaddr[11], nds_conf.AcceMax);	// AcceMax;
	EE_WriteVariable(rom_virtaddr[12], nds_conf.DeceMax);	// DeceMax;
	EE_WriteVariable(rom_virtaddr[13], nds_conf.Jerk);	// Jerk;
	EE_WriteVariable(rom_virtaddr[14], nds_conf.TimerArr);	// TimerArr;
	EE_WriteVariable(rom_virtaddr[15], nds_conf.Curmax);	// Curmax;
	EE_WriteVariable(rom_virtaddr[16], nds_conf.Volmax);	// Volmax;

	//Position PID
	EE_WriteVariable(rom_virtaddr[17], nds_conf.Kp0);	// Kp0;
	EE_WriteVariable(rom_virtaddr[18], nds_conf.Ki0);	// Ki0;
	EE_WriteVariable(rom_virtaddr[19], nds_conf.Kd0);	// Kd0;

	//Current PID
	EE_WriteVariable(rom_virtaddr[20], nds_conf.Kp1);	// Kp1;
	EE_WriteVariable(rom_virtaddr[21], nds_conf.Ki1);	// Ki1;
	EE_WriteVariable(rom_virtaddr[22], nds_conf.Kd1);	// Kd1;

	//Position PID error threshold
	EE_WriteVariable(rom_virtaddr[23], nds_conf.PidErrThres0);	// PidErrThres0;

	//Current PID error threshold
	EE_WriteVariable(rom_virtaddr[24], nds_conf.PidErrThres1);	// PidErrThres1;

	EE_WriteVariable(rom_virtaddr[25], nds_conf.HomeSpeed);	// HomeSpeed;

	//Zero speed from PWM duty
	EE_WriteVariable(rom_virtaddr[26], nds_conf.PWMDutyStart);	// PWMDutyStart;

	EE_WriteVariable(rom_virtaddr[28], nds_conf.MinDistErr);	// Min allowed distant error;

	EE_WriteVariable(rom_virtaddr[29], nds_conf.DeadTime);	// DeadTime

	EE_WriteVariable(rom_virtaddr[30], nds_conf.PWMPrescaler);	//  PWMPrescaler;
	EE_WriteVariable(rom_virtaddr[31], nds_conf.ADCCycles1);	//  ADCCycles1;
	EE_WriteVariable(rom_virtaddr[32], nds_conf.ADCCycles2);	//  ADCCycles2;
	EE_WriteVariable(rom_virtaddr[33], nds_conf.Polarity);	   //  Polarity;
	EE_WriteVariable(rom_virtaddr[34], nds_conf.SpeedSample);  	   //  SpeedSample;
	EE_WriteVariable(rom_virtaddr[35], nds_conf.PidSampleTime); //  PidSampleTime;
	EE_WriteVariable(rom_virtaddr[36], nds_conf.PidMaxVel);     //  PidMaxVel;
	EE_WriteVariable(rom_virtaddr[37], nds_conf.LimitLowPos); //  LimitLowPos;
	EE_WriteVariable(rom_virtaddr[38], nds_conf.LimitUpPos); //  LimitUpPos;
}*/

uint16_t CONF_setID(uint8_t id)
{
	if (id == 0 || id > 253)
		return 0;

	nds_conf.hwset.ID = id;
	uint16_t ret = EE_WriteVariable(rom_virtaddr[5], id);
	if (ret == HAL_OK){
		return 1;
	}
	return 0;
}

uint16_t CONF_setBaudrate(uint16_t baud)
{
	uint16_t ret = EE_WriteVariable(rom_virtaddr[7], baud);
	if (ret == HAL_OK){
		/*reset the MCU*/
		HAL_NVIC_SystemReset();
	}
	/*flash set failed*/
	return 1;
}

void CONF_setMaxVel(uint16_t vel)
{
	nds_conf.Velmax = vel;
	EE_WriteVariable(rom_virtaddr[10], nds_conf.Velmax);
}

void CONF_setMaxAcce(uint16_t acce, uint16_t dece)
{
	nds_conf.AcceMax = acce;
	nds_conf.DeceMax = dece;

	EE_WriteVariable(rom_virtaddr[11], nds_conf.AcceMax);	// AcceMax;
	EE_WriteVariable(rom_virtaddr[12], nds_conf.DeceMax);	// DeceMax;
}


uint8_t CONF_setPWMFreq(uint8_t prescaler, uint16_t period)
{
	if (prescaler == 0 || prescaler >= 72){
		// too small PWM frequency
		return 0;
	}

	if (period < 500 || period > 2000) {
		// wrong setting
		return 0;
	}

	if (HAL_OK != EE_WriteVariable(rom_virtaddr[30], prescaler) ||
		HAL_OK != EE_WriteVariable(rom_virtaddr[14], period)){
		return 0;
	}

	nds_conf.PWMPrescaler = prescaler;
	nds_conf.TimerArr = period;

	HAL_Delay(10);

	HAL_NVIC_SystemReset();

	/*
	// deinit TIMER
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);

	HAL_TIM_PWM_MspDeInit(&htim1);

	MX_TIM1_Init();
	*/

	return 1;
}

void CONF_setPWMController(uint16_t inc, uint16_t deadtime, uint16_t dutyStart)
{
	/* dutys0 --> timerArr, speed from 0 -> velmax
	 * dutyinterval = (timerArr - dutys0)/velmax*/

	if (deadtime < 10 || deadtime > nds_conf.TimerArr - 100){
		return;
	}

	/*10 is the minimize startup duty*/
	if (inc < 2 || inc >= 100){
		return;
	}

	if (dutyStart > nds_conf.TimerArr){
		return;
	}

	nds_conf.DeadTime     = deadtime;
	nds_conf.PWMDutyStart = dutyStart;
	nds_conf.PWMDutyInc   = inc;

	/*Zero speed from PWM duty*/
	EE_WriteVariable(rom_virtaddr[26], nds_conf.PWMDutyStart);
	EE_WriteVariable(rom_virtaddr[27], nds_conf.PWMDutyInc);	// PWMDutyInc;
	EE_WriteVariable(rom_virtaddr[29], nds_conf.DeadTime);
}

void CONF_setSpeedSample(uint16_t sample)
{
	if (sample == 0 || sample >= 100)
		return;

	nds_conf.SpeedSample = sample;
	EE_WriteVariable(rom_virtaddr[34], sample);  	   //  SpeedSample;

}

void CONF_setPolarity(uint8_t polarity)
{
	nds_conf.Polarity = polarity;
	EE_WriteVariable(rom_virtaddr[33], nds_conf.Polarity);
}

void CONF_setPID(uint16_t kp, uint16_t ki, uint16_t kd, uint16_t errThres, uint16_t pidSampleTime)
{
	EE_WriteVariable(rom_virtaddr[17], kp);	// Kp0;
	EE_WriteVariable(rom_virtaddr[18], ki);	// Ki0;
	EE_WriteVariable(rom_virtaddr[19], kd);	// Kd0;
	EE_WriteVariable(rom_virtaddr[23], errThres);	// PidErrThres0;
	EE_WriteVariable(rom_virtaddr[35], pidSampleTime);	// pidSampleTime;
}

void CONF_setPIDMaxVel(uint16_t maxvel)
{
	nds_conf.PidMaxVel = maxvel;
	EE_WriteVariable(rom_virtaddr[36], nds_conf.PidMaxVel);
}

void CONF_setPosLimit(uint8_t lowLimit, uint8_t upLimit)
{
	nds_conf.LimitLowPos = lowLimit;
	nds_conf.LimitUpPos  = upLimit;
	EE_WriteVariable(rom_virtaddr[37], nds_conf.LimitLowPos);
	EE_WriteVariable(rom_virtaddr[38], nds_conf.LimitUpPos);
}

void CONF_setPosPID(uint16_t kp, uint16_t ki, uint16_t kd)
{
	nds_conf.Kp0 = kp;
	nds_conf.Ki0 = ki;
	nds_conf.Kd0 = kd;
	EE_WriteVariable(rom_virtaddr[17], nds_conf.Kp0);
	EE_WriteVariable(rom_virtaddr[18], nds_conf.Ki0);
	EE_WriteVariable(rom_virtaddr[19], nds_conf.Kd0);
}

void CONF_setVelPID(uint16_t kp, uint16_t ki, uint16_t kd)
{
	nds_conf.Kp1 = kp;
	nds_conf.Ki1 = ki;
	nds_conf.Kd1 = kd;
	EE_WriteVariable(rom_virtaddr[20], nds_conf.Kp1);
	EE_WriteVariable(rom_virtaddr[21], nds_conf.Ki1);
	EE_WriteVariable(rom_virtaddr[22], nds_conf.Kd1);
}

void CONF_setUpgrade(uint8_t value)
{
	FLASH_OBProgramInitTypeDef obd;
	obd.OptionType  = OPTIONBYTE_DATA;
	obd.DATAAddress = OB_DATA_ADDRESS_DATA1;
	obd.DATAData    = value;

	HAL_FLASH_Unlock();
	HAL_FLASH_OB_Unlock();

	HAL_FLASHEx_OBErase();
	if ( HAL_FLASHEx_OBProgram(&obd) != HAL_OK )
	{
		HAL_FLASH_OB_Lock();
		HAL_FLASH_Lock();
		return;
	}

	// reset
	HAL_FLASH_OB_Launch();

	// Will not reach
	HAL_FLASH_OB_Lock();
	HAL_FLASH_Lock();
}
