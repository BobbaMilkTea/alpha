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
#include "string.h"
#include "include/display.h"
#include "include/hygrometer.h"

void disp_temperature(short temperature) {
	char buff[16] = "Temperature: ";
	char tempBuff[4] = {'\0'};
	sprintf(tempBuff, "%d", temperature);
	strcat(buff, tempBuff);
	display_msg(buff, 0);
}

void disp_humidity(short humidity) {
	char buff[16] = "Humidity: ";
	char humiBuff[4] = {'\0'};
	sprintf(humiBuff, "%d", humidity);
	strcat(buff, humiBuff);
	display_msg(buff, 1);
}

int main(void)
{
	short humidity = 0;
	short temperature = 0;
	display_init();
	display_msg("I am Milky ;-;", 0);
	display_msg("and I <3 ML", 1);
	micro_wait(5000);
	hygrometer_init();
	while(1) {
		hygrometer_read(&humidity, &temperature);
		disp_temperature(temperature);
		disp_humidity(humidity);
		micro_wait(5000);
	}
}
