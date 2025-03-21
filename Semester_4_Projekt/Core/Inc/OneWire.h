/*
 * OneWire.h
 *
 *  Created on: Mar 18, 2025
 *      Author: johra
 */

/**
 * Functions ...
 * been taken from "https://www.analog.com/en/resources/app-notes/1wire-search-algorithm.html",
 * and modified.
 */



/** TODO
 *
 *  Implement ReadByte()
 *  Implement Reset()
 *  Bus Timings defines instead of Magic No.
 *  CRC8 calculation/lookup
 */

#ifndef INC_ONEWIRE_FIRSTTESTS_H_
#define INC_ONEWIRE_FIRSTTESTS_H_

#include "stm32f4xx_hal.h"
#include <string.h>

// There is a great formula to calculate the min queue size, which this margin is to narrow to explain
#define ONEWIRE_BUS_QUEUE_SIZE 1024

// Bus Timings defines still missing... hardcoded into OneWire_BUS_x functions!

#define ONEWIRE_MAX_DEVICES 69

typedef enum {
    ONEWIRE_ATOMIC_WRITE    = 0,
    ONEWIRE_ATOMIC_READ     = 1,
    ONEWIRE_ATOMIC_DONE     = 2
} OneWire_BUS_InstructionSet ;

typedef struct {
    OneWire_BUS_InstructionSet instr;
    uint8_t* data;
    uint32_t wait_time;
} OneWire_BUS_Instruction_Data;

typedef struct {
    OneWire_BUS_Instruction_Data element[ONEWIRE_BUS_QUEUE_SIZE];
    uint32_t begin;
    uint32_t end;
} OneWire_BUS_Queue;

typedef struct {
    OneWire_BUS_Queue bus_queue;
    TIM_HandleTypeDef* htim;
    GPIO_TypeDef* gpio_port;
    uint16_t gpio_pin;

    uint8_t ROM_NO[ONEWIRE_MAX_DEVICES];
    int32_t LastDiscrepancy;
    int32_t LastFamilyDiscrepancy;
    int32_t LastDeviceFlag;
} OneWire_HandleTypedef;

void OneWire_BUS_queue_init(OneWire_BUS_Queue*);
uint8_t OneWire_BUS_enqueue(OneWire_BUS_Queue*, OneWire_BUS_Instruction_Data*);
uint8_t OneWire_BUS_dequeue(OneWire_BUS_Queue*, OneWire_BUS_Instruction_Data*);
void OneWire_BUS_WriteBit(OneWire_HandleTypedef*, uint8_t);
void OneWire_BUS_ReadBit(OneWire_HandleTypedef*, uint8_t*);

void OneWire_init(OneWire_HandleTypedef*,  TIM_HandleTypeDef*, GPIO_TypeDef*, uint16_t);
void OneWire_WriteByte(OneWire_HandleTypedef*, uint8_t);
void OneWire_ReadByte(OneWire_HandleTypedef*,uint8_t*);
void OneWire_TIM_Hook(OneWire_HandleTypedef*);


// modified functions from Analog Devices
uint8_t OneWire_First(OneWire_HandleTypedef*);
uint8_t OneWire_Next(OneWire_HandleTypedef*);
uint8_t  OneWire_Verify(OneWire_HandleTypedef*);
void OneWire_TargetSetup(OneWire_HandleTypedef*, uint8_t family_code);
void OneWire_FamilySkipSetup(OneWire_HandleTypedef*);
uint8_t  OneWire_Reset(OneWire_HandleTypedef*);
uint8_t  OneWire_Search(OneWire_HandleTypedef*);

#endif /* INC_ONEWIRE_FIRSTTESTS_H_ */
