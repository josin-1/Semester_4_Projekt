/*
 * i2clcd.h
 *
 *  Created on: April 19, 2025
 *      Author: Sebastian Heide
 *
 *
 *
 * 	For I2C SERIAL 20X4 LCD MODULE with PCF8574 I2C adapter chip
 *  Capable of displaying German "Umlauts" ä, ö, ü and ß
 */

#ifndef __I2CLCD_H
#define __I2CLCD_H

#include "stm32f4xx_hal.h"

uint8_t lcd_get_pos(uint8_t line, uint8_t column); // Dynamic calculation of the address

void lcd_init(void); // initialize lcd

void lcd_send_cmd(char cmd); // send command to the lcd

void lcd_send_data(char data); // send data to the lcd

void lcd_send_string(const char *str); // send string to the lcd

void lcd_write(const char *txt, uint8_t line, uint8_t column); // write string to specific line

void lcd_clear(void); //clear lcd

void lcd_clear_line(uint8_t line, uint8_t column); //clear line

#endif /* __I2CLCD_H */
