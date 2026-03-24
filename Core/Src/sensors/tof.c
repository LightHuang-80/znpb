/*
 * tof.c
 *
 *  TOF距离传感器模块 - 使用UART3独立通信（单字节中断接收）
 *  Created on: 2025年
 *      Author: Huang
 */

#include "main.h"
#include "usart.h"
#include <string.h>
#include "tof.h"

// 缓冲区
#define TOF_RX_BUF_SIZE 32

uint8_t tof_rxBuffer[TOF_RX_BUF_SIZE];
uint8_t tof_rxByte;  // 单字节接收缓冲

volatile uint16_t tof_rxLen = 0;
volatile uint8_t  tof_frameReceived = 0;
volatile uint8_t  tof_rxIndex = 0;
volatile uint32_t tof_lastRxTick = 0;

/* 距离传感器数据 */
volatile uint16_t deviceDistance = 0;  // 距离值，单位mm
volatile uint8_t  distanceValid = 0;   // 距离数据有效标志

/* 调试计数器 */
volatile uint32_t tof_debugRxCount = 0;    // 接收中断计数
volatile uint32_t tof_debugFrameCount = 0; // 帧处理计数
volatile uint32_t tof_debugErrCount = 0;   // 错误恢复计数

// 接收超时时间（ms）- 超过此时间无数据则认为UART异常
#define TOF_RX_TIMEOUT 1000

// CRC-16-Modbus 计算函数
static uint16_t tof_crc16(const uint8_t *data, uint16_t length) {
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

// 重置UART3接收
static void TOF_resetUart(void) {
    tof_debugErrCount++;
    
    // 清除错误标志
    __HAL_UART_CLEAR_OREFLAG(&huart3);
    __HAL_UART_CLEAR_NEFLAG(&huart3);
    __HAL_UART_CLEAR_FEFLAG(&huart3);
    
    // 中止当前操作
    HAL_UART_Abort(&huart3);
    
    // 重置缓冲区
    tof_rxIndex = 0;
    
    // 重新启动接收
    HAL_UART_Receive_IT(&huart3, &tof_rxByte, 1);
    
    // 更新时间戳
    tof_lastRxTick = HAL_GetTick();
}

// 初始化TOF模块
void TOF_init(void) {
    tof_rxLen = 0;
    tof_frameReceived = 0;
    tof_rxIndex = 0;
    tof_lastRxTick = HAL_GetTick();
    deviceDistance = 0;
    distanceValid = 0;
    
    // 启动单字节接收中断
    HAL_UART_Receive_IT(&huart3, &tof_rxByte, 1);
}

// 处理TOF响应帧
static int TOF_processFrame(uint8_t *buf, uint16_t len) {
    // TOF响应格式: [ID, 0x03, 字节数(0x02), 数据H, 数据L, CRC_L, CRC_H]
    if (len < 7) return -1;
    
    // 检查设备地址
    if (buf[0] != 0x06) return -1;
    
    // 检查功能码
    if (buf[1] != 0x03) return -1;
    
    // 检查字节数
    if (buf[2] != 0x02) return -1;
    
    // 验证CRC
    uint16_t crc_received = (buf[len-1] << 8) | buf[len-2];
    uint16_t crc_calculated = tof_crc16(buf, len-2);
    if (crc_received != crc_calculated) {
        return -1; // CRC错误
    }
    
    // 检查是否无效值（0xFF表示无效/超出量程）
    if (buf[3] == 0xFF) {
        distanceValid = 0;
        return -1;
    }
    // 提取距离值（高字节在前）
    deviceDistance = (buf[3] << 8) | buf[4];
    distanceValid = 1;
    
    return 0;
}

// TOF处理函数 - 在主循环中调用
void TOF_proc(void) {
    uint32_t tick = HAL_GetTick();
    
    // 检查接收超时 - 如果长时间没收到数据，重置UART
    if ((tick - tof_lastRxTick) > TOF_RX_TIMEOUT) {
        TOF_resetUart();
    }
    
    // 检查帧超时（空闲检测）- 5ms无数据表示帧结束
    if (tof_rxIndex > 0 && (tick - tof_lastRxTick) > 5) {
        tof_rxLen = tof_rxIndex;
        tof_frameReceived = 1;
        tof_rxIndex = 0;
    }
    
    // 处理接收到的数据
    if (tof_frameReceived) {
        tof_frameReceived = 0;
        tof_debugFrameCount++;
        
        if (tof_rxLen > 0) {
            TOF_processFrame(tof_rxBuffer, tof_rxLen);
            tof_rxLen = 0;
        }
    }
}

// UART3接收回调 - 每接收一个字节调用一次
void TOF_RxCallback(void) {
    HAL_StatusTypeDef status;
    
    tof_debugRxCount++;
    tof_lastRxTick = HAL_GetTick();
    
    // 保存数据到缓冲区
    if (tof_rxIndex < TOF_RX_BUF_SIZE) {
        tof_rxBuffer[tof_rxIndex++] = tof_rxByte;
    } else {
        tof_rxIndex = 0;  // 缓冲区溢出，重置
    }
    
    // 继续接收下一个字节，检查返回值
    status = HAL_UART_Receive_IT(&huart3, &tof_rxByte, 1);
    if (status != HAL_OK) {
        // 接收启动失败，尝试重置
        TOF_resetUart();
    }
}
