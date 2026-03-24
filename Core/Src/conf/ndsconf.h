/*
 * ndsconf.h
 *
 *  Created on: 2023年12月17日
 *      Author: Administrator
 */

#ifndef INC_NDSCONF_H_
#define INC_NDSCONF_H_

#include <stdint.h>

/* ID definitions*/
#define BROAD_CAST_ID	0x00
#define MASTER_ID       0x01
#define UNINIT_ID       0xFE

/*Adc index,
 * ADC5 -- Pos reference
 * ADC7 -- Power reference
 * ADC9 -- Motor current reference*/
#define POS_REF_SAMPLE  0x00
#define PWR_REF_SAMPLE  0x01
#define CUR_REF_SAMPLE  0x02

/* CMD definitions*/
#define SERVO_MOVE_TIME_WRITE 		1 	// 7
#define SERVO_MOVE_TIME_READ 		2 	// 3
#define SERVO_MOVE                  1
#define SERVO_READ_POS              2

#define SERVO_SET_ID                0xCD

#define SERVO_MOVE_TIME_WAIT_WRITE 	7 	// 7
#define SERVO_MOVE_TIME_WAIT_READ 	8 	// 3
#define SERVO_MOVE_START 			11 	// 3
#define SERVO_MOVE_STOP 			12 	// 3
#define SERVO_ID_WRITE 				13 	// 4
#define SERVO_ID_READ 				14 	// 3
#define SERVO_ANGLE_OFFSET_ADJUST 	17 	// 4
#define SERVO_ANGLE_OFFSET_WRITE 	18 	// 3
#define SERVO_ANGLE_OFFSET_READ 	19 	// 3
#define SERVO_ANGLE_LIMIT_WRITE 	20 	// 7
#define SERVO_ANGLE_LIMIT_READ 		21 	// 3
#define SERVO_VIN_LIMIT_WRITE 		22 	// 7
#define SERVO_VIN_LIMIT_READ 		23 	// 3
#define SERVO_TEMP_MAX_LIMIT_WRITE 	24 	// 4
#define SERVO_TEMP_MAX_LIMIT_READ 	25 	// 3
#define SERVO_TEMP_READ 			26 	// 3
#define SERVO_VIN_READ 				27 	// 3
#define SERVO_POS_READ 				28 	// 3
#define SERVO_OR_MOTOR_MODE_WRITE 	29 	// 7
#define SERVO_OR_MOTOR_MODE_READ 	30 	// 3
#define SERVO_LOAD_OR_UNLOAD_WRITE 	31 	// 4
#define SERVO_LOAD_OR_UNLOAD_READ 	32 	// 3
#define SERVO_LED_CTRL_WRITE 		33 	// 4
#define SERVO_LED_CTRL_READ 		34 	// 3
#define SERVO_LED_ERROR_WRITE 		35 	// 4
#define SERVO_LED_ERROR_READ 		36 	// 3

#define SERVO_POS_LIMIT             20
#define SERVO_UPGRADE               40

#define SERVO_STALL_GUARD           60

#define SERVO_VOL_CUR_REQ			62  // 0
#define SERVO_VOL_CUR_RESP			63  // 8
#define SERVO_RUNTARGET_WITH_SPEED  64
#define SERVO_PID_MOTION		    66  // 8
#define SERVO_SPEED_MOTION          68
#define SERVO_SCURVE_MOTION         70
#define SERVO_MOTOR_POLARITY        71
#define SERVO_MOTOR_POLARITY_QUERY  72
#define SERVO_POS_SPEED_REQ         80  // 0
#define SERVO_POS_SPEED_RESP        81  // 8
#define SERVO_SPEED_SAMPLE          82

/*
 * Command 0x53 ~ 0x57 reserved
 * */
#define SERVO_PID_UPDATE            88
#define SERVO_PID_PMS_UPDATE        89
#define SERVO_PID_QUERY             90

#define SERVO_PID_MAXVEL_UPDATE     92
#define SERVO_PID_MAXVEL_QUERY      94

#define SERVO_PWM_UPDATE            96  // 8
#define SERVO_PWM_QUERY             98
#define SERVO_PWM_RESP              99

#define SERVO_UUID_QUERY            120
#define SERVO_UUID_RESP             121
#define SERVO_HW_QUERY              122
#define SERVO_HW_RESP               123
/*N firmware configuration*/
typedef struct NDSHWSetting_ {
	uint16_t  Ver;
	uint8_t  ID;
	uint8_t  GID;
    uint8_t  Buadrate;
}NDSHWSetting_t;

typedef struct NDSProductUUID_ {
	uint16_t ManfId;
	uint16_t ProductId;
	uint16_t SerialId;
	uint16_t VsId;
}NDSProductUUID_t;

/*D ds global configuration*/
typedef struct NDSConf_ {
	NDSProductUUID_t puid;
	NDSHWSetting_t   hwset;
	uint16_t GearRatio_Hv;
	uint16_t GearRatio_Lv;   // GearRatio = Hv + Lv * 0.01f
	uint16_t Velmax;
	uint16_t AcceMax;
	uint16_t DeceMax;
	uint16_t Jerk;
	uint16_t TimerArr;
	uint16_t Curmax;
	uint16_t Volmax;

	/*Position PID*/
	uint16_t Kp0;
	uint16_t Ki0;
	uint16_t Kd0;

	/*Current PID*/
	uint16_t Kp1;
	uint16_t Ki1;
	uint16_t Kd1;

	/*Position PID error threshold*/
	uint16_t PidErrThres0;

	/*Current PID error threshold*/
	uint16_t PidErrThres1;

	uint16_t HomeSpeed;

	/*Zero speed from PWM duty*/
	uint16_t PWMDutyStart;

	/*PWM duty increased interval*/
	uint16_t PWMDutyInc;

	/*Min distant allowed error*/
	uint16_t MinDistErr;

	/*PWM dead time*/
	uint16_t DeadTime;

	uint8_t  PWMPrescaler;
	uint8_t  ADCCycles1;
	uint8_t  ADCCycles2;
	uint8_t  Polarity;
	uint16_t SpeedSample;
	uint16_t PidSampleTime;
	uint16_t PidMaxVel;
	uint16_t LimitLowPos;
	uint16_t LimitUpPos;
	uint16_t SCurveSpeedCoeff;

}NDSConf_t;

extern NDSConf_t  nds_conf;

void CONF_init();
void CONF_load();
uint16_t CONF_setID(uint8_t id);
uint16_t CONF_setBaudrate(uint16_t baud);
void CONF_setMaxVel(uint16_t vel);
void CONF_setMaxAcce(uint16_t acce, uint16_t dece);

uint8_t CONF_setPWMFreq(uint8_t prescaler, uint16_t period);
void CONF_setPWMController(uint16_t inc, uint16_t deadtime, uint16_t dutyStart);
void CONF_setPID(uint16_t kp, uint16_t ki, uint16_t kd, uint16_t errThres, uint16_t pidSampleTime);
void CONF_setPolarity(uint8_t polarity);
void CONF_setSCurve(uint16_t acce, uint16_t jerk, uint16_t sampletime, uint16_t maxvel, uint16_t speedCoeff);
void CONF_setPIDMaxVel(uint16_t maxvel);
void CONF_setSpeedSample(uint16_t sample);
void CONF_setPosLimit(uint8_t lowLimit, uint8_t upLimit);
void CONF_setUpgrade(uint8_t value);
void CONF_setPosPID(uint16_t kp, uint16_t ki, uint16_t kd);
void CONF_setVelPID(uint16_t kp, uint16_t ki, uint16_t kd);
#endif /* INC_NDSCONF_H_ */
