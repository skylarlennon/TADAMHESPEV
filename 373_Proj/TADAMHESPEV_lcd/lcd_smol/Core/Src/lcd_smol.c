/*
 * LCD.c
 *
 *  Created on: Nov 8, 2023
 *      Author: ndib
 */

#include <lcd_smol.h>
#include "stm32l4xx_hal.h"
#include <stdlib.h>

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif

// Probably going to move these to a different file, but for now
void uint16_to_bytes(uint16_t in, uint8_t *out)
{
	out[0] = (in >> 8) & 0xFF;
	out[1] = in & 0xFF;
}

static uint8_t init[] = {
	HX8357_SWRESET, 0x80 + 10/5, // Soft reset, then delay 10 ms
	HX8357D_SETC, 3,
	  0xFF, 0x83, 0x57,
	0xFF, 0x80 + 300/5,          // No command, just delay 300 ms
	HX8357_SETRGB, 4,
	  0x80, 0x00, 0x06, 0x06,    // 0x80 enables SDO pin (0x00 disables)
	HX8357D_SETCOM, 1,
	  0x25,                      // -1.52V
	HX8357_SETOSC, 1,
	  0x68,                      // Normal mode 70Hz, Idle mode 55 Hz
	HX8357_SETPANEL, 1,
	  0x05,                      // BGR, Gate direction swapped
	HX8357_SETPWR1, 6,
	  0x00,                      // Not deep standby
	  0x15,                      // BT
	  0x1C,                      // VSPR
	  0x1C,                      // VSNR
	  0x83,                      // AP
	  0xAA,                      // FS
	HX8357D_SETSTBA, 6,
	  0x50,                      // OPON normal
	  0x50,                      // OPON idle
	  0x01,                      // STBA
	  0x3C,                      // STBA
	  0x1E,                      // STBA
	  0x08,                      // GEN
	HX8357D_SETCYC, 7,
	  0x02,                      // NW 0x02
	  0x40,                      // RTN
	  0x00,                      // DIV
	  0x2A,                      // DUM
	  0x2A,                      // DUM
	  0x0D,                      // GDON
	  0x78,                      // GDOFF
	HX8357D_SETGAMMA, 34,
	  0x02, 0x0A, 0x11, 0x1d, 0x23, 0x35, 0x41, 0x4b, 0x4b,
	  0x42, 0x3A, 0x27, 0x1B, 0x08, 0x09, 0x03, 0x02, 0x0A,
	  0x11, 0x1d, 0x23, 0x35, 0x41, 0x4b, 0x4b, 0x42, 0x3A,
	  0x27, 0x1B, 0x08, 0x09, 0x03, 0x00, 0x01,
	HX8357_COLMOD, 1,
	  0x55,                      // 16 bit
	HX8357_MADCTL, 1,			// edit this to adjust rotation (horizontal = 0x60, vertical = 0xC0)
	  0x60,
	HX8357_TEON, 1,
	  0x00,                      // TW off
	HX8357_TEARLINE, 2,
	  0x00, 0x02,
	HX8357_SLPOUT, 0x80 + 150/5, // Exit Sleep, then delay 150 ms
	HX8357_DISPON, 0x80 +  50/5, // Main screen turn on, delay 50 ms
	0,                           // END OF COMMAND LIST
  };

void LCD_reset()
{
	HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_SET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_SET);
	HAL_Delay(200);
}

HAL_StatusTypeDef LCD_writeCommand(SPI_HandleTypeDef* spi, uint8_t cmd)
{
	HAL_StatusTypeDef result;
	HAL_GPIO_WritePin(DC_PORT, DC_PIN, GPIO_PIN_RESET);
	uint8_t buf[1];
	result = HAL_SPI_TransmitReceive(spi, &cmd, &buf[0], 1, 100);
	HAL_GPIO_WritePin(DC_PORT, DC_PIN, GPIO_PIN_SET);
	return result;
}

void LCD_startWrite()
{
	HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void LCD_endWrite()
{
	HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

int LCD_begin(SPI_HandleTypeDef* spi)
{
	LCD_reset();
	LCD_startWrite();
	uint8_t *addr = init;
	uint8_t cmd, x, numArgs;
	HAL_StatusTypeDef result;
	while((cmd = *(addr++)) > 0) // '0' command ends list
	{
		if (cmd != 0xFF) // '255' is ignored
		{
			result = LCD_writeCommand(spi, cmd);
			if (result != HAL_OK)
			{
				LCD_endWrite();
				return 0;
			}
		}
		x = *(addr++);
		numArgs = x & 0x7F;
		if (x & 0x80) // If high bit set...
		{
			HAL_Delay(numArgs * 5); // numArgs is actually a delay time (5ms units)
		}
		else // Otherwise, issue args to command...
		{
			while (numArgs--)
			{
				result = HAL_SPI_Transmit(spi, addr++, 1, 100);
				if (result != HAL_OK)
				{
					LCD_endWrite();
					return 0;
				}
			}
		}
	}
	LCD_endWrite();
	return 1;
}

int invertDisplay(SPI_HandleTypeDef* spi, int invert)
{
	LCD_startWrite();
	HAL_StatusTypeDef result = LCD_writeCommand(spi, invert ? HX8357_INVON : HX8357_INVOFF);
	LCD_endWrite();
	return result == HAL_OK;
}

int LCD_setAddrWindow(SPI_HandleTypeDef* spi, uint16_t x1, uint16_t y1, uint16_t w, uint16_t h)
{
	uint16_t x2 = x1 + w - 1;
	uint16_t y2 = y1 + h - 1;
	uint8_t buf[2];

	LCD_startWrite();
	LCD_writeCommand(spi, HX8357_CASET); // Column address set

	uint16_to_bytes(x1, buf);
	HAL_SPI_Transmit(spi, &buf[0], 2, 100);

	uint16_to_bytes(x2, buf);
	HAL_SPI_Transmit(spi, &buf[0], 2, 100);

	LCD_writeCommand(spi, HX8357_PASET); // Row address set

	uint16_to_bytes(y1, buf);
	HAL_SPI_Transmit(spi, &buf[0], 2, 100);

	uint16_to_bytes(y2, buf);
	HAL_SPI_Transmit(spi, &buf[0], 2, 100);

	LCD_endWrite();
	return 1;
}

uint16_t LCD_color565(uint8_t red, uint8_t green, uint8_t blue) {
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
}

int LCD_pushColor(SPI_HandleTypeDef* spi, uint16_t color) {
	LCD_startWrite();
	LCD_writeCommand(spi, HX8357_RAMWR); // Write to RAM
	HAL_SPI_Transmit(spi, (uint8_t*)&color, 2, 100);
	LCD_writeCommand(spi, HX8357_NOP); // Write command to stop RAMWR
	LCD_endWrite();
	return 1;
}

int LCD_pushColorCopy(SPI_HandleTypeDef* spi, uint16_t color, uint32_t count) {
	LCD_startWrite();

	LCD_writeCommand(spi, HX8357_RAMWR); // Write to RAM

	uint8_t buf[2];
	uint16_to_bytes(color, &buf[0]);
	for (int i = 0; i < count; ++i)
	{
		HAL_SPI_Transmit(spi, &buf[0], 2, 100);
	}

	LCD_writeCommand(spi, HX8357_NOP); // Write command to stop RAMWR

	LCD_endWrite();
	return 1;
}

int LCD_pushColors(SPI_HandleTypeDef* spi, uint16_t* colors, uint32_t len) {
	HAL_StatusTypeDef result;
	LCD_startWrite();

	result = LCD_writeCommand(spi, HX8357_RAMWR); // Write to RAM
    if (result != HAL_OK) return 0;

	result = HAL_SPI_Transmit(spi, (uint8_t*)colors, 2*len, 100);
	if (result != HAL_OK) return 0;

	result = LCD_writeCommand(spi, HX8357_NOP); // Write command to stop RAMWR
	if (result != HAL_OK) return 0;

	LCD_endWrite();
	return 1;
}

int LCD_writePixel(SPI_HandleTypeDef* spi, int16_t x, int16_t y, uint16_t color) {
    if ((x < 0) ||(x >= HX8357_TFTWIDTH) || (y < 0) || (y >= HX8357_TFTHEIGHT)) return 0;
    if(!LCD_setAddrWindow(spi, x, y, 1, 1)) return 0;
    if (!LCD_pushColor(spi, color)) return 0;
    return 1;
}

// Length of colors array must be w * h
int LCD_writePixels(SPI_HandleTypeDef* spi, uint16_t color,
					int16_t x, int16_t y, int16_t w, int16_t h)
{
	if ((x < 0) ||(x >= HX8357_TFTWIDTH) || (y < 0) || (y >= HX8357_TFTHEIGHT)) return 0;
	if(!LCD_setAddrWindow(spi, x, y, w, h)) return 0;
	if (!LCD_pushColorCopy(spi, color, w*h)) return 0;
	return 1;
}

uint16_t LCD_readPixel(SPI_HandleTypeDef* spi, int16_t x, int16_t y)
{
	if ((x < 0) ||(x >= HX8357_TFTWIDTH) || (y < 0) || (y >= HX8357_TFTHEIGHT)) return 0;
	if(!LCD_setAddrWindow(spi, x, y, 1, 1)) return 0;

	uint8_t color[2] = {0, 0};
	HAL_StatusTypeDef result;
	LCD_startWrite();

	result = LCD_writeCommand(spi, HX8357_RAMRD); // Write to RAM
	if (result != HAL_OK) return 0;

	result = HAL_SPI_Receive(spi, (uint8_t*)&color[0], 2, 100);
	if (result != HAL_OK) return 0;

	result = LCD_writeCommand(spi, HX8357_NOP); // Write command to stop RAMWR
	if (result != HAL_OK) return 0;

	LCD_endWrite();

	uint16_t ret = *((uint16_t*)&color[0]);
	return ret;
}


static const unsigned char font[] = {
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x3E, 0x5B, 0x4F, 0x5B, 0x3E,
	0x3E, 0x6B, 0x4F, 0x6B, 0x3E,
	0x1C, 0x3E, 0x7C, 0x3E, 0x1C,
	0x18, 0x3C, 0x7E, 0x3C, 0x18,
	0x1C, 0x57, 0x7D, 0x57, 0x1C,
	0x1C, 0x5E, 0x7F, 0x5E, 0x1C,
	0x00, 0x18, 0x3C, 0x18, 0x00,
	0xFF, 0xE7, 0xC3, 0xE7, 0xFF,
	0x00, 0x18, 0x24, 0x18, 0x00,
	0xFF, 0xE7, 0xDB, 0xE7, 0xFF,
	0x30, 0x48, 0x3A, 0x06, 0x0E,
	0x26, 0x29, 0x79, 0x29, 0x26,
	0x40, 0x7F, 0x05, 0x05, 0x07,
	0x40, 0x7F, 0x05, 0x25, 0x3F,
	0x5A, 0x3C, 0xE7, 0x3C, 0x5A,
	0x7F, 0x3E, 0x1C, 0x1C, 0x08,
	0x08, 0x1C, 0x1C, 0x3E, 0x7F,
	0x14, 0x22, 0x7F, 0x22, 0x14,
	0x5F, 0x5F, 0x00, 0x5F, 0x5F,
	0x06, 0x09, 0x7F, 0x01, 0x7F,
	0x00, 0x66, 0x89, 0x95, 0x6A,
	0x60, 0x60, 0x60, 0x60, 0x60,
	0x94, 0xA2, 0xFF, 0xA2, 0x94,
	0x08, 0x04, 0x7E, 0x04, 0x08,
	0x10, 0x20, 0x7E, 0x20, 0x10,
	0x08, 0x08, 0x2A, 0x1C, 0x08,
	0x08, 0x1C, 0x2A, 0x08, 0x08,
	0x1E, 0x10, 0x10, 0x10, 0x10,
	0x0C, 0x1E, 0x0C, 0x1E, 0x0C,
	0x30, 0x38, 0x3E, 0x38, 0x30,
	0x06, 0x0E, 0x3E, 0x0E, 0x06,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x5F, 0x00, 0x00,
	0x00, 0x07, 0x00, 0x07, 0x00,
	0x14, 0x7F, 0x14, 0x7F, 0x14,
	0x24, 0x2A, 0x7F, 0x2A, 0x12,
	0x23, 0x13, 0x08, 0x64, 0x62,
	0x36, 0x49, 0x56, 0x20, 0x50,
	0x00, 0x08, 0x07, 0x03, 0x00,
	0x00, 0x1C, 0x22, 0x41, 0x00,
	0x00, 0x41, 0x22, 0x1C, 0x00,
	0x2A, 0x1C, 0x7F, 0x1C, 0x2A,
	0x08, 0x08, 0x3E, 0x08, 0x08,
	0x00, 0x80, 0x70, 0x30, 0x00,
	0x08, 0x08, 0x08, 0x08, 0x08,
	0x00, 0x00, 0x60, 0x60, 0x00,
	0x20, 0x10, 0x08, 0x04, 0x02,
	0x3E, 0x51, 0x49, 0x45, 0x3E,
	0x00, 0x42, 0x7F, 0x40, 0x00,
	0x72, 0x49, 0x49, 0x49, 0x46,
	0x21, 0x41, 0x49, 0x4D, 0x33,
	0x18, 0x14, 0x12, 0x7F, 0x10,
	0x27, 0x45, 0x45, 0x45, 0x39,
	0x3C, 0x4A, 0x49, 0x49, 0x31,
	0x41, 0x21, 0x11, 0x09, 0x07,
	0x36, 0x49, 0x49, 0x49, 0x36,
	0x46, 0x49, 0x49, 0x29, 0x1E,
	0x00, 0x00, 0x14, 0x00, 0x00,
	0x00, 0x40, 0x34, 0x00, 0x00,
	0x00, 0x08, 0x14, 0x22, 0x41,
	0x14, 0x14, 0x14, 0x14, 0x14,
	0x00, 0x41, 0x22, 0x14, 0x08,
	0x02, 0x01, 0x59, 0x09, 0x06,
	0x3E, 0x41, 0x5D, 0x59, 0x4E,
	0x7C, 0x12, 0x11, 0x12, 0x7C,
	0x7F, 0x49, 0x49, 0x49, 0x36,
	0x3E, 0x41, 0x41, 0x41, 0x22,
	0x7F, 0x41, 0x41, 0x41, 0x3E,
	0x7F, 0x49, 0x49, 0x49, 0x41,
	0x7F, 0x09, 0x09, 0x09, 0x01,
	0x3E, 0x41, 0x41, 0x51, 0x73,
	0x7F, 0x08, 0x08, 0x08, 0x7F,
	0x00, 0x41, 0x7F, 0x41, 0x00,
	0x20, 0x40, 0x41, 0x3F, 0x01,
	0x7F, 0x08, 0x14, 0x22, 0x41,
	0x7F, 0x40, 0x40, 0x40, 0x40,
	0x7F, 0x02, 0x1C, 0x02, 0x7F,
	0x7F, 0x04, 0x08, 0x10, 0x7F,
	0x3E, 0x41, 0x41, 0x41, 0x3E,
	0x7F, 0x09, 0x09, 0x09, 0x06,
	0x3E, 0x41, 0x51, 0x21, 0x5E,
	0x7F, 0x09, 0x19, 0x29, 0x46,
	0x26, 0x49, 0x49, 0x49, 0x32,
	0x03, 0x01, 0x7F, 0x01, 0x03,
	0x3F, 0x40, 0x40, 0x40, 0x3F,
	0x1F, 0x20, 0x40, 0x20, 0x1F,
	0x3F, 0x40, 0x38, 0x40, 0x3F,
	0x63, 0x14, 0x08, 0x14, 0x63,
	0x03, 0x04, 0x78, 0x04, 0x03,
	0x61, 0x59, 0x49, 0x4D, 0x43,
	0x00, 0x7F, 0x41, 0x41, 0x41,
	0x02, 0x04, 0x08, 0x10, 0x20,
	0x00, 0x41, 0x41, 0x41, 0x7F,
	0x04, 0x02, 0x01, 0x02, 0x04,
	0x40, 0x40, 0x40, 0x40, 0x40,
	0x00, 0x03, 0x07, 0x08, 0x00,
	0x20, 0x54, 0x54, 0x78, 0x40,
	0x7F, 0x28, 0x44, 0x44, 0x38,
	0x38, 0x44, 0x44, 0x44, 0x28,
	0x38, 0x44, 0x44, 0x28, 0x7F,
	0x38, 0x54, 0x54, 0x54, 0x18,
	0x00, 0x08, 0x7E, 0x09, 0x02,
	0x18, 0xA4, 0xA4, 0x9C, 0x78,
	0x7F, 0x08, 0x04, 0x04, 0x78,
	0x00, 0x44, 0x7D, 0x40, 0x00,
	0x20, 0x40, 0x40, 0x3D, 0x00,
	0x7F, 0x10, 0x28, 0x44, 0x00,
	0x00, 0x41, 0x7F, 0x40, 0x00,
	0x7C, 0x04, 0x78, 0x04, 0x78,
	0x7C, 0x08, 0x04, 0x04, 0x78,
	0x38, 0x44, 0x44, 0x44, 0x38,
	0xFC, 0x18, 0x24, 0x24, 0x18,
	0x18, 0x24, 0x24, 0x18, 0xFC,
	0x7C, 0x08, 0x04, 0x04, 0x08,
	0x48, 0x54, 0x54, 0x54, 0x24,
	0x04, 0x04, 0x3F, 0x44, 0x24,
	0x3C, 0x40, 0x40, 0x20, 0x7C,
	0x1C, 0x20, 0x40, 0x20, 0x1C,
	0x3C, 0x40, 0x30, 0x40, 0x3C,
	0x44, 0x28, 0x10, 0x28, 0x44,
	0x4C, 0x90, 0x90, 0x90, 0x7C,
	0x44, 0x64, 0x54, 0x4C, 0x44,
	0x00, 0x08, 0x36, 0x41, 0x00,
	0x00, 0x00, 0x77, 0x00, 0x00,
	0x00, 0x41, 0x36, 0x08, 0x00,
	0x02, 0x01, 0x02, 0x04, 0x02,
	0x3C, 0x26, 0x23, 0x26, 0x3C,
	0x1E, 0xA1, 0xA1, 0x61, 0x12,
	0x3A, 0x40, 0x40, 0x20, 0x7A,
	0x38, 0x54, 0x54, 0x55, 0x59,
	0x21, 0x55, 0x55, 0x79, 0x41,
	0x22, 0x54, 0x54, 0x78, 0x42, // a-umlaut
	0x21, 0x55, 0x54, 0x78, 0x40,
	0x20, 0x54, 0x55, 0x79, 0x40,
	0x0C, 0x1E, 0x52, 0x72, 0x12,
	0x39, 0x55, 0x55, 0x55, 0x59,
	0x39, 0x54, 0x54, 0x54, 0x59,
	0x39, 0x55, 0x54, 0x54, 0x58,
	0x00, 0x00, 0x45, 0x7C, 0x41,
	0x00, 0x02, 0x45, 0x7D, 0x42,
	0x00, 0x01, 0x45, 0x7C, 0x40,
	0x7D, 0x12, 0x11, 0x12, 0x7D, // A-umlaut
	0xF0, 0x28, 0x25, 0x28, 0xF0,
	0x7C, 0x54, 0x55, 0x45, 0x00,
	0x20, 0x54, 0x54, 0x7C, 0x54,
	0x7C, 0x0A, 0x09, 0x7F, 0x49,
	0x32, 0x49, 0x49, 0x49, 0x32,
	0x3A, 0x44, 0x44, 0x44, 0x3A, // o-umlaut
	0x32, 0x4A, 0x48, 0x48, 0x30,
	0x3A, 0x41, 0x41, 0x21, 0x7A,
	0x3A, 0x42, 0x40, 0x20, 0x78,
	0x00, 0x9D, 0xA0, 0xA0, 0x7D,
	0x3D, 0x42, 0x42, 0x42, 0x3D, // O-umlaut
	0x3D, 0x40, 0x40, 0x40, 0x3D,
	0x3C, 0x24, 0xFF, 0x24, 0x24,
	0x48, 0x7E, 0x49, 0x43, 0x66,
	0x2B, 0x2F, 0xFC, 0x2F, 0x2B,
	0xFF, 0x09, 0x29, 0xF6, 0x20,
	0xC0, 0x88, 0x7E, 0x09, 0x03,
	0x20, 0x54, 0x54, 0x79, 0x41,
	0x00, 0x00, 0x44, 0x7D, 0x41,
	0x30, 0x48, 0x48, 0x4A, 0x32,
	0x38, 0x40, 0x40, 0x22, 0x7A,
	0x00, 0x7A, 0x0A, 0x0A, 0x72,
	0x7D, 0x0D, 0x19, 0x31, 0x7D,
	0x26, 0x29, 0x29, 0x2F, 0x28,
	0x26, 0x29, 0x29, 0x29, 0x26,
	0x30, 0x48, 0x4D, 0x40, 0x20,
	0x38, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x38,
	0x2F, 0x10, 0xC8, 0xAC, 0xBA,
	0x2F, 0x10, 0x28, 0x34, 0xFA,
	0x00, 0x00, 0x7B, 0x00, 0x00,
	0x08, 0x14, 0x2A, 0x14, 0x22,
	0x22, 0x14, 0x2A, 0x14, 0x08,
	0x55, 0x00, 0x55, 0x00, 0x55, // #176 (25% block) missing in old code
	0xAA, 0x55, 0xAA, 0x55, 0xAA, // 50% block
	0xFF, 0x55, 0xFF, 0x55, 0xFF, // 75% block
	0x00, 0x00, 0x00, 0xFF, 0x00,
	0x10, 0x10, 0x10, 0xFF, 0x00,
	0x14, 0x14, 0x14, 0xFF, 0x00,
	0x10, 0x10, 0xFF, 0x00, 0xFF,
	0x10, 0x10, 0xF0, 0x10, 0xF0,
	0x14, 0x14, 0x14, 0xFC, 0x00,
	0x14, 0x14, 0xF7, 0x00, 0xFF,
	0x00, 0x00, 0xFF, 0x00, 0xFF,
	0x14, 0x14, 0xF4, 0x04, 0xFC,
	0x14, 0x14, 0x17, 0x10, 0x1F,
	0x10, 0x10, 0x1F, 0x10, 0x1F,
	0x14, 0x14, 0x14, 0x1F, 0x00,
	0x10, 0x10, 0x10, 0xF0, 0x00,
	0x00, 0x00, 0x00, 0x1F, 0x10,
	0x10, 0x10, 0x10, 0x1F, 0x10,
	0x10, 0x10, 0x10, 0xF0, 0x10,
	0x00, 0x00, 0x00, 0xFF, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0xFF, 0x10,
	0x00, 0x00, 0x00, 0xFF, 0x14,
	0x00, 0x00, 0xFF, 0x00, 0xFF,
	0x00, 0x00, 0x1F, 0x10, 0x17,
	0x00, 0x00, 0xFC, 0x04, 0xF4,
	0x14, 0x14, 0x17, 0x10, 0x17,
	0x14, 0x14, 0xF4, 0x04, 0xF4,
	0x00, 0x00, 0xFF, 0x00, 0xF7,
	0x14, 0x14, 0x14, 0x14, 0x14,
	0x14, 0x14, 0xF7, 0x00, 0xF7,
	0x14, 0x14, 0x14, 0x17, 0x14,
	0x10, 0x10, 0x1F, 0x10, 0x1F,
	0x14, 0x14, 0x14, 0xF4, 0x14,
	0x10, 0x10, 0xF0, 0x10, 0xF0,
	0x00, 0x00, 0x1F, 0x10, 0x1F,
	0x00, 0x00, 0x00, 0x1F, 0x14,
	0x00, 0x00, 0x00, 0xFC, 0x14,
	0x00, 0x00, 0xF0, 0x10, 0xF0,
	0x10, 0x10, 0xFF, 0x10, 0xFF,
	0x14, 0x14, 0x14, 0xFF, 0x14,
	0x10, 0x10, 0x10, 0x1F, 0x00,
	0x00, 0x00, 0x00, 0xF0, 0x10,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0xFF, 0xFF, 0xFF, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xFF, 0xFF,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
	0x38, 0x44, 0x44, 0x38, 0x44,
	0xFC, 0x4A, 0x4A, 0x4A, 0x34, // sharp-s or beta
	0x7E, 0x02, 0x02, 0x06, 0x06,
	0x02, 0x7E, 0x02, 0x7E, 0x02,
	0x63, 0x55, 0x49, 0x41, 0x63,
	0x38, 0x44, 0x44, 0x3C, 0x04,
	0x40, 0x7E, 0x20, 0x1E, 0x20,
	0x06, 0x02, 0x7E, 0x02, 0x02,
	0x99, 0xA5, 0xE7, 0xA5, 0x99,
	0x1C, 0x2A, 0x49, 0x2A, 0x1C,
	0x4C, 0x72, 0x01, 0x72, 0x4C,
	0x30, 0x4A, 0x4D, 0x4D, 0x30,
	0x30, 0x48, 0x78, 0x48, 0x30,
	0xBC, 0x62, 0x5A, 0x46, 0x3D,
	0x3E, 0x49, 0x49, 0x49, 0x00,
	0x7E, 0x01, 0x01, 0x01, 0x7E,
	0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
	0x44, 0x44, 0x5F, 0x44, 0x44,
	0x40, 0x51, 0x4A, 0x44, 0x40,
	0x40, 0x44, 0x4A, 0x51, 0x40,
	0x00, 0x00, 0xFF, 0x01, 0x03,
	0xE0, 0x80, 0xFF, 0x00, 0x00,
	0x08, 0x08, 0x6B, 0x6B, 0x08,
	0x36, 0x12, 0x36, 0x24, 0x36,
	0x06, 0x0F, 0x09, 0x0F, 0x06,
	0x00, 0x00, 0x18, 0x18, 0x00,
	0x00, 0x00, 0x10, 0x10, 0x00,
	0x30, 0x40, 0xFF, 0x01, 0x01,
	0x00, 0x1F, 0x01, 0x01, 0x1E,
	0x00, 0x19, 0x1D, 0x17, 0x12,
	0x00, 0x3C, 0x3C, 0x3C, 0x3C,
	0x00, 0x00, 0x00, 0x00, 0x00  // #255 NBSP
};

static const unsigned char buttons[] = {
		0x70, 0x1C, 0x07, 0x1C, 0x70,
		0x0E, 0x38, 0xE0, 0x38, 0x0E
};

void LCD_drawChar(SPI_HandleTypeDef* spi, int16_t x, int16_t y, unsigned char c, uint16_t color, uint32_t size)
{
	if((x >= HX8357_TFTWIDTH)   || // Clip right
	   (y >= HX8357_TFTHEIGHT)  || // Clip bottom
	   ((x + 6 * size - 1) < 0) || // Clip left
	   ((y + 8 * size - 1) < 0))   // Clip top
		return;

	uint16_t bg = LCD_color565(255, 255, 255);
	LCD_startWrite();
	for(int8_t i = 0; i < 5; i++) // Char bitmap = 5 columns
	{
		uint8_t line = font[c * 5 + i];
		for(int8_t j = 0; j < 8; j++, line >>= 1)
		{
			if (line & 1)
			{
				if (size == 1) LCD_writePixel(spi, x+i, y+j, color);
				else LCD_writePixels(spi, color, x+i*size, y+j*size, size, size);
			}
			else
			{
				if (size == 1) LCD_writePixel(spi, x+i, y+j, bg);
				else LCD_writePixels(spi, bg, x+i*size, y+j*size, size, size);
			}
		}
	}
	LCD_endWrite();
}

void LCD_drawCharOPT(SPI_HandleTypeDef* spi, int16_t x, int16_t y, unsigned char c1, unsigned char c2, uint16_t color, uint32_t size)
{
	if((x >= HX8357_TFTWIDTH)   || // Clip right
	   (y >= HX8357_TFTHEIGHT)  || // Clip bottom
	   ((x + 6 * size - 1) < 0) || // Clip left
	   ((y + 8 * size - 1) < 0))   // Clip top
		return;

	uint16_t bg = LCD_color565(255, 255, 255);
	LCD_startWrite();
	for(int8_t i = 0; i < 5; i++) // Char bitmap = 5 columns
	{
		uint8_t line1 = font[c1 * 5 + i];
		uint8_t line2 = font[c2 * 5 + i];
		for(int8_t j = 0; j < 8; j++, line1 >>= 1, line2 >>= 1)
		{
			if ((line1 ^ line2) & 1)
			{
				if (line1 & 1)
				{
					if (size == 1) LCD_writePixel(spi, x+i, y+j, color);
					else LCD_writePixels(spi, color, x+i*size, y+j*size, size, size);
				}
				else
				{
					if (size == 1) LCD_writePixel(spi, x+i, y+j, bg);
					else LCD_writePixels(spi, bg, x+i*size, y+j*size, size, size);
				}
			}
		}
	}
	LCD_endWrite();
}

void LCD_drawCharNoBG(SPI_HandleTypeDef* spi, int16_t x, int16_t y, unsigned char c, uint16_t color, uint32_t size)
{
	if((x >= HX8357_TFTWIDTH)   || // Clip right
	   (y >= HX8357_TFTHEIGHT)  || // Clip bottom
	   ((x + 6 * size - 1) < 0) || // Clip left
	   ((y + 8 * size - 1) < 0))   // Clip top
		return;

	LCD_startWrite();
	for(int8_t i = 0; i < 5; i++) // Char bitmap = 5 columns
	{
		uint8_t line = font[c * 5 + i];
		for(int8_t j = 0; j < 8; j++, line >>= 1)
		{
			if (line & 1)
			{
				if (size == 1) LCD_writePixel(spi, x+i, y+j, color);
				else LCD_writePixels(spi, color, x+i*size, y+j*size, size, size);
			}
		}
	}
	LCD_endWrite();
}

void LCD_drawString(SPI_HandleTypeDef* spi, int16_t x, int16_t y, unsigned char* c, uint32_t length, uint16_t color, uint32_t size)
{
	for (int i = 0; i < length; ++i) LCD_drawChar(spi, x + i*6*size, y, c[i], color, size);
}

void LCD_drawStringOPT(SPI_HandleTypeDef* spi, int16_t x, int16_t y, unsigned char* c1, unsigned char* c2, uint32_t length, uint16_t color, uint32_t size)
{
	for (int i = 0; i < length; ++i) LCD_drawCharOPT(spi, x + i*6*size, y, c1[i], c2[i], color, size);
}

void LCD_drawStringNoBG(SPI_HandleTypeDef* spi, int16_t x, int16_t y, unsigned char* c, uint32_t length, uint16_t color, uint32_t size)
{
	for (int i = 0; i < length; ++i) LCD_drawCharNoBG(spi, x + i*6*size, y, c[i], color, size);
}

void LCD_drawButton(SPI_HandleTypeDef* spi, int16_t x, int16_t y, int button, uint16_t color, uint32_t size)
{
	if((x >= HX8357_TFTWIDTH)   || // Clip right
	   (y >= HX8357_TFTHEIGHT)  || // Clip bottom
	   ((x + 6 * size - 1) < 0) || // Clip left
	   ((y + 8 * size - 1) < 0))   // Clip top
		return;

	uint16_t bg = LCD_color565(255, 255, 255);
	LCD_startWrite();
	for(int8_t i = 0; i < 5; i++) // Char bitmap = 5 columns
	{
		uint8_t line = buttons[button * 5 + i];
		for(int8_t j = 0; j < 8; j++, line >>= 1)
		{
			if (line & 1)
			{
				if (size == 1) LCD_writePixel(spi, x+i, y+j, color);
				else LCD_writePixels(spi, color, x+i*size, y+j*size, size, size);
			}
			else
			{
				if (size == 1) LCD_writePixel(spi, x+i, y+j, bg);
				else LCD_writePixels(spi, bg, x+i*size, y+j*size, size, size);
			}
		}
	}
	LCD_endWrite();
}

void LCD_drawButtonNoBG(SPI_HandleTypeDef* spi, int16_t x, int16_t y, int button, uint16_t color, uint32_t size)
{
	if((x >= HX8357_TFTWIDTH)   || // Clip right
	   (y >= HX8357_TFTHEIGHT)  || // Clip bottom
	   ((x + 6 * size - 1) < 0) || // Clip left
	   ((y + 8 * size - 1) < 0))   // Clip top
		return;

	LCD_startWrite();
	for(int8_t i = 0; i < 5; i++) // Char bitmap = 5 columns
	{
		uint8_t line = buttons[button * 5 + i];
		for(int8_t j = 0; j < 8; j++, line >>= 1)
		{
			if (line & 1)
			{
				if (size == 1) LCD_writePixel(spi, x+i, y+j, color);
				else LCD_writePixels(spi, color, x+i*size, y+j*size, size, size);
			}
		}
	}
	LCD_endWrite();
}

void LCD_writeLine(SPI_HandleTypeDef *spi, int x0, int y0, int x1, int y1, int color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1) {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      LCD_writePixel(spi, y0, x0, color);
    } else {
      LCD_writePixel(spi, x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}


void LCD_writeFastVLine(SPI_HandleTypeDef *spi, int16_t x, int16_t y, int16_t h, uint16_t color) {
	LCD_writeLine(spi, x, y, x, y + h - 1, color);
}

void LCD_writeFastHLine(SPI_HandleTypeDef *spi, int16_t x, int16_t y, int16_t w, uint16_t color) {
	LCD_writeLine(spi, x, y, x + w - 1, y, color);
}

void LCD_fillRect(SPI_HandleTypeDef *spi, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
	for (int16_t i = y; i < y + h; i++) {
		LCD_writeFastHLine(spi, x, i, w, color);
	}
}

void LCD_fillTriangle(SPI_HandleTypeDef *spi, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
	int16_t a, b, y, last;

	  // Sort coordinates by Y order (y2 >= y1 >= y0)
	  if (y0 > y1) {
	    _swap_int16_t(y0, y1);
	    _swap_int16_t(x0, x1);
	  }
	  if (y1 > y2) {
	    _swap_int16_t(y2, y1);
	    _swap_int16_t(x2, x1);
	  }
	  if (y0 > y1) {
	    _swap_int16_t(y0, y1);
	    _swap_int16_t(x0, x1);
	  }

	  if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
	    a = b = x0;
	    if (x1 < a)
	      a = x1;
	    else if (x1 > b)
	      b = x1;
	    if (x2 < a)
	      a = x2;
	    else if (x2 > b)
	      b = x2;
	    LCD_writeFastHLine(spi, a, y0, b - a + 1, color);
	    return;
	  }

	  int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
	          dx12 = x2 - x1, dy12 = y2 - y1;
	  int32_t sa = 0, sb = 0;

	  // For upper part of triangle, find scanline crossings for segments
	  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
	  // is included here (and second loop will be skipped, avoiding a /0
	  // error there), otherwise scanline y1 is skipped here and handled
	  // in the second loop...which also avoids a /0 error here if y0=y1
	  // (flat-topped triangle).
	  if (y1 == y2)
	    last = y1; // Include y1 scanline
	  else
	    last = y1 - 1; // Skip it

	  for (y = y0; y <= last; y++) {
	    a = x0 + sa / dy01;
	    b = x0 + sb / dy02;
	    sa += dx01;
	    sb += dx02;
	    /* longhand:
	    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
	    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
	    */
	    if (a > b)
	      _swap_int16_t(a, b);
	    LCD_writeFastHLine(spi, a, y, b - a + 1, color);
	  }

	  // For lower part of triangle, find scanline crossings for segments
	  // 0-2 and 1-2.  This loop is skipped if y1=y2.
	  sa = (int32_t)dx12 * (y - y1);
	  sb = (int32_t)dx02 * (y - y0);
	  for (; y <= y2; y++) {
	    a = x1 + sa / dy12;
	    b = x0 + sb / dy02;
	    sa += dx12;
	    sb += dx02;
	    /* longhand:
	    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
	    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
	    */
	    if (a > b)
	      _swap_int16_t(a, b);
	    LCD_writeFastHLine(spi, a, y, b - a + 1, color);
	  }
}

//homemade functions for TADAMHASPEV, move to different file
void LCD_drawBattery(SPI_HandleTypeDef* spi, int16_t x, int16_t y, uint32_t size) {
	//make battery thicker?
	//left
	if(!LCD_setAddrWindow(spi, x, y + size, 1, 22*size)) return;
	if (!LCD_pushColorCopy(spi, HX8357_BLACK, 22*size)) return;

	//right
	if(!LCD_setAddrWindow(spi, x + 10*size, y + size, 1, 22*size)) return;
	if (!LCD_pushColorCopy(spi, HX8357_BLACK, 22*size)) return;

	//top
	if(!LCD_setAddrWindow(spi, x, y + size, 10*size, 1)) return;
	if (!LCD_pushColorCopy(spi, HX8357_BLACK, 10*size)) return;

	//bottom
	if(!LCD_setAddrWindow(spi, x, y + 23*size, 10*size, 1)) return;
	if (!LCD_pushColorCopy(spi, HX8357_BLACK, 10*size)) return;

	//lil cap
	if(!LCD_setAddrWindow(spi, x + 3*size, y, 4*size, 1)) return;
	if (!LCD_pushColorCopy(spi, HX8357_BLACK, 4*size)) return;

	if(!LCD_setAddrWindow(spi, x + 3*size, y, 1, size)) return;
	if (!LCD_pushColorCopy(spi, HX8357_BLACK, size)) return;

	if(!LCD_setAddrWindow(spi, x + 3*size + 4*size, y, 1, size)) return;
	if (!LCD_pushColorCopy(spi, HX8357_BLACK, size)) return;
	return;
}

void LCD_drawFrame(SPI_HandleTypeDef* spi) {
	//rows
	LCD_writeLine(spi,0,80,360,80,HX8357_BLACK);
	LCD_writeLine(spi,0,160,360,160,HX8357_BLACK);
	LCD_writeLine(spi,0,240,360,240,HX8357_BLACK);

	//column
	LCD_writeLine(spi,360,0,360,320,HX8357_BLACK);
	return;
}

int offset = 0;

void LCD_fillBattery(SPI_HandleTypeDef* spi, int16_t x, int16_t y, uint32_t size, int level) {
	LCD_writePixels(spi, HX8357_WHITE, x + 3, ((y + size) + 3), 10*size - 6, (22*size - 6));
	offset = 22*size - (22*size)*(level/100.0);
	if (level > 0) {
		if (level < 20) {
			LCD_writePixels(spi, HX8357_RED, x + 3, ((y + size) + 3) + offset, 10*size - 6, (22*size - 6) - offset);
		} else if (level < 50) {
			LCD_writePixels(spi, HX8357_YELLOW, x + 3, ((y + size) + 3) + offset, 10*size - 6, (22*size - 6) - offset);
		} else {
			LCD_writePixels(spi, HX8357_GREEN, x + 3, ((y + size) + 3) + offset, 10*size - 6, (22*size - 6) - offset);
		}
	}
	//level = 100 --> y offset 0
	//level = 0 --> y offset 22*size
}

int accel;
int temp;
int power;
char accelString[3];
char tempString[3];
char powerString[3];

void LCD_updateVals(SPI_HandleTypeDef* spi, int buf[]) {
	//buf[0:1] accel, buf[2:3] temp, buf[4:5] power
	accel = (buf[0] << 4) | buf[1];
	temp = (buf[2] << 4) | buf[3];
	power = (buf[4] << 4) | buf[5];
	itoa(accel,accelString,10);
	itoa(temp,tempString,10);
	itoa(power,powerString,10);
	LCD_drawString(spi,146,30 + 80*1,accelString,3,HX8357_BLACK,4);
	LCD_drawString(spi,146,30 + 80*2,tempString,3,HX8357_BLACK,4);
	LCD_drawString(spi,146,30 + 80*3,powerString,3,HX8357_BLACK,4);
}

//void LCD_warnings(SPI_HandleTypeDef* spi, int temp, int level, int *Twarning, int *Vwarning) {
//	if ((*Twarning == 0 & *Vwarning == 0) & ((temp > 50) | (level < 30))) {
//		LCD_writePixels(spi,HX8357_WHITE,0,0,359,79);
//	}
//	if (temp > 50 & *Twarning == 0) {
////		LCD_fillTriangle(spi,15,55,45,55,30,25,HX8357_GREEN);
//		LCD_drawString(spi,20,30 + 80*0,"HIGH TEMP",9,HX8357_RED,3);
//		*Twarning = 1;
//	}
//	if (level < 30 & *Vwarning == 0) {
////		LCD_fillTriangle(spi,180,55,210,55,195,25,HX8357_GREEN);
//		LCD_drawString(spi,198,30 + 80*0,"LOW VOLT",8,HX8357_RED,3);
//		*Vwarning = 1;
//	}
//	if (*Twarning == 1 & temp <= 50) {
//		LCD_writePixels(spi,HX8357_WHITE,0,0,195,79);
//		*Twarning = 0;
//	}
//	if (*Vwarning == 1 & level >= 30) {
//		LCD_writePixels(spi,HX8357_WHITE,196,0,163,79);
//		*Vwarning = 0;
//	}
//	if (*Twarning == 0 & *Vwarning == 0) {
//		LCD_drawString(spi,20,30 + 80*0,"TADAMHESPEV | UMSM",18,HX8357_BLACK,3);
//	}
//}

int tempWarn = 0;
int voltWarn = 0;

void LCD_warnings(SPI_HandleTypeDef* spi, int temp, int level, int *warning) {
	if ((temp >= 50 | level <= 30) & *warning == 0) {
		LCD_writePixels(spi,HX8357_WHITE,0,0,359,79);
		*warning = 1;
	}
	if (temp >= 50 & tempWarn == 0) {
		LCD_fillTriangle(spi,15,55,45,55,30,25,HX8357_GREEN);
		LCD_drawString(spi,55,30 + 80*0,"HIGH TEMP",9,HX8357_RED,2);
		tempWarn = 1;
	}
	if (level <= 30 & voltWarn == 0) {
		LCD_fillTriangle(spi,180,55,210,55,195,25,HX8357_GREEN);
		LCD_drawString(spi,220,30 + 80*0,"LOW VOLT",8,HX8357_RED,2);
		voltWarn = 1;
	}
	if (temp < 50 & tempWarn == 1) {
		LCD_writePixels(spi,HX8357_WHITE,0,0,179,79);
		tempWarn = 0;
	}
	if (level > 30 & voltWarn == 1) {
		LCD_writePixels(spi,HX8357_WHITE,180,0,179,79);
		voltWarn = 0;
	}
	if (*warning == 1 & voltWarn == 0 & tempWarn == 0) {
		LCD_writePixels(spi,HX8357_WHITE,0,0,359,79);
		LCD_drawString(spi,20,30 + 80*0,"TADAMHESPEV | UMSM",18,HX8357_BLACK,3);
		*warning = 0;
	}
}

void LCD_updateBattery(SPI_HandleTypeDef* spi, int level) {
	if (level < 0) level = 0;
	if (level > 100) level = 100;
	char lev[3];
	itoa(level,lev,10);

	LCD_fillBattery(spi,380,120,8,level);
	LCD_drawString(spi,370,50,lev,3,HX8357_BLACK,4);
}
