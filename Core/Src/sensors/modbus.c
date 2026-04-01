/*
 * modbus.c
 *
 *  Created on: 2025年8月13日
 *      Author: Huang
 */

#include "main.h"
#include "usart.h"
#include "tim.h"
#include <string.h>

#include "ndsconf.h"

// 缓冲区
#define MB_RX_BUF_SIZE 256
#define MB_TX_BUF_SIZE 8

/* 角度传感器5个 */
#define MB_DEVICE_COUNT 5

uint8_t mb_rxBuffer[MB_RX_BUF_SIZE];
uint8_t mb_txBuffer[MB_DEVICE_COUNT][MB_TX_BUF_SIZE] = {}; // 轮询数据读取从站的保持寄存器

volatile uint16_t mb_rxLen = 0;
volatile uint8_t  mb_frameReceived = 0;

/* 角度传感器数据 (ID 1-5) */
volatile float   deviceAngle[5];
volatile float   deviceSpeed[5];

// CRC-16-Modbus 计算函数（参考之前回答）
uint16_t modbus_crc16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    const uint16_t polynomial = 0xA001;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// 初始化
void Modbus_init(void) {

	// 初始化查询包，ID:0x01，功能码:0x03，寄存器地址:0x0380
	// 数据6位
	mb_txBuffer[0][0] = 0x01;
	mb_txBuffer[0][1] = 0x03;
	mb_txBuffer[0][2] = 0x03;
	mb_txBuffer[0][3] = 0x80;
	mb_txBuffer[0][4] = 0x00;
	mb_txBuffer[0][5] = 0x06;
	mb_txBuffer[0][6] = 0xC4;
	mb_txBuffer[0][7] = 0x64;

	// 初始化查询包，ID:0x02，功能码:0x03，寄存器地址:0x0380
	// 数据6位
	mb_txBuffer[1][0] = 0x02;
	mb_txBuffer[1][1] = 0x03;
	mb_txBuffer[1][2] = 0x03;
	mb_txBuffer[1][3] = 0x80;
	mb_txBuffer[1][4] = 0x00;
	mb_txBuffer[1][5] = 0x06;
	mb_txBuffer[1][6] = 0xC4;
	mb_txBuffer[1][7] = 0x57;

	// 初始化查询包，ID:0x04，功能码:0x03，寄存器地址:0x0380
	// 数据6位
	mb_txBuffer[2][0] = 0x04;
	mb_txBuffer[2][1] = 0x03;
	mb_txBuffer[2][2] = 0x03;
	mb_txBuffer[2][3] = 0x80;
	mb_txBuffer[2][4] = 0x00;
	mb_txBuffer[2][5] = 0x06;
	mb_txBuffer[2][6] = 0xC4;
	mb_txBuffer[2][7] = 0x31;

	// 初始化查询包，ID:0x05，功能码:0x03，寄存器地址:0x0380
	// 数据6位
	mb_txBuffer[3][0] = 0x05;
	mb_txBuffer[3][1] = 0x03;
	mb_txBuffer[3][2] = 0x03;
	mb_txBuffer[3][3] = 0x80;
	mb_txBuffer[3][4] = 0x00;
	mb_txBuffer[3][5] = 0x06;
	mb_txBuffer[3][6] = 0xC5;
	mb_txBuffer[3][7] = 0xE0;

	// 初始化查询包，ID:0x03，功能码:0x03，寄存器地址:0x0380
	// 数据6位
	mb_txBuffer[4][0] = 0x03;
	mb_txBuffer[4][1] = 0x03;
	mb_txBuffer[4][2] = 0x03;
	mb_txBuffer[4][3] = 0x80;
	mb_txBuffer[4][4] = 0x00;
	mb_txBuffer[4][5] = 0x06;
	mb_txBuffer[4][6] = 0xC5;
	mb_txBuffer[4][7] = 0x86;

	mb_rxLen = 0;
    // 启动定时器（10 ms 周期）
    HAL_TIM_Base_Start_IT(&htim2);
    // 启动接收
    HAL_UARTEx_ReceiveToIdle_IT(&huart2, mb_rxBuffer, MB_RX_BUF_SIZE);
}

// 定时器中断：每 33 ms 发送查询
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	static uint8_t txIdx = 0;
    if (htim->Instance == TIM2) {
        // 发送查询包
        HAL_UART_Transmit(&huart2, mb_txBuffer[txIdx], MB_TX_BUF_SIZE, HAL_MAX_DELAY);
        txIdx += 1;
       	if (txIdx >= MB_DEVICE_COUNT){
       		txIdx = 0;
       	}
    }
}

// 接收回调：处理完整帧（空闲中断方式）- 仅USART2
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART2) {
    	mb_rxLen = Size;
        mb_frameReceived = 1; // 标记帧接收完成
        // 重新启动接收
        HAL_UARTEx_ReceiveToIdle_IT(&huart2, mb_rxBuffer, MB_RX_BUF_SIZE);
    }
}

// 处理 Modbus 帧并换算角度
int Process_Modbus_Frame(uint8_t *buf, uint16_t len) {
	if (len < 7) return -1; // 最小响应：地址(1) + 功能码(1) + 字节计数(1) + 数据(2) + CRC(2)

	uint8_t iD = buf[0];
	if (iD > 5 || iD == 0) return -1;  // Illegal device id (1-5)

    // 验证 CRC
    uint16_t crc_received = (buf[len-1] << 8) | buf[len-2];
    uint16_t crc_calculated = modbus_crc16(buf, len-2);
    if (crc_received != crc_calculated) {
        return -1; // CRC 错误
    }

    // 检查功能码
    if (buf[1] != 0x03) {
        return -1; // 不是读响应
    }

    // 角度传感器 (ID = 1-5)
    // 检查字节计数
    if (buf[2] != 0x0C) {
        return -1; // 格式错误
    }

    // 提取 状态码
    int16_t  raw_status = (buf[11] << 8) | buf[12];
    if (raw_status != 0){
    	return -1;
    }

    // 提取 21位角度数据
    uint32_t raw_data = (buf[3] << 24) | (buf[4] << 16) | (buf[5] << 8) | buf[6];
    uint32_t encoder_value = raw_data & 0x1FFFFF; // 取低 21 位

    // 换算角度, 2097152(2^21) / 360 = 5825.4222
    float angle = (float)encoder_value / 5825.4222f;
    deviceAngle[iD - 1] = angle - 180.0f;

    // 提取 角速度, 21bit 编码器 0.2861 rpm (600000 / 2^21）
    int16_t  raw_speed = (buf[13] << 8) | buf[14];
    deviceSpeed[iD - 1] = raw_speed * 0.2861f;

    return 0;
}

void Modbus_proc()
{
	if (mb_frameReceived){
		mb_frameReceived = 0;

		if (mb_rxLen > 0){
			Process_Modbus_Frame(mb_rxBuffer, mb_rxLen);
			mb_rxLen = 0;
		}
	}
}

/**
 * @brief 复位编码器
 * @param device_id 设备ID (1-5)
 * @param reset_type 复位类型值，直接写入0x3C1寄存器
 *        - 0x8000: 位置清零
 *        - 0x4000: 复位圈数
 *        - 0x2000: 复位全部错误
 *        - 0x1000: 进入线性校准状态
 *        - 可组合使用，如 0xC000 = 位置清零 + 复位圈数
 * @return 0:成功, -1:参数错误
 */
int Modbus_resetEncoder(uint8_t device_id, uint16_t reset_type)
{
	if (device_id < 1 || device_id > 5) {
		return -1;
	}

	uint8_t frame[16];
	uint16_t crc;

	// 1. 停止TIM2定时器，暂停正常轮询
	HAL_TIM_Base_Stop_IT(&htim2);

	// 短暂延时，等待总线空闲
	HAL_Delay(10);

	// 2. 发送解锁命令 (功能码0x10，写多个寄存器)
	// 寄存器地址: 0x03C2, 写入2个寄存器
	// 数据: 0x000F (读写锁标志), 0x0000 (读写锁密码)
	frame[0] = device_id;           // 从站地址
	frame[1] = 0x10;                // 功能码: 写多个寄存器
	frame[2] = 0x03;                // 寄存器起始地址高字节
	frame[3] = 0xC2;                // 寄存器起始地址低字节 (0x03C2)
	frame[4] = 0x00;                // 寄存器数量高字节
	frame[5] = 0x02;                // 寄存器数量低字节 (2个寄存器)
	frame[6] = 0x04;                // 字节计数 (2个寄存器 * 2字节)
	frame[7] = 0x00;                // 读写锁标志高字节
	frame[8] = 0x0F;                // 读写锁标志低字节 (0x000F)
	frame[9] = 0x00;                // 读写锁密码高字节
	frame[10] = 0x00;               // 读写锁密码低字节 (0x0000)

	// 计算CRC (小端序: 先低字节后高字节)
	crc = modbus_crc16(frame, 11);
	frame[11] = crc & 0xFF;         // CRC低字节
	frame[12] = (crc >> 8) & 0xFF;  // CRC高字节

	// 发送解锁命令
	HAL_UART_Transmit(&huart2, frame, 13, 500);

	// 等待解锁命令处理完成
	HAL_Delay(30);

	// 3. 发送复位命令 (功能码0x06，写单个寄存器)
	// 寄存器地址: 0x03C1, 写入reset_type值
	frame[0] = device_id;           // 从站地址
	frame[1] = 0x06;                // 功能码: 写单个寄存器
	frame[2] = 0x03;                // 寄存器地址高字节
	frame[3] = 0xC1;                // 寄存器地址低字节 (0x03C1)
	frame[4] = (reset_type >> 8) & 0xFF;   // 寄存器值高字节
	frame[5] = reset_type & 0xFF;          // 寄存器值低字节

	// 计算CRC (小端序)
	crc = modbus_crc16(frame, 6);
	frame[6] = crc & 0xFF;          // CRC低字节
	frame[7] = (crc >> 8) & 0xFF;   // CRC高字节

	// 发送复位命令
	HAL_UART_Transmit(&huart2, frame, 8, 500);

	// 等待复位命令处理完成
	HAL_Delay(20);

	// 4. 恢复TIM2定时器，继续正常轮询
	HAL_TIM_Base_Start_IT(&htim2);

	return 0;
}

/**
 * @brief 重启编码器设备 (使用功能码0x41)
 * @param device_id 设备ID (1-5)
 * @return 0:成功, -1:参数错误
 */
int Modbus_restartEncoder(uint8_t device_id)
{
	if (device_id < 1 || device_id > 5) {
		return -1;
	}

	uint8_t frame[4];
	uint16_t crc;

	// 停止TIM2定时器
	HAL_TIM_Base_Stop_IT(&htim2);

	// 短暂延时
	HAL_Delay(10);

	// 发送重启命令 (功能码0x41)
	frame[0] = device_id;           // 从站地址
	frame[1] = 0x41;                // 功能码: 设备重启

	// 计算CRC (小端序)
	crc = modbus_crc16(frame, 2);
	frame[2] = crc & 0xFF;          // CRC低字节
	frame[3] = (crc >> 8) & 0xFF;   // CRC高字节

	// 发送重启命令
	HAL_UART_Transmit(&huart2, frame, 4, 500);

	// 等待设备重启
	HAL_Delay(100);

	// 恢复TIM2定时器
	HAL_TIM_Base_Start_IT(&htim2);

	return 0;
}
