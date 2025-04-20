/*
 * i2clcd.c
 *
 *  Created on: April 19, 2025
 *      Author: Sebastian Heide
 */

#include "i2clcd.h"

extern I2C_HandleTypeDef hi2c1;

uint8_t lcd_get_pos(uint8_t line, uint8_t column)
{
    static const uint8_t line_offset[] = {0x00, 0x40, 0x14, 0x54}; // Start address of each line
    return line_offset[line] + column; // Dynamic calculation of the address
}

void lcd_send_cmd(char cmd)	//control commands
{
	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (cmd & 0xf0); //extract upper 4 bits
	data_l = ((cmd << 4) & 0xf0); //extract lower 4 bits
	data_t[0] = data_u | 0x0C; //en=1, rs=0
	data_t[1] = data_u | 0x08; //en=0, rs=0
	data_t[2] = data_l | 0x0C; //en=1, rs=0
	data_t[3] = data_l | 0x08; //en=0, rs=0
	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, SLAVE_ADDRESS_LCD, (uint8_t *)data_t, 4, 100);
	//The LCD receives an 8-bit command, which is sent in two 4-bit nibbles.
	if (status == HAL_ERROR)
	{
		lcd_i2c_error();
	}
}

void lcd_send_data(char data) //drawing commands
{
	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (data & 0xf0); //extract upper 4 bits
	data_l = ((data << 4) & 0xf0); //extract lower 4 bits
	data_t[0] = data_u | 0x0D; //en=1, rs=1
	data_t[1] = data_u | 0x09; //en=0, rs=1
	data_t[2] = data_l | 0x0D; //en=1, rs=1
	data_t[3] = data_l | 0x09; //en=0, rs=1
	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, SLAVE_ADDRESS_LCD, (uint8_t *)data_t, 4, 100);
	//The LCD receives an 8-bit command, which is sent in two 4-bit nibbles.
	if (status == HAL_ERROR)
		{
			lcd_i2c_error();
		}
}

__weak void lcd_i2c_error(void)
{
    __NOP();//No Operation
}

void lcd_clear(void)
{
    lcd_send_cmd(0x01);  // Clear Display
    HAL_Delay(1);        // wait 1ms
}

void lcd_init(void)
{
	// 4 bit initialisation
	HAL_Delay(41); // wait for more than 40ms (Vcc rises)
	lcd_send_cmd(0x30);
	HAL_Delay(4.5); // wait for more than 4.1ms
	lcd_send_cmd(0x30);
	HAL_Delay(1); // wait for more than 100us
	lcd_send_cmd(0x30);
	HAL_Delay(10);
	lcd_send_cmd(0x20); // 4bit mode
	HAL_Delay(10);

	// display initialisation
	lcd_send_cmd(0x28); // Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters)
	HAL_Delay(1);
	lcd_send_cmd(0x08); //Display on/off control --> D=0,C=0, B=0  ---> display off
	HAL_Delay(1);
	lcd_send_cmd(0x01); // clear display
	HAL_Delay(2);
	lcd_send_cmd(0x06); //Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
	HAL_Delay(1);
	lcd_send_cmd(0x0C); //Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits)
}


void lcd_send_string(const char *str)
{
    static const struct {
        uint8_t utf8;
        uint8_t lcd;
    } char_map[] = {
        {0xA4, 0xE1}, // ä -> LCD-Code für ä
        {0xB6, 0xEF}, // ö -> LCD-Code für ö
        {0xBC, 0xF5}, // ü -> LCD-Code für ü
        {0x9F, 0xE2}  // ß -> LCD-Code für ß
    };

    while (*str)
    {
        if ((uint8_t)*str == 0xC3) // UTF-8 Multibyte-Präfix
        {
            str++; // Check next character
            bool found = false;

            for (int i = 0; i < sizeof(char_map) / sizeof(char_map[0]); i++)
            {
                if ((uint8_t)*str == char_map[i].utf8)
                {
                    lcd_send_data(char_map[i].lcd);
                    str++; // Skips the character
                    found = true;
                    break; // Exit loop because character was replaced
                }
            }

            if (!found) // If no replacement character was found, send original character
            {
                lcd_send_data(*str);
                str++;
            }
        }
        else
        {
            lcd_send_data(*str);
            str++;
        }
    }
}

void lcd_write(const char *txt, uint8_t line, uint8_t column)
{
	lcd_clear_line(line, column);
	lcd_send_cmd(0x80 | lcd_get_pos(line, column)); // Cursor set
	lcd_send_string(txt);
}

void lcd_clear_line(uint8_t line, uint8_t column)
{
	lcd_send_cmd(0x80 | lcd_get_pos(line, column)); // Cursor set
	for (int i = 0; i < (COLUMS - column); i++)
	{
		lcd_send_data(' ');
	}
}
