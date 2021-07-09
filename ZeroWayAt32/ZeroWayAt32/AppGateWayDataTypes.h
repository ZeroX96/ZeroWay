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
#define GATEWAY_ERROR_BASE (0x70U)

typedef enum{
    GATEWAY_ERR_NO = GATEWAY_ERROR_BASE,
    GATEWAY_ERR_INV_PARAMS,
    GATEWAY_ERR_BUSY
}GateWayRetT;

typedef enum{
    CHANNEL_AUTO,
    CHANNEL_USART,
    CHANNEL_TWI,
    CHANNEL_SPI,
}GateWayChannelT;

typedef enum{
    DATA_LITTLE_ENDIAN,
    DATA_BIG_ENDIAN
}GateWayEndianessT;
/*
* the level of the data priority, 
* if it's high, it will be handled just in the next request handling
*/
typedef enum{
    DATA_PRIO_NORM,
    DATA_PRIO_HIGH
}GateWayDataPrioT;

typedef enum{
    TARGET_0X00 = 0x00,
}TargetIdT;

typedef struct{
    GateWayDataPrioT  ObjDataPriority;
    const void*       ObjDataPtr; // void pointer to handle any type of data
    GateWayChannelT   ObjChannel;
    GateWayEndianessT ObjDataEndianess;
    uint16_t          ObjDataBytesNumb;
    TargetIdT         ObjTargetId;
}GateWayT;

typedef GateWayT QueueEntry;//TODO: visible here or not? //Done
#endif /* APPGATEWAYDATATYPES_H_ */