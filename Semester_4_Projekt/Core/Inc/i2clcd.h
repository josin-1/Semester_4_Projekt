/*
 * i2clcd.h
 *
 *  Created on: April 19, 2025
 *      Author: Sebastian Heide
 *
 *
 *
 * 	For I2C SERIAL 20X4 LCD MODULE with PCF8574 I2C adapter chip
 * 	PCF8574 I2C address is set by pins A2, A1, A0:
 * 	A2 A1 A0
 * 	0  0  0 -> 0x40
 * 	0  0  1 -> 0x42
 * 	0  1  0 -> 0x44
 * 	0  1  1 -> 0x46
 * 	1  0  0 -> 0x48
 * 	1  0  1 -> 0x4A
 * 	1  1  0 -> 0x4C
 * 	1  1  1 -> 0x4E
 *
 *  Capable of displaying German "Umlauts" ä, ö, ü and ß
 *
 */


#include "stm32f4xx_hal.h"
#include "stdbool.h"

#ifndef __I2CLCD_H
#define __I2CLCD_H
#define SLAVE_ADDRESS_LCD 0x4E //Slave address depends on PCF8574 address
#define LINES 4
#define COLUMS 20

uint8_t lcd_get_pos(uint8_t line, uint8_t column); // Dynamic calculation of the address

void lcd_init(void); // initialize lcd

void lcd_send_cmd(char cmd); // send command to the lcd

void lcd_send_data(char data); // send data to the lcd

void lcd_send_string(const char *str); // send string to the lcd

void lcd_write(const char *txt, uint8_t line, uint8_t column); // write string to specific line

void lcd_clear(void); //clear lcd

void lcd_clear_line(uint8_t line, uint8_t column); //clear line

__weak void lcd_i2c_error(void); //I2C Error

#endif // __I2CLCD_H
