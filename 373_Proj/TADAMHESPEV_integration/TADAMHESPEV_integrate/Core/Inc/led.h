/*
 * led.h
 *
 *  Created on: Nov 24, 2023
 *      Author: skylarlennon
 */

#ifndef INC_LED_H_
#define INC_LED_H_

#include "stdint.h"

void setupLEDS();
void clearLEDstrip();
void ledTest();
void makeLEDFrame(uint8_t* frame, uint8_t gr_yel_red);
void printLEDs(float accVal);


#endif /* INC_LED_H_ */
