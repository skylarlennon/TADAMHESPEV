/*
 * acc.c
 *
 *  Created on: Nov 24, 2023
 *      Author: skylarlennon
 */
#include "acc.h"
#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "stdint.h"
//#include "stm32l4xx_hal_gpio.h"
//#include "stm32l4xx_hal_conf.h"
//#include "stm32l4xx_it.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio.h"


/**********CONFIG ACCELEROMETER**********/
#define ACC_IR_CTRL1 0x20
#define CTR1_SETUP 0b10010111 // enable xyz accel measurement and select data rate
/**********ACCELEROMETER I2C ADDR AND SUB ADDRESSES**********/
#define ACC_I2C_ADDR 	0b0011001
//#define ACC_I2C_ADDR_WRITE 	0b00110010

#define OUT_X_L_A 0x28
#define OUT_X_H_A 0x29
#define OUT_Y_L_A 0x2A
#define OUT_Y_H_A 0x2B
#define OUT_Z_L_A 0x2C
#define OUT_Z_H_A 0x2D

extern I2C_HandleTypeDef hi2c1;

void setupAccModule(){
	uint8_t buf[10]= {ACC_IR_CTRL1, CTR1_SETUP};
	HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, ACC_I2C_ADDR << 1, buf, 2, 1000);
	if(ret == 0){
		printf("We gucci\n");
	}
	else{
		printf("we not gucci setupAccModule\n");
	}
}

uint8_t accFloat2Binary(float accVal){
	if(fabs(accVal) < 0.125){
		return 0;
	}
	else if(fabs(accVal) < 0.25){
		return 1;
	}
	else if(fabs(accVal) < 0.375){
		return 2;
	}
	else if(fabs(accVal) < 0.5){
		return 3;
	}
	else if(fabs(accVal) < 0.625){
		return 4;
	}
	else if(fabs(accVal) < 0.75){
		return 5;
	}
	else if(fabs(accVal) < 0.875){
		return 6;
	}
	else if(fabs(accVal) < 1){
		return 7;
	}
	else if(fabs(accVal) < 1.125){
		return 8;
	}
	else if(fabs(accVal) < 1.25){
		return 9;
	}
	else if(fabs(accVal) < 1.375){
		return 10;
	}
	else if(fabs(accVal) < 1.5){
		return 11;
	}
	else if(fabs(accVal) < 1.625){
		return 12;
	}
	else if(fabs(accVal) < 1.75){
		return 13;
	}
	else if(fabs(accVal) < 1.875){
		return 14;
	}
	else if(fabs(accVal) < 2){
		return 15;
	}
	else{
		return 0xFF;
	}
}

void printBinaryNewline(uint8_t accBin){
	int size = sizeof(accBin)*8;
	for(int i = size - 1; i >= 0; --i){
		printf("%i",(accBin >> i)&1);
	}
	printf("\n");
}

void printBinary(uint8_t accBin){
	int size = sizeof(accBin)*8;
	for(int i = size - 1; i >= 0; --i){
		printf("%i",(accBin >> i)&1);
	}
}

void printAll(uint16_t raw, float acc, uint8_t accBin){
	printf("Raw:\t%u\tAcc:\t%f\tBinary:\t",raw,acc);
	printBinary(accBin);
	printf("\n");
}

float ReadAccData(){
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 1);
		uint8_t buf[1]= {OUT_X_L_A | 1 << 7}; //Auto-Increment OUT_X_L_A
		uint8_t rbuf[2];
		float accVal = 0;

		HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, ACC_I2C_ADDR << 1, buf, 1, 1000);
		//[TODO] - Have error checking for communication errors
		ret =  HAL_I2C_Master_Receive(&hi2c1, ACC_I2C_ADDR << 1, rbuf, 2, 1000);
		//[TODO] - Have error checking for communication errors

		uint16_t raw = (rbuf[1] << 8) | rbuf[0];	// 2's compliment, +-2g's
		if(raw > 64100){
			accVal = 0;
		}
		else if(raw & 0x8000){ //if value is negative
			int16_t temp = -((raw ^ 0xFFFF) + 1);
			accVal = (temp / (float)(1 << 15))*2;
		}
		else{ //positive acceleration
			accVal = (raw / (float)((1 << 15) - 1))*2;
		}
//		printf("Accel:\t%f\n",accVal);
		return accVal;
//		uint8_t binAcc = accFloat2Binary(accVal);
		//[TODO] - error check if binAcc = -1;

//		printf("%i\t%b\n",binAcc);

//		printf("%i\t%f\n",cnt++, accVal);
}

void print_raw_acc_bin(){
		uint8_t buf[1]= {OUT_X_L_A | 1 << 7}; //Auto-Increment OUT_X_L_A
		uint8_t rbuf[2];
		float accVal = 0;

		HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, ACC_I2C_ADDR << 1, buf, 1, 1000);
		//[TODO] - Have error checking for communication errors
		ret =  HAL_I2C_Master_Receive(&hi2c1, ACC_I2C_ADDR << 1, rbuf, 2, 1000);
		//[TODO] - Have error checking for communication errors

		uint16_t raw = (rbuf[1] << 8) | rbuf[0];	// 2's compliment, +-2g's
		if(raw > 64100){
			accVal = 0;
		}
		else if(raw & 0x8000){ //if value is negative
			int16_t temp = -((raw ^ 0xFFFF) + 1);
			accVal = (temp / (float)(1 << 15))*2;
		}
		else{ //positive acceleration
			accVal = (raw / (float)((1 << 15) - 1))*2;
		}
		uint8_t accBin = accFloat2Binary(accVal);
		printAll(raw,accVal,accBin);
}







void write4BitGPIOs(uint8_t binVal){
//	HAL_GPIO_WritePin(GPIOX, GPIO_PIN_XX,  pin_state);

	//PB6, PB5, PB4, PB3 MSB --> LSB
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, (binVal >> 3) & 1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, (binVal >> 2) & 1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, (binVal >> 1) & 1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, binVal & 1);
}

void writeGPIOCountTest(){
	// 0
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6,0); // ACC[3]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,0); // ACC[2]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,0); // ACC[1]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,0); // ACC[0]

	HAL_Delay(500);

	//1
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6,0); // ACC[3]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,0); // ACC[2]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,0); // ACC[1]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,1); // ACC[0]

	HAL_Delay(500);

	//2
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6,0); // ACC[3]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,0); // ACC[2]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,1); // ACC[1]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,0); // ACC[0]

	HAL_Delay(500);

	//3
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6,0); // ACC[3]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,0); // ACC[2]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,1); // ACC[1]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,1); // ACC[0]

	HAL_Delay(500);

	//4
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6,0); // ACC[3]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,1); // ACC[2]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,0); // ACC[1]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,0); // ACC[0]

	HAL_Delay(500);

	//5
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6,0); // ACC[3]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,1); // ACC[2]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,0); // ACC[1]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,1); // ACC[0]

	HAL_Delay(500);

	//6
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6,0); // ACC[3]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,1); // ACC[2]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,1); // ACC[1]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,0); // ACC[0]

	HAL_Delay(500);

	//7
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6,0); // ACC[3]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,1); // ACC[2]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,1); // ACC[1]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,1); // ACC[0]

	HAL_Delay(500);

	//8
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6,1); // ACC[3]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,0); // ACC[2]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,0); // ACC[1]
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,0); // ACC[0]

	HAL_Delay(500);
}
