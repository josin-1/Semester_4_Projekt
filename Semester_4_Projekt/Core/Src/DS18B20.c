/*
 * DS18B20.c
 *
 *  Created on: Mar 28, 2025
 *      Author: johra
 */

#include "DS18B20.h"

/**
* @brief
*
* @param hds18b20:
*
* @retval None
*/
void DS18B20_init(DS18B20_HandleTypedef* hds18b20, OneWire_HandleTypedef* honew){
    hds18b20->honew = honew;
}

void DS18B20_ReadScratchpad(DS18B20_HandleTypedef* hds18b20){
    OneWire_WriteByte(hds18b20->honew, DS18B20_CMD_RD_SCRATCHPAD);
    for(uint8_t i = 0; i < 9; ++i){
        OneWire_ReadByte(hds18b20->honew, &(hds18b20->scratchpad[i]));
    }
}

void DS18B20_WriteScratchpad(DS18B20_HandleTypedef* hds18b20){
    OneWire_WriteByte(hds18b20->honew, DS18B20_CMD_WR_SCRATCHPAD);
    for(uint8_t i = 2; i < 5; ++i){
        OneWire_WriteByte(hds18b20->honew, hds18b20->scratchpad[i]);
    }
}

void DS18B20_getTemp(DS18B20_HandleTypedef* hds18b20, float* temp_ptr){
    if (temp_ptr == NULL) return;
    *temp_ptr = hds18b20->temp;
}

void DS18B20_setThresholdT(DS18B20_HandleTypedef* hds18b20, uint8_t tLow, uint8_t tHigh){
    hds18b20->scratchpad[2] = tHigh;
    hds18b20->scratchpad[3] = tLow;
    DS18B20_WriteScratchpad(hds18b20);
    OneWire_WriteByte(hds18b20->honew, DS18B20_CMD_CP_SCRATCHPAD);
}

void DS18B20_setResolution(DS18B20_HandleTypedef* hds18b20, DS18B20_Resolution resolution){
    hds18b20->scratchpad[4] = (resolution << 5); // see datasheet page 9
    DS18B20_WriteScratchpad(hds18b20);
    OneWire_WriteByte(hds18b20->honew, DS18B20_CMD_CP_SCRATCHPAD);
}
