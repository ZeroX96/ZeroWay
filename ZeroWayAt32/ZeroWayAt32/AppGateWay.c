/*
 * AppGateWay.c
 *
 * Created: 7/6/2021 7:44:23 PM
 *  Author: ZeroX
 */ 
#include "AppGateWay.h"

// #if (USE_STATIC_ALLOCATION == TRUE)
// static GateWayT* GatewayQueue[GATEWAY_QUEUE_SIZE] = {NULL};
// #endif //USE_STATIC_ALLOCATION
// 
// #if (USE_DYNAMIC_ALLOCATION == TRUE)
// static GateWayT* GatewayQueue = calloc(GATEWAY_QUEUE_SIZE, sizeof(GateWayT*));
// #endif //USE_STATIC_ALLOCATION

//here in the atmega32 there are three communication channels,
//    so edit according to your system and point to the currently operating exchange request
#warning "edit this part of the code, on this mcu, i have only three channels"
static GateWayT SpiDataHandler;   // to handle the current data exchange operation for the spi channel
static GateWayT TwiDataHandler;   // to handle the current data exchange operation for the twi channel
static GateWayT UsartDataHandler; // to handle the current data exchange operation for the usart channel

//internals to handle the hardware
static UsartT UsartChannelHandler;
static SpiT   SpiChannelHandler;
static TwiT   TwiChannelHandler;

//internals to handle callbacks from the interrupts
static void UsartCallbackHandler(void);
static void SpiCallbackHandler(void);
static void TwiCallbackHandler(void);

//internal queues handle the requests
static GatewayQueueT  UsartRequestsHandler;
static GatewayQueueT  AutoRequestsHandler;
static GatewayQueueT  SpiRequestsHandler;
static GatewayQueueT  TwiRequestsHandler;


//called before any other function 
GateWayRetT GateWayInit(void){
    /*--initialize the communication channels--*/
    
    //initialize the USART
    UsartInit(&UsartChannelHandler, A_SENDER, ONE_STP_BIT, EVEN_PARITY, 
               EIGHT_DATA_BITS, INTERRUPT_DRIVEN, Rising_EDGE);
    //define the usart callback
    UsartSetIsrTxcCb(&UsartChannelHandler, UsartCallbackHandler);
    //initialize the SPI
    SpiInit(&SpiChannelHandler, SPI_1_base, FREQ_BY_4, MASTER_EN, 
              SPI_INTERRUPTING, MODE_2, MSB_FIRST);
    //define the spi callback
    SpiSetIsrCallback(&SpiChannelHandler,SpiCallbackHandler);
    //initialize the TWI
    TwiChannelHandler.TwiBase = TWI_0_BASE;
    TwiChannelHandler.TwiBitRate = 0x50;
    TwiChannelHandler.TwiCallBkFunc = TwiCallbackHandler;
    TwiChannelHandler.TwiGenCallRecognition = TWI_GEN_CALL_RECOGNIZED;
    TwiChannelHandler.TwiMySlaveAdd = 0x35;
    TwiChannelHandler.TwiPrescaller = TWI_PRESCALLER_4;
    TwiChannelHandler.TwiTargetedSlaveAdd = 0x20; //Todo: Let the user define it
    TwiChannelHandler.TwiTWAR_Offset = TWAR_0_OFFSET;
    TwiChannelHandler.TwiTWBR_Offset = TWBR_0_OFFSET;
    TwiChannelHandler.TwiTWCR_Offset = TWCR_0_OFFSET;
    TwiChannelHandler.TwiTWDR_Offset = TWDR_0_OFFSET,
    TwiChannelHandler.TwiTWSR_Offset = TWSR_0_OFFSET;
    TwiInit(&TwiChannelHandler);
    
    /*--initialize the Internal Queues to handle the requests--*/
    GatewayQueueCreate(&UsartRequestsHandler);
    GatewayQueueCreate(&SpiRequestsHandler);
    GatewayQueueCreate(&TwiRequestsHandler);
    
}

GateWayRetT GateWaySend(GateWayT* GateWayElement){
    if (GateWayElement == NULL){
        return GATEWAY_ERR_INV_PARAMS;
    }
    //temporary variable to get the state of queue operations
    GatewayQueueRetT TempReturnVal = QLB_NO_ERRORS;
    
    ////check the channel
    //if auto, it will be added to the queue with the least pending requests
    if (GateWayElement->ObjChannel == CHANNEL_AUTO){
        //get the current sizes of the queues
        uint8_t UsartSize = GatewayQueueSize(&UsartRequestsHandler);
        uint8_t SpiSize = GatewayQueueSize(&SpiRequestsHandler);
        uint8_t TwiSize = GatewayQueueSize(&TwiRequestsHandler);
        //check the channels queues to see if one is full to exclude it and use another queue
        if (QLB_NOT_FULL == GatewayQueueFull(&UsartRequestsHandler)){
            UsartSize = MAX_POSIBBLE_VALUE;
        }else if (QLB_NOT_FULL == GatewayQueueFull(&SpiRequestsHandler)){
            SpiSize = MAX_POSIBBLE_VALUE;
        }else if (QLB_NOT_FULL == GatewayQueueFull(&TwiRequestsHandler)){
            TwiSize = MAX_POSIBBLE_VALUE;
        }        
        
        //compare the queues sizes and select the one with smallest number
        if (( UsartSize <= SpiSize) && (UsartSize <= TwiSize)){
            //if the usart queue have the smallest number of requests, add this request to it
            //but first, check the data priority if it's high or normal
            if (GateWayElement->ObjDataPriority == DATA_PRIO_HIGH){
                GatewayQueueAppendFront(&UsartRequestsHandler, GateWayElement);
            }else{ 
            // if it's normal or even if the user sent an invalid priority number,
            //  it will be treated as a normal request
                GatewayQueueAppend(&UsartRequestsHandler, GateWayElement);
            }
        }        
        else if (( SpiSize <= UsartSize) && (SpiSize <= TwiSize)){
            //if the spi queue have the smallest number of requests, add this request to it
            //but first, check the data priority if it's high or normal
            if (GateWayElement->ObjDataPriority == DATA_PRIO_HIGH){
                GatewayQueueAppendFront(&SpiRequestsHandler, GateWayElement);
                }else{
                // if it's normal or even if the user sent an invalid priority number,
                //  it will be treated as a normal request
                GatewayQueueAppend(&SpiRequestsHandler, GateWayElement);
            }
        }        
        else{ //(( TwiSize <= SpiSize) && (TwiSize <= UsartSize)){
            //if the usart queue have the smallest number of requests, add this request to it
            //but first, check the data priority if it's high or normal
            if (GateWayElement->ObjDataPriority == DATA_PRIO_HIGH){
                GatewayQueueAppendFront(&TwiRequestsHandler, GateWayElement);
                }else{
                // if it's normal or even if the user sent an invalid priority number,
                //  it will be treated as a normal request
                GatewayQueueAppend(&TwiRequestsHandler, GateWayElement);
            }
            
        }
    }
    else if(GateWayElement->ObjChannel == CHANNEL_SPI){
        //check if there is a space in the queue
        //if not, i would tell the user that I'm busy
        //note, here i used the QLB_NOT_FULL to check if it's not full for both
        //statically and dynamically allocated queues but it's the same value, dont worry
        if(QLB_NOT_FULL != GatewayQueueFull(SpiRequestsHandler)){
            GatewayQueueAppend(&SpiRequestsHandler, GateWayElement);
        }
        else{
            return GATEWAY_ERR_BUSY;
        }            
    }
    else if(GateWayElement->ObjChannel == CHANNEL_TWI){
        //check if there is a space in the queue
        //if not, i would tell the user that I'm busy
        //note, here i used the QLB_NOT_FULL to check if it's not full for both
        //statically and dynamically allocated queues but it's the same value, dont worry
        if(QLB_NOT_FULL != GatewayQueueFull(TwiRequestsHandler)){
        //but first, check the data priority if it's high or normal
            if (GateWayElement->ObjDataPriority == DATA_PRIO_HIGH){
                GatewayQueueAppendFront(&TwiRequestsHandler, GateWayElement);
                }else{
                // if it's normal or even if the user sent an invalid priority number,
                //  it will be treated as a normal request
                GatewayQueueAppend(&TwiRequestsHandler, GateWayElement);
            }
            GatewayQueueAppend(&TwiRequestsHandler, GateWayElement);
        }
        else{
            return GATEWAY_ERR_BUSY;
        }
    }
    else if(GateWayElement->ObjChannel == CHANNEL_USART){
        //check if there is a space in the queue
        //if not, i would tell the user that I'm busy
        //note, here i used the QLB_NOT_FULL to check if it's not full for both
        //statically and dynamically allocated queues but it's the same value, dont worry
        if(QLB_NOT_FULL != GatewayQueueFull(UsartRequestsHandler)){
        //first, check the data priority if it's high or normal
            if (GateWayElement->ObjDataPriority == DATA_PRIO_HIGH){
                GatewayQueueAppendFront(&UsartRequestsHandler, GateWayElement);
                }else{
                // if it's normal or even if the user sent an invalid priority number,
                //  it will be treated as a normal request
                GatewayQueueAppend(&UsartRequestsHandler, GateWayElement);
            }
        }
        else{
            return GATEWAY_ERR_BUSY;
        }
    }
    else{
        return GATEWAY_ERR_INV_PARAMS;
    }
    //now see if there is a free channel that can handle the available requests
    
    return GATEWAY_ERR_NO;
}

GateWayRetT GateWayDisp(void){
    return GATEWAY_ERR_NO;
}