/*
 * canport.c
 *
 *  Created on: 2024年8月23日
 *      Author: Huang
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ndsconf.h"
#include "can.h"
#include "CO_fifo.h"
#include "canport.h"

#define DATABUF_SIZE  300
uint8_t CANDataBuf[DATABUF_SIZE];

canport_t canport = {
		0,            //CAN_HandleTypeDef* hcan;
		0,            //uint32_t CANerrorStatus;
		0,			  //uint32_t errOld;
		0,            //uint16_t CANtxCount;
		{0},          //CO_fifo_t rxBuf;
		0,            //uint16_t  txSize;
		{{0}},        //CANtx_t   txArray[4];
		0,            //uint32_t  primask_send;
		0,            //bool_t    isopen;
		0,            //bool_t    firstCANtxMessage;
		0             //bool_t    bufferInhibitFlag;
};

void
CANsetConfigurationMode(CAN_HandleTypeDef* hcan) {
    /* Put CAN module in configuration mode */
    if (hcan != NULL) {
#ifdef CO_STM32_FDCAN_Driver
        HAL_FDCAN_Stop(hcan);
#else
        HAL_CAN_Stop(hcan);
        canport.isopen = false;
#endif
    }
}

void
CANsetNormalMode(CAN_HandleTypeDef* hcan) {
    /* Put CAN module in normal mode */
    if (hcan != NULL) {
#ifdef CO_STM32_FDCAN_Driver
        if (HAL_FDCAN_Start((hcan) == HAL_OK)
#else
        if (HAL_CAN_Start(hcan) == HAL_OK)
#endif
        {
            canport.isopen = true;
        }
    }
}

CO_ReturnError_t
CANmodule_init(CAN_HandleTypeDef* hcan, uint16_t CANbitRate) {

	canport.hcan = hcan;

    canport.CANerrorStatus = 0;
    canport.isopen = false;

    CO_fifo_init(&(canport.rxBuf), (char *)CANDataBuf, DATABUF_SIZE);

    canport.bufferInhibitFlag = false;
    canport.firstCANtxMessage = true;
    canport.CANtxCount = 0U;
    canport.errOld = 0U;

    canport.txSize = 4;
    for (uint16_t i = 0U; i < canport.txSize; i++) {
       canport.txArray[i].bufferFull = false;
    }
    /*
     * Configure global filter that is used as last check if message did not pass any of other filters:
     *
     * We do not rely on hardware filters in this example
     * and are performing software filters instead
     *
     * Accept non-matching standard ID messages
     * Reject non-matching extended ID messages
     */

#ifdef CO_STM32_FDCAN_Driver
    if (HAL_FDCAN_ConfigGlobalFilter(((CANopenNodeSTM32*)CANptr)->CANHandle, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_REJECT,
                                     FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE)
        != HAL_OK) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
#else
    CAN_FilterTypeDef FilterConfig;
#if defined(CAN)
    FilterConfig.FilterBank = 0;
#else
    if (hcan->Instance == CAN1) {
        FilterConfig.FilterBank = 0;
    } else {
        FilterConfig.FilterBank = 14;
    }
#endif

    FilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    FilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    FilterConfig.FilterIdHigh = 0x0;
    FilterConfig.FilterIdLow = 0x0;
    FilterConfig.FilterMaskIdHigh = 0x0;
    FilterConfig.FilterMaskIdLow = 0x0;
    FilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;

    FilterConfig.FilterActivation = ENABLE;
    FilterConfig.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(hcan, &FilterConfig) != HAL_OK) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
#endif
    /* Enable notifications */
    /* Activate the CAN notification interrupts */
#ifdef CO_STM32_FDCAN_Driver
    if (HAL_FDCAN_ActivateNotification(((CANopenNodeSTM32*)CANptr)->CANHandle,
                                       0 | FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO1_NEW_MESSAGE
                                           | FDCAN_IT_TX_COMPLETE | FDCAN_IT_TX_FIFO_EMPTY | FDCAN_IT_BUS_OFF
                                           | FDCAN_IT_ARB_PROTOCOL_ERROR | FDCAN_IT_DATA_PROTOCOL_ERROR
                                           | FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_ERROR_WARNING,
                                       0xFFFFFFFF)
        != HAL_OK) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
#else
    if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING
                                           | CAN_IT_RX_FIFO1_MSG_PENDING
                                           | CAN_IT_TX_MAILBOX_EMPTY)
        != HAL_OK) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
#endif

    return CO_ERROR_NO;
}

/******************************************************************************/
void
CANmodule_disable() {
    if (canport.hcan != NULL) {
#ifdef CO_STM32_FDCAN_Driver
        HAL_FDCAN_Stop(((CANopenNodeSTM32*)CANmodule->CANptr)->CANHandle);

#else
        HAL_CAN_Stop(canport.hcan);
#endif
    }
}

/******************************************************************************/
CANtx_t*
CANtxBufferInit(CANtx_t* buffer, uint16_t ident, bool_t rtr, uint8_t noOfBytes,
                   bool_t syncFlag) {

    /* CAN identifier, DLC and rtr, bit aligned with CAN module transmit buffer */
    buffer->ident = ((uint32_t)ident & CANID_MASK) | ((uint32_t)(rtr ? FLAG_RTR : 0x00));
    buffer->DLC = noOfBytes;
    buffer->bufferFull = false;
    buffer->syncFlag = syncFlag;
    return buffer;
}

/******************************************************************************/
CO_ReturnError_t
CANrxBufferInit(CANrx_t* buffer, uint16_t ident, uint16_t mask, bool_t rtr, void* object,
                   void (*CANrx_callback)(void* object, void* message)) {
	CO_ReturnError_t ret = CO_ERROR_NO;

    if (canport.hcan != NULL && object != NULL && CANrx_callback != NULL) {

    	/* Configure object variables */
        buffer->object = object;
        buffer->CANrx_callback = CANrx_callback;

        /*
         * Configure global identifier, including RTR bit
         *
         * This is later used for RX operation match case
         */
        buffer->ident = (ident & CANID_MASK) | (rtr ? FLAG_RTR : 0x00);
        buffer->mask = (mask & CANID_MASK) | FLAG_RTR;

    } else {
        ret = CO_ERROR_ILLEGAL_ARGUMENT;
    }

    return ret;
}

/**
 * \brief           Send CAN message to network
 * This function must be called with atomic access.
 *
 * \param[in]       CANmodule: CAN module instance
 * \param[in]       buffer: Pointer to buffer to transmit
 */
static uint8_t
prv_send_can_message(CANtx_t* buffer) {

    uint8_t success = 0;

    /* Check if TX FIFO is ready to accept more messages */
#ifdef CO_STM32_FDCAN_Driver
    static FDCAN_TxHeaderTypeDef tx_hdr;
    if (HAL_FDCAN_GetTxFifoFreeLevel(((CANopenNodeSTM32*)CANmodule->CANptr)->CANHandle) > 0) {
        /*
         * RTR flag is part of identifier value
         * hence it needs to be properly decoded
         */
        tx_hdr.Identifier = buffer->ident & CANID_MASK;
        tx_hdr.TxFrameType = (buffer->ident & FLAG_RTR) ? FDCAN_REMOTE_FRAME : FDCAN_DATA_FRAME;
        tx_hdr.IdType = FDCAN_STANDARD_ID;
        tx_hdr.FDFormat = FDCAN_CLASSIC_CAN;
        tx_hdr.BitRateSwitch = FDCAN_BRS_OFF;
        tx_hdr.MessageMarker = 0;
        tx_hdr.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
        tx_hdr.TxEventFifoControl = FDCAN_NO_TX_EVENTS;

        switch (buffer->DLC) {
            case 0:
                tx_hdr.DataLength = FDCAN_DLC_BYTES_0;
                break;
            case 1:
                tx_hdr.DataLength = FDCAN_DLC_BYTES_1;
                break;
            case 2:
                tx_hdr.DataLength = FDCAN_DLC_BYTES_2;
                break;
            case 3:
                tx_hdr.DataLength = FDCAN_DLC_BYTES_3;
                break;
            case 4:
                tx_hdr.DataLength = FDCAN_DLC_BYTES_4;
                break;
            case 5:
                tx_hdr.DataLength = FDCAN_DLC_BYTES_5;
                break;
            case 6:
                tx_hdr.DataLength = FDCAN_DLC_BYTES_6;
                break;
            case 7:
                tx_hdr.DataLength = FDCAN_DLC_BYTES_7;
                break;
            case 8:
                tx_hdr.DataLength = FDCAN_DLC_BYTES_8;
                break;
            default: /* Hard error... */
                break;
        }

        /* Now add message to FIFO. Should not fail */
        success =
            HAL_FDCAN_AddMessageToTxFifoQ(((CANopenNodeSTM32*)CANmodule->CANptr)->CANHandle, &tx_hdr, buffer->data)
            == HAL_OK;
    }
#else
    static CAN_TxHeaderTypeDef tx_hdr;
    /* Check if TX FIFO is ready to accept more messages */
    if (HAL_CAN_GetTxMailboxesFreeLevel(canport.hcan) > 0) {
        /*
    		 * RTR flag is part of identifier value
    		 * hence it needs to be properly decoded
    		 */
        tx_hdr.ExtId = 0u;
        tx_hdr.IDE = CAN_ID_STD;
        tx_hdr.DLC = buffer->DLC;
        tx_hdr.StdId = buffer->ident & CANID_MASK;
        tx_hdr.RTR = (buffer->ident & FLAG_RTR) ? CAN_RTR_REMOTE : CAN_RTR_DATA;

        uint32_t TxMailboxNum; // Transmission MailBox number

        /* Now add message to FIFO. Should not fail */
        success = HAL_CAN_AddTxMessage(canport.hcan, &tx_hdr, buffer->data,
                                       &TxMailboxNum)
                  == HAL_OK;
    }
#endif
    return success;
}

/******************************************************************************/
CO_ReturnError_t
CANsend(CANtx_t* buffer) {
	CO_ReturnError_t err = CO_ERROR_NO;

    /* Verify overflow */
    if (buffer->bufferFull) {
        if (!canport.firstCANtxMessage) {
            /* don't set error, if bootup message is still on buffers */
            canport.CANerrorStatus |= CO_CAN_ERRTX_OVERFLOW;
        }
        err = CO_ERROR_TX_OVERFLOW;
    }

    /*
     * Send message to CAN network
     *
     * Lock interrupts for atomic operation
     */
    CO_LOCK_CAN_SEND(&canport);
    if (prv_send_can_message(buffer)) {
        canport.bufferInhibitFlag = buffer->syncFlag;
    } else {
        buffer->bufferFull = true;
        canport.CANtxCount++;
    }
    CO_UNLOCK_CAN_SEND(&canport);

    return err;
}

/******************************************************************************/
void
CANclearPendingSync() {
	uint32_t tpdoDeleted = 0U;
    CO_LOCK_CAN_SEND(&canport);
    /* Abort message from CAN module, if there is synchronous TPDO.
     * Take special care with this functionality. */
    if (/*messageIsOnCanBuffer && */ canport.bufferInhibitFlag) {
        /* clear TXREQ */
        canport.bufferInhibitFlag = false;
        tpdoDeleted = 1U;
    }
    /* delete also pending synchronous TPDOs in TX buffers */
    if (canport.CANtxCount > 0) {
        for (uint16_t i = canport.txSize; i > 0U; --i) {
            if (canport.txArray[i].bufferFull) {
                if (canport.txArray[i].syncFlag) {
                	canport.txArray[i].bufferFull = false;
                	canport.CANtxCount--;
                    tpdoDeleted = 2U;
                }
            }
        }
    }
    CO_UNLOCK_CAN_SEND(&canport);
    if (tpdoDeleted) {
        canport.CANerrorStatus |= CO_CAN_ERRTX_PDO_LATE;
    }
}

/******************************************************************************/
/* Get error counters from the module. If necessary, function may use
    * different way to determine errors. */

void
CANmodule_process() {
    uint32_t err = 0;

    // CANOpen just care about Bus_off, Warning, Passive and Overflow
    // I didn't find overflow error register in STM32, if you find it please let me know

#ifdef CO_STM32_FDCAN_Driver

    err = ((FDCAN_HandleTypeDef*)((CANopenNodeSTM32*)CANmodule->CANptr)->CANHandle)->Instance->PSR
          & (FDCAN_PSR_BO | FDCAN_PSR_EW | FDCAN_PSR_EP);

    if (CANmodule->errOld != err) {

        uint16_t status = CANmodule->CANerrorStatus;

        CANmodule->errOld = err;

        if (err & FDCAN_PSR_BO) {
            status |= CO_CAN_ERRTX_BUS_OFF;
            // In this driver we expect that the controller is automatically handling the protocol exceptions.

        } else {
            /* recalculate CANerrorStatus, first clear some flags */
            status &= 0xFFFF
                      ^ (CO_CAN_ERRTX_BUS_OFF | CO_CAN_ERRRX_WARNING | CO_CAN_ERRRX_PASSIVE | CO_CAN_ERRTX_WARNING
                         | CO_CAN_ERRTX_PASSIVE);

            if (err & FDCAN_PSR_EW) {
                status |= CO_CAN_ERRRX_WARNING | CO_CAN_ERRTX_WARNING;
            }

            if (err & FDCAN_PSR_EP) {
                status |= CO_CAN_ERRRX_PASSIVE | CO_CAN_ERRTX_PASSIVE;
            }
        }

        CANmodule->CANerrorStatus = status;
    }
#else

    err = (canport.hcan->Instance->ESR)
          & (CAN_ESR_BOFF | CAN_ESR_EPVF | CAN_ESR_EWGF);

    //    uint32_t esrVal = ((CAN_HandleTypeDef*)((CANopenNodeSTM32*)CANmodule->CANptr)->CANHandle)->Instance->ESR; Debug purpose
    if (canport.errOld != err) {

        uint16_t status = canport.CANerrorStatus;

        canport.errOld = err;

        if (err & CAN_ESR_BOFF) {
            status |= CO_CAN_ERRTX_BUS_OFF;
            // In this driver, we assume that auto bus recovery is activated ! so this error will eventually handled automatically.

        } else {
            /* recalculate CANerrorStatus, first clear some flags */
            status &= 0xFFFF
                      ^ (CO_CAN_ERRTX_BUS_OFF | CO_CAN_ERRRX_WARNING | CO_CAN_ERRRX_PASSIVE | CO_CAN_ERRTX_WARNING
                         | CO_CAN_ERRTX_PASSIVE);

            if (err & CAN_ESR_EWGF) {
                status |= CO_CAN_ERRRX_WARNING | CO_CAN_ERRTX_WARNING;
            }

            if (err & CAN_ESR_EPVF) {
                status |= CO_CAN_ERRRX_PASSIVE | CO_CAN_ERRTX_PASSIVE;
            }
        }

        canport.CANerrorStatus = status;
    }

#endif
}

#ifdef CO_STM32_FDCAN_Driver
static void
prv_read_can_received_msg(FDCAN_HandleTypeDef* hfdcan, uint32_t fifo, uint32_t fifo_isrs)
#else
static void
prv_read_can_received_msg(CAN_HandleTypeDef* hcan, uint32_t fifo, uint32_t fifo_isrs)
#endif
{
    CANrxMsg_t rcvMsg;

#ifdef CO_STM32_FDCAN_Driver
    static FDCAN_RxHeaderTypeDef rx_hdr;
    /* Read received message from FIFO */
    if (HAL_FDCAN_GetRxMessage(hfdcan, fifo, &rx_hdr, rcvMsg.data) != HAL_OK) {
        return;
    }
    /* Setup identifier (with RTR) and length */
    rcvMsg.ident = rx_hdr.Identifier | (rx_hdr.RxFrameType == FDCAN_REMOTE_FRAME ? FLAG_RTR : 0x00);
    switch (rx_hdr.DataLength) {
        case FDCAN_DLC_BYTES_0:
            rcvMsg.dlc = 0;
            break;
        case FDCAN_DLC_BYTES_1:
            rcvMsg.dlc = 1;
            break;
        case FDCAN_DLC_BYTES_2:
            rcvMsg.dlc = 2;
            break;
        case FDCAN_DLC_BYTES_3:
            rcvMsg.dlc = 3;
            break;
        case FDCAN_DLC_BYTES_4:
            rcvMsg.dlc = 4;
            break;
        case FDCAN_DLC_BYTES_5:
            rcvMsg.dlc = 5;
            break;
        case FDCAN_DLC_BYTES_6:
            rcvMsg.dlc = 6;
            break;
        case FDCAN_DLC_BYTES_7:
            rcvMsg.dlc = 7;
            break;
        case FDCAN_DLC_BYTES_8:
            rcvMsg.dlc = 8;
            break;
        default:
            rcvMsg.dlc = 0;
            break; /* Invalid length when more than 8 */
    }
    rcvMsgIdent = rcvMsg.ident;
#else
    static CAN_RxHeaderTypeDef rx_hdr;
    /* Read received message from FIFO */
    if (HAL_CAN_GetRxMessage(hcan, fifo, &rx_hdr, rcvMsg.data) != HAL_OK) {
        return;
    }

    /* Setup identifier (with RTR) and length */
    if (rx_hdr.StdId == nds_conf.hwset.ID){
        rcvMsg.ident = rx_hdr.StdId | (rx_hdr.RTR == CAN_RTR_REMOTE ? FLAG_RTR : 0x00);
        rcvMsg.dlc = rx_hdr.DLC;
        CO_fifo_write(&canport.rxBuf, (const char*)&rcvMsg, sizeof(CANrxMsg_t), NULL);
    }

    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
#endif

    /*
     * Hardware filters are not used for the moment
     * \todo: Implement hardware filters...
     */
	/*
	 * We are not using hardware filters, hence it is necessary
	 * to manually match received message ID with all buffers
	 */


}

#ifdef CO_STM32_FDCAN_Driver
/**
 * \brief           Rx FIFO 0 callback.
 * \param[in]       hfdcan: pointer to an FDCAN_HandleTypeDef structure that contains
 *                      the configuration information for the specified FDCAN.
 * \param[in]       RxFifo0ITs: indicates which Rx FIFO 0 interrupts are signaled.
 */
void
HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs) {
    if (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) {
        prv_read_can_received_msg(hfdcan, FDCAN_RX_FIFO0, RxFifo0ITs);
    }
}

/**
 * \brief           Rx FIFO 1 callback.
 * \param[in]       hfdcan: pointer to an FDCAN_HandleTypeDef structure that contains
 *                      the configuration information for the specified FDCAN.
 * \param[in]       RxFifo1ITs: indicates which Rx FIFO 0 interrupts are signaled.
 */
void
HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo1ITs) {
    if (RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) {
        prv_read_can_received_msg(hfdcan, FDCAN_RX_FIFO1, RxFifo1ITs);
    }
}

/**
 * \brief           TX buffer has been well transmitted callback
 * \param[in]       hfdcan: pointer to an FDCAN_HandleTypeDef structure that contains
 *                      the configuration information for the specified FDCAN.
 * \param[in]       BufferIndexes: Bits of successfully sent TX buffers
 */
void
HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef* hfdcan, uint32_t BufferIndexes) {
    CANModule_local->firstCANtxMessage = false;            /* First CAN message (bootup) was sent successfully */
    CANModule_local->bufferInhibitFlag = false;            /* Clear flag from previous message */
    if (CANModule_local->CANtxCount > 0U) {                /* Are there any new messages waiting to be send */
        CO_CANtx_t* buffer = &CANModule_local->txArray[0]; /* Start with first buffer handle */
        uint16_t i;

        /*
         * Try to send more buffers, process all empty ones
         *
         * This function is always called from interrupt,
         * however to make sure no preemption can happen, interrupts are anyway locked
         * (unless you can guarantee no higher priority interrupt will try to access to FDCAN instance and send data,
         *  then no need to lock interrupts..)
         */
        CO_LOCK_CAN_SEND(CANModule_local);
        for (i = CANModule_local->txSize; i > 0U; --i, ++buffer) {
            /* Try to send message */
            if (buffer->bufferFull) {
                if (prv_send_can_message(CANModule_local, buffer)) {
                    buffer->bufferFull = false;
                    CANModule_local->CANtxCount--;
                    CANModule_local->bufferInhibitFlag = buffer->syncFlag;
                }
            }
        }
        /* Clear counter if no more messages */
        if (i == 0U) {
            CANModule_local->CANtxCount = 0U;
        }
        CO_UNLOCK_CAN_SEND(CANModule_local);
    }
}
#else
/**
 * \brief           Rx FIFO 0 callback.
 * \param[in]       hcan: pointer to an CAN_HandleTypeDef structure that contains
 *                      the configuration information for the specified CAN.
 */
void
HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan) {
    if (canport.isopen)
    	prv_read_can_received_msg(hcan, CAN_RX_FIFO0, 0);
}

/**
 * \brief           Rx FIFO 1 callback.
 * \param[in]       hcan: pointer to an CAN_HandleTypeDef structure that contains
 *                      the configuration information for the specified CAN.
 */
void
HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef* hcan) {
	if (canport.isopen)
		prv_read_can_received_msg(hcan, CAN_RX_FIFO1, 0);
}

/**
 * \brief           TX buffer has been well transmitted callback
 * \param[in]       hcan: pointer to an CAN_HandleTypeDef structure that contains
 *                      the configuration information for the specified CAN.
 * \param[in]       MailboxNumber: the mailbox number that has been transmitted
 */
void
CANinterrupt_TX(CAN_HandleTypeDef* hcan, uint32_t MailboxNumber) {

    canport.firstCANtxMessage = false;            /* First CAN message (bootup) was sent successfully */
    canport.bufferInhibitFlag = false;            /* Clear flag from previous message */
    if (canport.CANtxCount > 0U) {                /* Are there any new messages waiting to be send */
        CANtx_t* buffer = &(canport.txArray[0]); /* Start with first buffer handle */
        uint16_t i;

        /*
		 * Try to send more buffers, process all empty ones
		 *
		 * This function is always called from interrupt,
		 * however to make sure no preemption can happen, interrupts are anyway locked
		 * (unless you can guarantee no higher priority interrupt will try to access to CAN instance and send data,
		 *  then no need to lock interrupts..)
		 */
        CO_LOCK_CAN_SEND(&canport);
        for (i = canport.txSize; i > 0U; --i, ++buffer) {
            /* Try to send message */
            if (buffer->bufferFull) {
                if (prv_send_can_message(buffer)) {
                    buffer->bufferFull = false;
                    canport.CANtxCount--;
                    canport.bufferInhibitFlag = buffer->syncFlag;
                }
            }
        }

        /* Clear counter if no more messages */
        if (i == 0U) {
            canport.CANtxCount = 0U;
        }
        CO_UNLOCK_CAN_SEND(&canport);
    }
}

void
HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef* hcan) {
    CANinterrupt_TX(hcan, CAN_TX_MAILBOX0);
}

void
HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef* hcan) {
    CANinterrupt_TX(hcan, CAN_TX_MAILBOX0);
}

void
HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef* hcan) {
    CANinterrupt_TX(hcan, CAN_TX_MAILBOX0);
}
#endif

uint8_t CANreadMsg(CANrxMsg_t* buffer)
{
	uint8_t ret = 0;

	if (CO_fifo_getOccupied(&canport.rxBuf) >= sizeof(CANrxMsg_t)){
		size_t len = CO_fifo_read(&canport.rxBuf, (char *)buffer, sizeof(CANrxMsg_t), NULL);
		if (len >= sizeof(CANrxMsg_t)){
			ret = 1;
		}
	}

	return ret;
}
