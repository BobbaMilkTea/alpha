/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "stdio.h"
#include "display.h"

int main(void)
{
	display_init();
	display_msg("I am Milky ;-;", 0);
	display_msg("and I <3 ML", 1);
	for(;;);
}
