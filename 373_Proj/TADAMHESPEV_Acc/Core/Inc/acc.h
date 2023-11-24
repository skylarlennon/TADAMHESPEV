/*
 * acc.h
 *
 *  Created on: Nov 24, 2023
 *      Author: skylarlennon
 */

#include "stdint.h"

#ifndef ACC_H
#define ACC_H

// Function prototypes
void setupAccModule();
uint8_t accFloat2Binary(float accVal);
void printBinaryNewline(uint8_t accBin);
void printBinary(uint8_t accBin);
void printAll(uint16_t raw, float acc, uint8_t accBin);
float ReadAccData();
void print_raw_acc_bin();
void write4BitGPIOs(uint8_t binVal);
void writeGPIOCountTest();

// Add more function prototypes as needed

#endif /* ACC_H */
