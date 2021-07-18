/*
 * main.c
 *
 * Created: 7/8/2021 1:17:38 AM
 *  Author: ZeroX
 */ 
#include "AppGateWay.h"
//MY_DEFAULT_CHANNEL_BITRATE
int main(void){
    GateWayInit();
    while(1){
        /*YOuR ApP HErE*/
        /*YoU CaN CAlL GATEWaYSend As NEEdED*/
        GateWaySend();
        GateWayDispatch();
    }
    return 0;
}//2150