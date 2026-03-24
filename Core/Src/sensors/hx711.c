/*
 * hx711.c
 *
 *  Created on: 2025年9月19日
 *      Author: Huang
 */


#include "hx711.h"
#include "hx711Config.h"
#if (_HX711_USE_FREERTOS == 1)
#include "cmsis_os.h"
#define hx711_delay(x)    osDelay(x)
#else
#define hx711_delay(x)    HAL_Delay(x)
#endif

hx711_t g_hx711;
int32_t hx711_value_sum;
int32_t hx711_ave_samples;
int32_t hx711_eo_data;

void hx711_delay_us(uint32_t cnt)
{
	__HAL_TIM_SET_COUNTER(&htim4, 0);
	while (htim4.Instance->CNT < cnt);
}

void hx711_init(hx711_t *hx711, GPIO_TypeDef *clk_gpio, uint16_t clk_pin, GPIO_TypeDef *dat_gpio, uint16_t dat_pin)
{
  hx711->clk_gpio = clk_gpio;
  hx711->clk_pin = clk_pin;
  hx711->dat_gpio = dat_gpio;
  hx711->dat_pin = dat_pin;
  hx711->gain = 3;

  GPIO_InitTypeDef  gpio = {0};
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Pin = clk_pin;
  HAL_GPIO_Init(clk_gpio, &gpio);
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Pin = dat_pin;
  HAL_GPIO_Init(dat_gpio, &gpio);

  HAL_GPIO_WritePin(hx711->clk_gpio, hx711->clk_pin, GPIO_PIN_SET);
  hx711_delay(10);
  HAL_GPIO_WritePin(hx711->clk_gpio, hx711->clk_pin, GPIO_PIN_RESET);
  hx711_delay(10);

  //hx711_value(hx711);
  //hx711_value(hx711);

  hx711_value_sum = 0;
  hx711_ave_samples = 5;
}

int32_t hx711_value(hx711_t *hx711)
{
  uint32_t data = 0;
  int32_t filler; //to fill the rest of the 32 bits.
  int32_t ret_value; //final value to return after adding the filling and the data together.

  for(int8_t i=0; i<24 ; i++)
  {
    HAL_GPIO_WritePin(hx711->clk_gpio, hx711->clk_pin, GPIO_PIN_SET);
    hx711_delay_us(1);

    data = data << 1;
    if(HAL_GPIO_ReadPin(hx711->dat_gpio, hx711->dat_pin) == GPIO_PIN_SET)
      data ++;

    HAL_GPIO_WritePin(hx711->clk_gpio, hx711->clk_pin, GPIO_PIN_RESET);
    hx711_delay_us(1);
  }

  for( int i = 0; i < hx711->gain; i ++ ) {
	HAL_GPIO_WritePin(hx711->clk_gpio, hx711->clk_pin, GPIO_PIN_SET); //set clock pin to 1.
	hx711_delay_us(1); //delay

	HAL_GPIO_WritePin(hx711->clk_gpio, hx711->clk_pin, GPIO_PIN_RESET); //set clock pin to 0.
	hx711_delay_us(1); //delay, here we are making a clock cycle.
  }

  if( data & 0x800000 ) //here we are checking if theres values in the 24 bits by anding.
    filler = 0xFF000000; //if there are values we add 1's to the last 8 bits which are needed as this is a 32-bit adc.
  else
    filler = 0x00000000; //however, if nothing is in the data we just add 0's.

  ret_value = filler + data; //the return value is the addition of the data with the filler to have the 32-bits.
  return ret_value; //returning the value to be printed.
}

int32_t hx711_value_ave(hx711_t *hx711, uint16_t sample)
{
  int64_t  ave = 0;
  for(uint16_t i=0 ; i<sample ; i++)
  {
    ave += hx711_value(hx711);
    hx711_delay(5);
  }
  int32_t answer = (int32_t)(ave / sample);

  return answer;
}

void hx711_setGain(hx711_t *hx711, uint8_t gain)  //the values should be 32, 64 or 128
{
	if(gain < 64) hx711->gain = 2; //32, channel B
	else if(gain < 128) hx711->gain = 3; //64, channel A
	else hx711->gain = 1; //128, channel A
}

void hx711_tare(hx711_t *hx711, uint16_t sample)
{
  int64_t  ave = 0;
  for(uint16_t i=0 ; i<sample ; i++)
  {
    ave += hx711_value(hx711);
    hx711_delay(5);
  }
  hx711->offset = (int32_t)(ave / sample);
}

void hx711_calibration(hx711_t *hx711, int32_t noload_raw, int32_t load_raw, float scale)
{
  hx711->offset = noload_raw;
  hx711->coef = (load_raw - noload_raw) / scale;
}

float hx711_weight(hx711_t *hx711, uint16_t sample)
{
  int64_t  ave = 0;
  for(uint16_t i=0 ; i<sample ; i++)
  {
    ave += hx711_value(hx711);
    hx711_delay(5);
  }
  int32_t data = (int32_t)(ave / sample);
  float answer =  (data - hx711->offset) / hx711->coef;

  return answer;
}

uint8_t hx711_is_ready(hx711_t *hx711) //making sure that the HX711 module is ready
{
  return HAL_GPIO_ReadPin(hx711->dat_gpio, hx711->dat_pin) == GPIO_PIN_RESET; //reading a value from it, the reseting it.
}

//#############################################################################################
void hx711_coef_set(hx711_t *hx711, float coef)
{
  hx711->coef = coef;
}
//#############################################################################################
float hx711_coef_get(hx711_t *hx711)
{
  return hx711->coef;
}
//#############################################################################################
void hx711_power_down(hx711_t *hx711)
{
  HAL_GPIO_WritePin(hx711->clk_gpio, hx711->clk_pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(hx711->clk_gpio, hx711->clk_pin, GPIO_PIN_SET);
  hx711_delay(1);
}
//#############################################################################################
void hx711_power_up(hx711_t *hx711)
{
  HAL_GPIO_WritePin(hx711->clk_gpio, hx711->clk_pin, GPIO_PIN_RESET);
}

void hx711_proc()
{
	static uint32_t samples = 0;
	static uint32_t lastTick = 0;

	uint32_t curTick = HAL_GetTick();

	if (!hx711_is_ready(&g_hx711)){
		lastTick = curTick;
		return;
	}

	if (curTick - lastTick > 5){
		hx711_value_sum += hx711_value(&g_hx711);
		samples ++;
	}

	if (samples >= hx711_ave_samples){
		/*获取平均数*/
		hx711_eo_data = hx711_value_sum / samples;

		samples = 0;
		hx711_value_sum = 0;
	}

	lastTick = curTick;
}
