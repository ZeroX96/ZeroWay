/*
 * AppGateWay.c
 *
 * Created: 7/6/2021 7:44:23 PM
 *  Author: ZeroX
 */ 
#include "AppGateWay.h"
typedef enum{
    CHANNEL_EMPTY     = (0x98U),
    CHANNEL_NOT_EMPTY = (0x70U),
}ChannelStateT;
//here in the atmega32 there are three communication channels,
//    so edit according to your system and point to the currently operating exchange request
//#warning "edit this part of the code, on this mcu, i have only three channels"
static GateWayT SpiDataHandler;   // to handle the current data exchange operation for the Spi channel
static GateWayT TwiDataHandler;   // to handle the current data exchange operation for the Twi channel
static GateWayT UsartDataHandler; // to handle the current data exchange operation for the Usart channel

//internals to handle the hardware
static UsartT UsartChannelHandler;
static SpiT   SpiChannelHandler;
static TwiT   TwiChannelHandler;

ChannelStateT UsartChannelState= CHANNEL_EMPTY;
ChannelStateT SpiChannelState  = CHANNEL_EMPTY;
ChannelStateT TwiChannelState  = CHANNEL_EMPTY;
//internals to handle the number of bytes sent by each channel
static volatile uint32_t UsartSentByteNumber = 0;
static volatile uint32_t SpiSentByteNumber   = 0;
static volatile uint32_t TwiSentByteNumber   = 0;

//flags used in the sending operations through the different channels
static UsartT UsartSentByteFlag = USART_FLAG_LOW;
static SpiT   SpiSentByteFlag   = SPI_FLAG_LOW;
static TwiT   TwiSentByteFlag   = TWI_FLAG_LOW;

//internal queues handle the requests
static GatewayQueueT  UsartRequestsQueue;
static GatewayQueueT  SpiRequestsQueue;
static GatewayQueueT  TwiRequestsQueue;

//internals to handle callbacks from the interrupts
static void UsartCallbackHandler(void);
static void SpiCallbackHandler(void);
static void TwiCallbackHandler(void);


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
    GatewayQueueCreate(&UsartRequestsQueue);
    GatewayQueueCreate(&SpiRequestsQueue);
    GatewayQueueCreate(&TwiRequestsQueue);
    
}

GateWayRetT GateWaySend(GateWayT* GateWayElement){
    if (GateWayElement == NULL){
        return GATEWAY_ERR_INV_PARAMS;
    }
        //temporary variable to get the state of queue operations
        //GatewayQueueRetT TempReturnVal = QLB_NO_ERRORS;
    
    ////check the channel
    //if auto, it will be added to the queue with the least pending requests
    if (GateWayElement->ObjChannel == CHANNEL_AUTO){
        //get the current sizes of the queues
        uint8_t UsartSize = GatewayQueueSize(&UsartRequestsQueue);
        uint8_t SpiSize = GatewayQueueSize(&SpiRequestsQueue);
        uint8_t TwiSize = GatewayQueueSize(&TwiRequestsQueue);
        //check the channels queues to see if one is full to exclude it and use another queue
        if (QLB_NOT_FULL == GatewayQueueFull(&UsartRequestsQueue)){
            UsartSize = MAX_POSIBBLE_VALUE;
        }else if (QLB_NOT_FULL == GatewayQueueFull(&SpiRequestsQueue)){
            SpiSize = MAX_POSIBBLE_VALUE;
        }else if (QLB_NOT_FULL == GatewayQueueFull(&TwiRequestsQueue)){
            TwiSize = MAX_POSIBBLE_VALUE;
        }else{
            //do-nothing
        }
        
        //compare the queues sizes and select the one with smallest number
        if (( UsartSize <= SpiSize) && (UsartSize <= TwiSize)){
            //if the usart queue have the smallest number of requests, add this request to it
            //but first, check the data priority if it's high or normal
            if (GateWayElement->ObjDataPriority == DATA_PRIO_HIGH){
                GatewayQueueAppendFront(&UsartRequestsQueue, GateWayElement);
            }else{ 
            // if it's normal or even if the user sent an invalid priority number,
            //  it will be treated as a normal request
                GatewayQueueAppend(&UsartRequestsQueue, GateWayElement);
            }
        }        
        else if (( SpiSize <= UsartSize) && (SpiSize <= TwiSize)){
            //if the spi queue have the smallest number of requests, add this request to it
            //but first, check the data priority if it's high or normal
            if (GateWayElement->ObjDataPriority == DATA_PRIO_HIGH){
                GatewayQueueAppendFront(&SpiRequestsQueue, GateWayElement);
                }else{
                // if it's normal or even if the user sent an invalid priority number,
                //  it will be treated as a normal request
                GatewayQueueAppend(&SpiRequestsQueue, GateWayElement);
            }
        }        
        else{ //(( TwiSize <= SpiSize) && (TwiSize <= UsartSize)){
            //if the twi queue have the smallest number of requests, add this request to it
            //but first, check the data priority if it's high or normal
            if (GateWayElement->ObjDataPriority == DATA_PRIO_HIGH){
                GatewayQueueAppendFront(&TwiRequestsQueue, GateWayElement);
            }
            else{
                // if it's normal or even if the user sent an invalid priority number,
                //  it will be treated as a normal request
                GatewayQueueAppend(&TwiRequestsQueue, GateWayElement);
            }
            
        }
    }
    else if(GateWayElement->ObjChannel == CHANNEL_SPI){
        //check if there is a space in the queue
        //if not, i would tell the user that I'm busy
        //note, here i used the QLB_NOT_FULL to check if it's not full for both
        //statically and dynamically allocated queues but it's the same value, dont worry
        if(QLB_NOT_FULL != GatewayQueueFull(&SpiRequestsQueue)){
            //but first, check the data priority if it's high or normal
            if (GateWayElement->ObjDataPriority == DATA_PRIO_HIGH){
                GatewayQueueAppendFront(&SpiRequestsQueue, GateWayElement);
            }
            else{
                // if it's normal or even if the user sent an invalid priority number,
                //  it will be treated as a normal request
                GatewayQueueAppend(&SpiRequestsQueue, GateWayElement);
            }
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
        if(QLB_NOT_FULL != GatewayQueueFull(&TwiRequestsQueue)){
            //but first, check the data priority if it's high or normal
            if (GateWayElement->ObjDataPriority == DATA_PRIO_HIGH){
                GatewayQueueAppendFront(&TwiRequestsQueue, GateWayElement);
            }
            else{
                // if it's normal or even if the user sent an invalid priority number,
                //  it will be treated as a normal request
                GatewayQueueAppend(&TwiRequestsQueue, GateWayElement);
            }
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
        if(QLB_NOT_FULL != GatewayQueueFull(UsartRequestsQueue)){
            //first, check the data priority if it's high or normal
            if (GateWayElement->ObjDataPriority == DATA_PRIO_HIGH){
                GatewayQueueAppendFront(&UsartRequestsQueue, GateWayElement);
            }
            else{
                // if it's normal or even if the user sent an invalid priority number,
                //  it will be treated as a normal request
                GatewayQueueAppend(&UsartRequestsQueue, GateWayElement);
            }
        }
        else{
            return GATEWAY_ERR_BUSY;
        }
    }
    else{
        return GATEWAY_ERR_INV_PARAMS;
    }
    //now see if there is a free channel that can handle one of the available requests
    if (UsartChannelState == CHANNEL_EMPTY){
        //if the usart queue is not empty, retrieve an element to be served next
        if( GatewayQueueEmpty(&UsartRequestsQueue) != QLB_NOT_EMPTY){
            GatewayQueueServe(&UsartRequestsQueue, &UsartDataHandler);
            UsartChannelState = CHANNEL_NOT_EMPTY;
        }  
    } 
    if (SpiChannelState == CHANNEL_EMPTY){
        //if the usart queue is not empty, retrieve an element to be served next
        if( GatewayQueueEmpty(&SpiRequestsQueue) != QLB_NOT_EMPTY){
            GatewayQueueServe(&SpiRequestsQueue, &SpiDataHandler);
            SpiChannelState = CHANNEL_NOT_EMPTY;
        }
    }
    if (TwiChannelState == CHANNEL_EMPTY){
        //if the usart queue is not empty, retrieve an element to be served next
        if( GatewayQueueEmpty(&TwiRequestsQueue) != QLB_NOT_EMPTY){
            GatewayQueueServe(&TwiRequestsQueue, &TwiDataHandler);
            TwiChannelState = CHANNEL_NOT_EMPTY;
        }
    }
    
    return GATEWAY_ERR_NO;
}

GateWayRetT GateWayDispatch(void){
	static uint8_t TemporaryUsartExchange = 0;
	static uint8_t TemporarySpiExchange = 0;
	static uint8_t TemporaryTwiExchange = 0;
    //check if the byte of every channel is sent or not yet
    //check if there is a pending/running send operation
    //if no operation to be done through any of the three channels, return
    if( (UsartChannelState == CHANNEL_EMPTY) && (GatewayQueueEmpty(&UsartRequestsQueue) == QLB_EMPTY) &&
        ( SpiChannelState == CHANNEL_EMPTY ) && (GatewayQueueEmpty(&SpiRequestsQueue) == QLB_EMPTY  ) &&
        ( TwiChannelState == CHANNEL_EMPTY ) && (GatewayQueueEmpty(&TwiRequestsQueue) == QLB_EMPTY  )
      ){
        return GATEWAY_ERR_FREE;
    }
    if (UsartDataHandler.ObjDataBytesNumb <= UsartSentByteNumber){
        UsartSentByteNumber = 0;
        UsartChannelState = CHANNEL_EMPTY;
    }
    if (TwiDataHandler.ObjDataBytesNumb <= TwiSentByteNumber){
        TwiSentByteNumber = 0;
        TwiChannelState = CHANNEL_EMPTY;
    }
    if (SpiDataHandler.ObjDataBytesNumb <= SpiSentByteNumber){
        SpiSentByteNumber = 0;
        SpiChannelState = CHANNEL_EMPTY;
    }
    /*check if there is any task for the three channels*/
    //if the Usart channel handler is free and there is a task in the Usart queue, bring it on!
    if((UsartChannelState == CHANNEL_EMPTY) && (GatewayQueueEmpty(&UsartRequestsQueue) == QLB_NOT_EMPTY)){
        //get a task from the queue to the handler
        GatewayQueueServe(&UsartRequestsQueue, &UsartDataHandler);
        UsartChannelState = CHANNEL_NOT_EMPTY;
        if (TwiDataHandler.ObjCfgEdit == CFG_EDIT_TRUE){
	        UsartInit(&UsartChannelHandler,
	        *((UsartT*)SpiDataHandler.ObjChannelCfg).objOpMode,
	        *((UsartT*)SpiDataHandler.ObjChannelCfg).ObjStopBitsNo,
	        *((UsartT*)SpiDataHandler.ObjChannelCfg).objParityMode,
	        *((UsartT*)SpiDataHandler.ObjChannelCfg).ObjDataBitsNo,
	        *((UsartT*)SpiDataHandler.ObjChannelCfg).ObjNotificsMode,
	        *((UsartT*)SpiDataHandler.ObjChannelCfg).ClkPolarity);
        }
    }
    //if the Twi channel handler is free and there is a task in the Twi queue, bring it on!
    if((TwiChannelState == CHANNEL_EMPTY) && (GatewayQueueEmpty(&TwiRequestsQueue) == QLB_NOT_EMPTY)){
        //get a task from the queue to the handler
        GatewayQueueServe(&TwiRequestsQueue, &TwiDataHandler);
        TwiChannelState = CHANNEL_NOT_EMPTY;
        if (TwiDataHandler.ObjCfgEdit == CFG_EDIT_TRUE){
			TwiInit((TwiT*)TwiDataHandler.ObjChannelCfg)
		}
    }
    //if the Spi channel handler is free and there is a task in the Spi queue, bring it on!
    if((SpiChannelState == CHANNEL_EMPTY) && (GatewayQueueEmpty(&SpiRequestsQueue) == QLB_NOT_EMPTY)){
        //get a task from the queue to the handler
        GatewayQueueServe(&SpiRequestsQueue, &SpiDataHandler);
        SpiChannelState = CHANNEL_NOT_EMPTY;
		if (SpiDataHandler.ObjCfgEdit == CFG_EDIT_TRUE){
		SpiInit(&SpiChannelHandler,
		        *((SpiT*)SpiDataHandler.ObjChannelCfg).SpiBase,
			    *((SpiT*)SpiDataHandler.ObjChannelCfg).SpiFreq,
				*((SpiT*)SpiDataHandler.ObjChannelCfg).SpiOpMode,
				*((SpiT*)SpiDataHandler.ObjChannelCfg).SpiNotificsMode,
				*((SpiT*)SpiDataHandler.ObjChannelCfg).SpiTransferMode,
				*((SpiT*)SpiDataHandler.ObjChannelCfg).SpiDataOrder);
		}
    }
	//Now process an Exchange operation if available
	if( (UsartChannelState == CHANNEL_NOT_EMPTY) && (UsartSentByteFlag == SPI_FLAG_HIGH) ){
		//usart send
		UsartSendByte(&UsartChannelHandler, (uint8_t*)(UsartDataHandler.ObjDataPtr + UsartSentByteNumber));
		UsartSentByteFlag = USART_FLAG_LOW;
    }
    if( (SpiChannelState == CHANNEL_NOT_EMPTY) && (SpiSentByteFlag == SPI_FLAG_HIGH) ){
		//spi send
	    SpiExchangeDATA(&SpiChannelHandler,(uint8_t*)(SpiDataHandler.ObjDataPtr + SpiSentByteNumber), &TemporarySpiExchange );
	    SpiSentByteFlag = SPI_FLAG_LOW;
    }
    if( (TwiChannelState == CHANNEL_NOT_EMPTY) && (TwiSentByteFlag == SPI_FLAG_HIGH) ){
		//twi send
		TwiSendDataByte(&TwiChannelHandler, (uint8_t*)(TwiDataHandler.ObjDataPtr + TwiSentByteNumber));
	    TwiSentByteFlag = TWI_FLAG_LOW;
    }
    return GATEWAY_ERR_NO;
}

static void UsartCallbackHandler(void){
    //when the interrupt happens, raise the flag
    UsartSentByteFlag = USART_FLAG_HIGH;
    UsartSentByteNumber++;
}   
static void SpiCallbackHandler(void){
    //when the interrupt happens, raise the flag
    SpiSentByteFlag = SPI_FLAG_HIGH;   
    SpiSentByteNumber++;
}
static void TwiCallbackHandler(void){
    //when the interrupt happens, raise the flag
    TwiSentByteFlag = TWI_FLAG_HIGH;
    TwiSentByteNumber++;  
}