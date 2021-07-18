/*
 * AppGateWayDataTypes.h
 *
 * Created: 7/7/2021 10:42:49 PM
 *  Author: ZeroX
 */ 


#ifndef APPGATEWAYDATATYPES_H_
#define APPGATEWAYDATATYPES_H_
#include <stdint.h>
#include <stdbool.h>

#define USART_FLAG_HIGH     (0x96U)
#define USART_FLAG_LOW      (0x69U)
#define SPI_FLAG_HIGH       (0x08U)
#define SPI_FLAG_LOW        (0x80U)
#define TWI_FLAG_HIGH       (0x20U)
#define TWI_FLAG_LOW        (0x02U)
/*******************************************************************************/
/* GATEWAY_ERR_NO == No errors occured while executing the current task        */
/* GATEWAY_ERR_INV_PARAMS = Gateway function X is called with wrong parameters */
/* GATEWAY_ERR_BUSY = gateway queues are full & Can't handle any more data     */
/* GATEWAY_ERR_FREE = the gateway is called and there is no data to process    */
/*******************************************************************************/
#define GATEWAY_ERROR_BASE (0x70U)

typedef enum{
    GATEWAY_ERR_NO = GATEWAY_ERROR_BASE,
    GATEWAY_ERR_INV_PARAMS,
    GATEWAY_ERR_BUSY,
    GATEWAY_ERR_FREE,   
}GateWayRetT;
/************************************************************************/
/* Defines the channel through which the data will be sent              */
/* If it's defined as an auto, the system will assign it to the channel */
/* that has the smallest number of requests in the system.              */
/************************************************************************/
typedef enum{
    CHANNEL_AUTO,
    CHANNEL_USART,
    CHANNEL_TWI,
    CHANNEL_SPI,
}GateWayChannelT;
/************************************************************************/
/* the endianess that the data to be sent is of i.e. big/little-endian  */
/************************************************************************/
typedef enum{
    DATA_LITTLE_ENDIAN,
    DATA_BIG_ENDIAN
}GateWayEndianessT;
/************************************************************************/
/* the level of the data priority,                                      */
/* if it's high, it will be handled just in the next request handling   */
/* if it's normal, it will be handled as normal and just added to the q */
/************************************************************************/
typedef enum{
    DATA_PRIO_NORM,
    DATA_PRIO_HIGH
}GateWayDataPrioT;
/************************************************************************/
/* define whither the channel configuration needs to be edited or not   */
/************************************************************************/
typedef enum{
    CFG_EDIT_FALSE,
    CFG_EDIT_TRUE,
}GateWayCfgEditT;
/************************************************************************/
/*                                                                      */
/************************************************************************/
typedef struct{
    GateWayDataPrioT  ObjDataPriority;  // the data priority whither it's normal or very important
    const void*       ObjDataPtr;       // void pointer to handle any type of data
    GateWayChannelT   ObjChannel;       // which channel to use
    GateWayEndianessT ObjDataEndianess; // the endianess that the data to be sent is of i.e. big/little-endian
    uint16_t          ObjDataBytesNumb; // the number of data bytes to be sent through one of the channels
    const void*       ObjChannelCfg;    // Void Pointer to one of the three types, TwiT, SpiT, UsartT to config the channel
    GateWayCfgEditT   ObjCfgEdit;       //used to check if the channel configuration needs to be edited or not
}GateWayT;

typedef GateWayT QueueEntry;//TODO: visible here or not? //Done
#endif /* APPGATEWAYDATATYPES_H_ */