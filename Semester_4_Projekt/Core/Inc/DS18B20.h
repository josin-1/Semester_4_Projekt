/*
 * DS18B20.h
 *
 *  Created on: Mar 28, 2025
 *      Author: Johra-Markus Singh
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


/* --- Max Conversion Time ---
 * +------------+-----------+
 * | Resolution | Time [ms] |
 * +------------+-----------+
 * |    9 Bit   |   93.75   |
 * |   10 Bit   |  187.75   |
 * |   11 Bit   |    375    |
 * |   12 Bit   |    750    |
 * +------------+-----------+
 */
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
    uint8_t presence_pulse;
} DS18B20_HandleTypedef;


void DS18B20_init(DS18B20_HandleTypedef*, OneWire_HandleTypedef*);

void DS18B20_ConvertT_single(DS18B20_HandleTypedef*);
void DS18B20_ConvertT_all(DS18B20_HandleTypedef*);
void DS18B20_ReadScratchpad(DS18B20_HandleTypedef*);
void DS18B20_WriteScratchpad(DS18B20_HandleTypedef*);
void DS18B20_RecallEEPROM(DS18B20_HandleTypedef*);
void DS18B20_ReadROM(DS18B20_HandleTypedef*);

float DS18B20_getTemp(DS18B20_HandleTypedef*);

void DS18B20_setThresholdT(DS18B20_HandleTypedef*, int8_t, int8_t);
void DS18B20_getThresholdT(DS18B20_HandleTypedef*, int8_t*, int8_t*);

void DS18B20_setResolution(DS18B20_HandleTypedef*, DS18B20_Resolution);
DS18B20_Resolution DS18B20_getResolution(DS18B20_HandleTypedef*);


#endif /* INC_DS18B20_H_ */
