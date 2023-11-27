/*
 * led.c
 *
 *  Created on: Nov 24, 2023
 *      Author: skylarlennon
 */

#include "led.h"
#include "math.h"
#include "stdint.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio.h"

#define NUM_LEDS 17
#define NUM_TEST_LEDS 2
#define NUM_MSG_BYTES 74 //4 + 4*NUM_LEDS + 1 + ceil((NUM_LEDS - 1)/16)
#define MAX_ACCEL 2.0
#define START_FRAME 0x00000000
#define LED_FRAME_START_BRIGHT 0xF0 // 0b111 xxxxx (for brightness)
#define LED_FRAME_START_OFF 0xE0 // 0b111 xxxxx (for brightness)
// MAKE SURE THESE SUM TO NUM_LEDS
#define NUM_GREEN 5
#define NUM_YELLOW 8
#define NUM_RED 4

float LED_INDEX_THRESHOLD[NUM_LEDS];
uint8_t LED_COLOR_ARR[NUM_LEDS];

extern SPI_HandleTypeDef hspi1;

void setupLEDS(){
	float acc_inc = MAX_ACCEL/NUM_LEDS;
	for(int i = 0; i < NUM_LEDS; ++i){
		LED_INDEX_THRESHOLD[i] = i*acc_inc;

		if(i < NUM_GREEN){
			LED_COLOR_ARR[i] = 0; // 0 = green
		}
		else if(i < NUM_GREEN + NUM_YELLOW){
			LED_COLOR_ARR[i] = 1; // 1 = yellow
		}
		else{
			LED_COLOR_ARR[i] = 2; // 2 = red
		}
	}
}

void ledTest(){
	//4 start + (4*2 led) + ceil((NUM_TEST_LEDS - 1)/16) = 13 bytes
	uint8_t buf[13];

	// Probably redundant, but fill the start frame
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 0;

	// led_frame = [31:29] = 111; [28:24] = brightness; [23:16] = Blue; [15:8] = Green; [7:0] = Red
	// frames at lower indexes are closest to the data input
	buf[13] = 0b11110000;
	buf[12] = 0;

	//LED closest to connector turned off
	buf[4] = 0b11100000; // no brightness
	buf[5] = 0;
	buf[6] = 0;
	buf[7] = 0;

	//2nd LED with RED turned on
	buf[8] = 0b11110000;
	buf[9] = 0;
	buf[10] = 0;
	buf[11] = 255;

	//end frame
	buf[12] = 0;

	HAL_StatusTypeDef ret = HAL_SPI_Transmit(&hspi1, (uint8_t *)buf, 13,1000);

	if(ret != HAL_OK){
		//There is a problem
	}
}

void makeLEDFrame(uint8_t* frame, uint8_t gr_yel_red){
	switch(gr_yel_red){
		case 0:								// green
			*(frame) = 0; 		//red
			*(frame + 1) = 255; //green
			*(frame + 2) = 0; 	//blue
			*(frame + 3) = LED_FRAME_START_BRIGHT;
			break;
		case 1:					 			// yellow
			*(frame) = 100; 	//red
			*(frame + 1) = 100; //green
			*(frame + 2) = 0; 	//blue
			*(frame + 3) = LED_FRAME_START_BRIGHT;
			break;
		case 2:					 			// red
			*(frame) = 255; 	//red
			*(frame + 1) = 0; 	//green
			*(frame + 2) = 0; 	//blue
			*(frame + 3) = LED_FRAME_START_BRIGHT;
			break;
		default:							// no brightness
			*(frame) = 0; //red
			*(frame + 1) = 0; //green
			*(frame + 2) = 255; //blue
			*(frame + 3) = LED_FRAME_START_OFF;
			break;
	}

}

void printLEDs(float accVal){
	accVal = fabs(accVal);

	uint8_t led_msg[NUM_MSG_BYTES];

	//create start frame
	int i = 0;
	for(; i < 4; ++i){
		led_msg[i] = 0;
	}

	//create LED frame
	for(int j = 0; j < NUM_LEDS; ++j){
		uint8_t led_frame[4];
		if(accVal > LED_INDEX_THRESHOLD[j]){
			makeLEDFrame(&led_frame,LED_COLOR_ARR[j]); //LED_COLOR_ARR[j] = 0,1,2
		}
		else{
			makeLEDFrame(&led_frame,4); //no brightness
		}
		//add the frame to the rest of the message
		led_msg[i] = led_frame;
		i = i + 4;
	}

	//create the end frame
	for(; i < NUM_MSG_BYTES; ++i){
		led_msg[i] = 0;
	}
}
