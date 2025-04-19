/*
 * DS18B20.c
 *
 *  Created on: Mar 28, 2025
 *      Author: Johra-Markus Singh
 */

#include "DS18B20.h"

/**
* @brief DS18B20 lib Handle Struct Initialization
*
* This function initializes the handling struct for this lib.
*
* @param hds18b20: DS18B20_HandleTypedef pointer
* @param honew: OneWire_HandleTypedef pointer
*
* @retval None
*/
void DS18B20_init(DS18B20_HandleTypedef* hds18b20, OneWire_HandleTypedef* honew){
    hds18b20->honew = honew;
}

/**
* @brief Start temperature conversion
*
* Start a temperature conversion for a single sensor (rom in hds18b20 must be correct).
* (MatchRom - rom in hds18b20 must be correct)
*
* @param hds18b20: DS18B20_HandleTypedef pointer
*
* @retval None
*/
void DS18B20_ConvertT_single(DS18B20_HandleTypedef* hds18b20){
    OneWire_Reset(hds18b20->honew, &(hds18b20->presence_pulse));
    OneWire_MatchROM(hds18b20->honew, hds18b20->rom);
    OneWire_WriteByte(hds18b20->honew, DS18B20_CMD_CONVERT_T);
}

/**
* @brief Start temperature conversion for all DS18B20
*
* Start a temperature conversion for all sensors on the bus (SkipRom).
*
* @param hds18b20: DS18B20_HandleTypedef pointer
*
* @retval None
*/
void DS18B20_ConvertT_all(DS18B20_HandleTypedef* hds18b20){
    OneWire_Reset(hds18b20->honew, &(hds18b20->presence_pulse));
    OneWire_SkipROM(hds18b20->honew);
    OneWire_WriteByte(hds18b20->honew, DS18B20_CMD_CONVERT_T);
}

/**
* @brief Read Scratchpad
*
* Read the scratchpad from a single sensor.
* (MatchRom - rom in hds18b20 must be correct)
*
* @param hds18b20: DS18B20_HandleTypedef pointer
*
* @retval None
*/
void DS18B20_ReadScratchpad(DS18B20_HandleTypedef* hds18b20){
    OneWire_Reset(hds18b20->honew, &(hds18b20->presence_pulse));
    OneWire_MatchROM(hds18b20->honew, hds18b20->rom);
    OneWire_WriteByte(hds18b20->honew, DS18B20_CMD_RD_SCRATCHPAD);
    for(uint8_t i = 0; i < 9; ++i){
        OneWire_ReadByte(hds18b20->honew, &(hds18b20->scratchpad[i]));
    }
}

/**
* @brief Write Scratchpad
*
* Write the scratchpad (threshold temperatures and resolution)
* to a single sensor (rom in hds18b20 must be correct).
* (MatchRom - rom in hds18b20 must be correct)
*
* @param hds18b20: DS18B20_HandleTypedef pointer
*
* @retval None
*/
void DS18B20_WriteScratchpad(DS18B20_HandleTypedef* hds18b20){
    OneWire_Reset(hds18b20->honew, &(hds18b20->presence_pulse));
    OneWire_MatchROM(hds18b20->honew, hds18b20->rom);
    OneWire_WriteByte(hds18b20->honew, DS18B20_CMD_WR_SCRATCHPAD);
    for(uint8_t i = 2; i < 5; ++i){
        OneWire_WriteByte(hds18b20->honew, hds18b20->scratchpad[i]);
    }
}

/**
* @brief Write Scratchpad
*
* Write the scratchpad (threshold temperatures and resolution)
* to a single sensor (rom in hds18b20 must be correct).
* (MatchRom - rom in hds18b20 must be correct)
*
* @param hds18b20: DS18B20_HandleTypedef pointer
*
* @retval None
*/
void DS18B20_RecallEEPROM(DS18B20_HandleTypedef* hds18b20) {
    OneWire_Reset(hds18b20->honew, &(hds18b20->presence_pulse));
    OneWire_MatchROM(hds18b20->honew, hds18b20->rom);
    OneWire_WriteByte(hds18b20->honew, DS18B20_CMD_RECALL_EEPROM);
}

/**
* @brief Read ROM
*
* Read the rom from a single sensor and save it into hds18b20->rom.
* (SkipRom)
* Only one sensor can be present on the bus.
*
* @param hds18b20: DS18B20_HandleTypedef pointer
*
* @retval None
*/
void DS18B20_ReadROM(DS18B20_HandleTypedef* hds18b20){
    OneWire_Reset(hds18b20->honew, &(hds18b20->presence_pulse));
    OneWire_ReadROM(hds18b20->honew, hds18b20->rom);
}

/**
* @brief Get the temperature
*
* Read the scratchpad from the DS18B20 and convert the temperature to float.
*
* @param hds18b20: DS18B20_HandleTypedef pointer
*
* @retval temperature as float
*/
float DS18B20_getTemp(DS18B20_HandleTypedef* hds18b20){
//    DS18B20_ReadScratchpad(hds18b20);
    return (hds18b20->scratchpad[0] | (hds18b20->scratchpad[1] << 8)) / 16.0f;
}

/**
* @brief Set Threshold Temperatures
*
* Write new Threshold Temperature values onto the scratchpad of the sensor.
* If the meas. temperature gets below tLow or above tHigh the sensor will
* be present at a Alarm Search (still work in progress)
* (MatchRom - rom in hds18b20 must be correct)
*
* @param hds18b20: DS18B20_HandleTypedef pointer
* @param tLow: Low threshold temperature
* @param tHigh: Low threshold temperature
*
* @retval None
*/
void DS18B20_setThresholdT(DS18B20_HandleTypedef* hds18b20, int8_t tLow, int8_t tHigh){
    hds18b20->scratchpad[2] = tHigh;
    hds18b20->scratchpad[3] = tLow;

    DS18B20_WriteScratchpad(hds18b20);

    OneWire_Reset(hds18b20->honew, &(hds18b20->presence_pulse));
    OneWire_MatchROM(hds18b20->honew, hds18b20->rom);
    OneWire_WriteByte(hds18b20->honew, DS18B20_CMD_CP_SCRATCHPAD);
}

/**
* @brief Get Threshold Temperatures
*
* Read the scratchpad from the DS18B20, convert the threshold temperatures
* and save it into the given pointers.
*
* @param hds18b20: DS18B20_HandleTypedef pointer
* @param tLow: Pointer to where low threshold temp should be saved
* @param tHigh: Pointer to where high threshold temp should be saved
*
* @retval None
*/
void DS18B20_getThresholdT(DS18B20_HandleTypedef* hds18b20, int8_t* tHigh, int8_t* tLow){
//    DS18B20_ReadScratchpad(hds18b20);
    *tHigh = hds18b20->scratchpad[2];
    *tLow = hds18b20->scratchpad[3];
}

/**
* @brief Set Resolution
*
* Write a new resolution value onto the scratchpad of the sensor.
* (MatchRom - rom in hds18b20 must be correct)
*
* @param hds18b20: DS18B20_HandleTypedef pointer
* @param resolution: resolution to be set (enum DS18B20_Resolution)
*
* @retval None
*/
void DS18B20_setResolution(DS18B20_HandleTypedef* hds18b20, DS18B20_Resolution resolution){
    // see datasheet page 9
    hds18b20->scratchpad[4] &= 0b10011111;
    hds18b20->scratchpad[4] |= (resolution << 5);

    DS18B20_WriteScratchpad(hds18b20);

    OneWire_Reset(hds18b20->honew, &(hds18b20->presence_pulse));
    OneWire_MatchROM(hds18b20->honew, hds18b20->rom);
    OneWire_WriteByte(hds18b20->honew, DS18B20_CMD_CP_SCRATCHPAD);
}

/**
* @brief Get Current Resolution
*
* Read the scratchpad from the DS18B20, convert the resolution and return it.
*
* @param hds18b20: DS18B20_HandleTypedef pointer
*
* @retval Resolution as enum type
*/
DS18B20_Resolution DS18B20_getResolution(DS18B20_HandleTypedef* hds18b20){
//    DS18B20_ReadScratchpad(hds18b20);
    return (hds18b20->scratchpad[4] & 0b01100000) >> 5;
}
