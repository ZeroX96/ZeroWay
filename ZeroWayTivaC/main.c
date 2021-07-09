

/**
 * main.c
 */
#include "TM4C123GH6PM.h"

int main(void)
{
    SYSCTL->RCGCGPIO = 0x96;
	return 0;
}
