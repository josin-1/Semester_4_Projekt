/*
 * DS18B20.h
 *
 *  Created on: Mar 28, 2025
 *      Author: johra
 */

#ifndef INC_DS18B20_H_
#define INC_DS18B20_H_

#include "stm32f4xx_hal.h"
#include "OneWire.h"

typedef enum {
    DS18B20_CMD_CONVERT_T     = 0x44,
    DS18B20_CMD_WR_SCRATCHPAD = 0x4E,
    DS18B20_CMD_RD_SCRATCHPAD = 0xBE,
    DS18B20_CMD_CP_SCRATCHPAD = 0x48,
    DS18B20_CMD_RECALL_EEPROM = 0xB8,
    DS18B20_CMD_READ_POW_SUPP = 0xB4,
} DS18B20_CMD;

typedef enum {
    DS18B20_RES_9Bit  = 0b00,
    DS18B20_RES_10Bit = 0b01,
    DS18B20_RES_11Bit = 0b10,
    DS18B20_RES_12Bit = 0b11,
} DS18B20_Resolution;

typedef struct {
    OneWire_HandleTypedef* honew;

    uint8_t rom[8];
    uint8_t scratchpad[9];

    float temp;
} DS18B20_HandleTypedef;


void DS18B20_init(DS18B20_HandleTypedef*, OneWire_HandleTypedef*);
void DS18B20_ReadScratchpad(DS18B20_HandleTypedef*);
void DS18B20_WriteScratchpad(DS18B20_HandleTypedef*);
void DS18B20_getTemp(DS18B20_HandleTypedef*, float*);
void DS18B20_setThresholdT(DS18B20_HandleTypedef*, uint8_t, uint8_t);
void DS18B20_setResolution(DS18B20_HandleTypedef*, DS18B20_Resolution);


#endif /* INC_DS18B20_H_ */
