/*
 * GateWayQueue.h
 *
 * Created: 7/7/2021 6:31:03 AM
 *  Author: ZeroX
 */ 


#ifndef GATEWAYQUEUE_H_
#define GATEWAYQUEUE_H_
#include "AppGateWayCfg.h"
#include "AppGateWayDataTypes.h"

#if (USE_STATIC_ALLOCATION == TRUE)
    #include "Queue_ArrBased.h"
    typedef Queue_ArrBased GatewayQueueT;
    typedef QAB_return_t   GatewayQueueRetT;
//// END OF USE_STATIC_ALLOCATION
#elif (USE_DYNAMIC_ALLOCATION == TRUE)
    #include "Queue_LinkBased.h"
    typedef Queue_LinkBased GatewayQueueT;
    typedef QLB_return_t    GatewayQueueRetT;
//// END OF USE_DYNAMIC_ALLOCATION
#endif //USE_DYNAMIC_ALLOCATION or USE_STATIC_ALLOCATION


#ifndef MAX_QUEUE_LENGTH
#define MAX_QUEUE_LENGTH GATEWAY_QUEUE_SIZE //NOT IMPORTANT IN THE LINKED BASED
#endif //MAX_QUEUE_LENGTH


GatewayQueueRetT GatewayQueueCreate      (GatewayQueueT* ptr_queue);
GatewayQueueRetT GatewayQueueEmpty       (GatewayQueueT* ptr_queue);
GatewayQueueRetT GatewayQueueFull        (GatewayQueueT* ptr_queue);
uint32_t         GatewayQueueSize        (GatewayQueueT* ptr_queue);
GatewayQueueRetT GatewayQueueAppend      (GatewayQueueT* ptr_queue,QueueEntry* ptr_entry);
GatewayQueueRetT GatewayQueueServe       (GatewayQueueT* ptr_queue,QueueEntry* ptr_entry);
GatewayQueueRetT GatewayQueueRetrive     (GatewayQueueT* ptr_queue,QueueEntry* ptr_entry);
GatewayQueueRetT GatewayQueueClear       (GatewayQueueT* ptr_queue);
GatewayQueueRetT GatewayQueueTraverse    (GatewayQueueT* ptr_queue,void(*ptr_func)(QueueEntry));
GatewayQueueRetT GatewayQueueAppendFront (GatewayQueueT* ptr_queue,QueueEntry* ptr_entry);

#endif /* GATEWAYQUEUE_H_ */