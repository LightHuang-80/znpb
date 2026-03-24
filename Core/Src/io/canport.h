/*
 * canport.h
 *
 *  Created on: 2024年8月23日
 *      Author: Huang
 */

#ifndef SRC_IO_CANPORT_H_
#define SRC_IO_CANPORT_H_

#include <stdint.h>
#include <stdbool.h>
#include "CO_fifo.h"

typedef struct {
    uint32_t ident;  /*!< Standard identifier */
    uint8_t dlc;     /*!< Data length */
    uint8_t data[8]; /*!< Received data */
} CANrxMsg_t;


/* Received message object */
typedef struct {
    uint16_t ident;
    uint16_t mask;
    void*    object;
    void (*CANrx_callback)(void* object, void* message);
} CANrx_t;

/* Transmit message object */
typedef struct {
    uint32_t ident;
    uint8_t  DLC;
    uint8_t  data[8];
    volatile bool_t bufferFull;
    volatile bool_t syncFlag;
} CANtx_t;

typedef struct canport_ {
	CAN_HandleTypeDef* hcan;
	uint32_t           CANerrorStatus;
	uint32_t           errOld;
	uint16_t           CANtxCount;
	CO_fifo_t          rxBuf;
	uint16_t           txSize;
	CANtx_t            txArray[4];
	uint32_t           primask_send;
	bool_t             isopen;
	bool_t             firstCANtxMessage;
	bool_t             bufferInhibitFlag;
}canport_t;

extern canport_t canport;

/* Access to received CAN message */
#define CO_CANrxMsg_readIdent(msg) ((uint16_t)(((CANrxMsg_t*)(msg)))->ident)
#define CO_CANrxMsg_readDLC(msg)   ((uint8_t)(((CANrxMsg_t*)(msg)))->dlc)
#define CO_CANrxMsg_readData(msg)  ((uint8_t*)(((CANrxMsg_t*)(msg)))->data)

#define CO_LOCK_CAN_SEND(CAN_PORT)                                                                                   \
    do {                                                                                                               \
        (CAN_PORT)->primask_send = __get_PRIMASK();                                                                  \
        __disable_irq();                                                                                               \
    } while (0)
#define CO_UNLOCK_CAN_SEND(CAN_PORT) __set_PRIMASK((CAN_PORT)->primask_send)

/**
 * CAN error status bitmasks.
 *
 * CAN warning level is reached, if CAN transmit or receive error counter is
 * more or equal to 96. CAN passive level is reached, if counters are more or
 * equal to 128. Transmitter goes in error state 'bus off' if transmit error
 * counter is more or equal to 256.
 */
typedef enum {
    CO_CAN_ERRTX_WARNING = 0x0001,  /**< 0x0001, CAN transmitter warning */
    CO_CAN_ERRTX_PASSIVE = 0x0002,  /**< 0x0002, CAN transmitter passive */
    CO_CAN_ERRTX_BUS_OFF = 0x0004,  /**< 0x0004, CAN transmitter bus off */
    CO_CAN_ERRTX_OVERFLOW = 0x0008, /**< 0x0008, CAN transmitter overflow */

    CO_CAN_ERRTX_PDO_LATE = 0x0080, /**< 0x0080, TPDO is outside sync window */

    CO_CAN_ERRRX_WARNING = 0x0100,  /**< 0x0100, CAN receiver warning */
    CO_CAN_ERRRX_PASSIVE = 0x0200,  /**< 0x0200, CAN receiver passive */
    CO_CAN_ERRRX_OVERFLOW = 0x0800, /**< 0x0800, CAN receiver overflow */

    CO_CAN_ERR_WARN_PASSIVE = 0x0303/**< 0x0303, combination */
} CO_CAN_ERR_status_t;


/**
 * Return values of some CANopen functions. If function was executed
 * successfully it returns 0 otherwise it returns <0.
 */
typedef enum {
    CO_ERROR_NO = 0,                /**< Operation completed successfully */
    CO_ERROR_ILLEGAL_ARGUMENT = -1, /**< Error in function arguments */
    CO_ERROR_OUT_OF_MEMORY = -2,    /**< Memory allocation failed */
    CO_ERROR_TIMEOUT = -3,          /**< Function timeout */
    CO_ERROR_ILLEGAL_BAUDRATE = -4, /**< Illegal baudrate passed to function
                                         CO_CANmodule_init() */
    CO_ERROR_RX_OVERFLOW = -5,      /**< Previous message was not processed
                                         yet */
    CO_ERROR_RX_PDO_OVERFLOW = -6,  /**< previous PDO was not processed yet */
    CO_ERROR_RX_MSG_LENGTH = -7,    /**< Wrong receive message length */
    CO_ERROR_RX_PDO_LENGTH = -8,    /**< Wrong receive PDO length */
    CO_ERROR_TX_OVERFLOW = -9,      /**< Previous message is still waiting,
                                         buffer full */
    CO_ERROR_TX_PDO_WINDOW = -10,   /**< Synchronous TPDO is outside window */
    CO_ERROR_TX_UNCONFIGURED = -11, /**< Transmit buffer was not configured
                                         properly */
    CO_ERROR_OD_PARAMETERS = -12,   /**< Error in Object Dictionary parameters*/
    CO_ERROR_DATA_CORRUPT = -13,    /**< Stored data are corrupt */
    CO_ERROR_CRC = -14,             /**< CRC does not match */
    CO_ERROR_TX_BUSY = -15,         /**< Sending rejected because driver is
                                         busy. Try again */
    CO_ERROR_WRONG_NMT_STATE = -16, /**< Command can't be processed in current
                                         state */
    CO_ERROR_SYSCALL = -17,         /**< Syscall failed */
    CO_ERROR_INVALID_STATE = -18,   /**< Driver not ready */
    CO_ERROR_NODE_ID_UNCONFIGURED_LSS = -19 /**< Node-id is in LSS unconfigured
                                         state. If objects are handled properly,
                                         this may not be an error. */
} CO_ReturnError_t;

/* CAN masks for identifiers */
#define CANID_MASK 0x07FF /*!< CAN standard ID mask */
#define FLAG_RTR   0x8000 /*!< RTR flag, part of identifier */

CO_ReturnError_t CANmodule_init(CAN_HandleTypeDef* hcan, uint16_t CANbitRate);
void CANmodule_process();
void CANclearPendingSync();
uint8_t CANreadMsg(CANrxMsg_t* buffer);
void CANsetNormalMode(CAN_HandleTypeDef* hcan);
CO_ReturnError_t CANsend(CANtx_t* buffer);
#endif /* SRC_IO_CANPORT_H_ */
