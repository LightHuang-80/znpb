/*
 * gpioexti.c
 *
 *  Created on: 2025年9月20日
 *      Author: Huang
 */


#include "main.h"
#include "gpio.h"
#include "gpioexti.h"

volatile uint8_t g_touchedEvent = 0;
volatile uint8_t g_touchedState[3] = {};

void GPIO_updateTouchedState()
{
	GPIO_PinState pinState;

	pinState = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);
	if (pinState == GPIO_PIN_SET){
		/*接触传感器，离开金属*/
		g_touchedState[0] = GPIO_PIN_RESET;
	}else{
		/*靠近了金属*/
		g_touchedState[0] = GPIO_PIN_SET;
	}

	pinState = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13);
	if (pinState == GPIO_PIN_SET){
		/*接触传感器，离开金属*/
		g_touchedState[1] = GPIO_PIN_RESET;
	}else{
		/*靠近了金属*/
		g_touchedState[1] = GPIO_PIN_SET;
	}

	pinState = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
	if (pinState == GPIO_PIN_SET){
		/*接触传感器，离开金属*/
		g_touchedState[2] = GPIO_PIN_RESET;
	}else{
		/*靠近了金属*/
		g_touchedState[2] = GPIO_PIN_SET;
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin){
	case GPIO_PIN_12:{
		/*INT3, 左下接口*/
		g_touchedEvent = 3;
		}break;

	case GPIO_PIN_13:{
		/*INT2, 左中接口*/
		g_touchedEvent = 2;
		}break;
	case GPIO_PIN_14:{
		/*INT1, 左上接口*/
		g_touchedEvent = 1;
		}break;
	}

	GPIO_updateTouchedState();
}
