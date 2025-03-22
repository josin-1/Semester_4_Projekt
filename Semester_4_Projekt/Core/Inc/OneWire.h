/*
 * OneWire.h
 *
 *  Created on: Mar 18, 2025
 *      Author: Johra-Markus Singh
 */

/** TODO
 *
 *  Bus Timings defines instead of Magic No.
 *
 *  NOTE:
 *  Ive deleted the whole search algorithm functions again, because they
 *  are not doable at this point, with the non-blocking nature of the libs
 */

#ifndef INC_ONEWIRE_FIRSTTESTS_H_
#define INC_ONEWIRE_FIRSTTESTS_H_

#include "stm32f4xx_hal.h"
#include <string.h>

#define ONEWIRE_BUS_QUEUE_SIZE 1024

// Bus Timings defines still missing... hardcoded into OneWire_BUS_x functions!

#define ONEWIRE_MAX_DEVICES 10

typedef enum {
    ONEWIRE_ATOMIC_WRITE      = 0,
    ONEWIRE_ATOMIC_READ       = 1,
    ONEWIRE_ATOMIC_READ_DONE  = 2,
    ONEWIRE_ATOMIC_RESET_DONE = 3
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

typedef enum {
    ONEWIRE_CMD_SearchROM   = 0xF0,
    ONEWIRE_CMD_ReadROM     = 0x33,
    ONEWIRE_CMD_MatchROM    = 0x55,
    ONEWIRE_CMD_SkipROM     = 0xCC,
    ONEWIRE_CMD_AlarmSearch = 0xEC,
} OneWire_ROM_CMD;

typedef struct {
    OneWire_BUS_Queue bus_queue;
    TIM_HandleTypeDef* htim;
    GPIO_TypeDef* gpio_port;
    uint16_t gpio_pin;

    uint8_t temp_byte_buffer[8];
    uint8_t received_byte;
} OneWire_HandleTypedef;

void OneWire_BUS_queue_init(OneWire_BUS_Queue*);
uint8_t OneWire_BUS_enqueue(OneWire_BUS_Queue*, OneWire_BUS_Instruction_Data*);
uint8_t OneWire_BUS_dequeue(OneWire_BUS_Queue*, OneWire_BUS_Instruction_Data*);
void OneWire_BUS_WriteBit(OneWire_HandleTypedef*, uint8_t);
void OneWire_BUS_ReadBit(OneWire_HandleTypedef*, uint8_t*);

void OneWire_init(OneWire_HandleTypedef*,  TIM_HandleTypeDef*, GPIO_TypeDef*, uint16_t);
void OneWire_WriteByte(OneWire_HandleTypedef*, uint8_t);
void OneWire_ReadByte(OneWire_HandleTypedef*, uint8_t*);
void OneWire_Reset(OneWire_HandleTypedef*, uint8_t*);
void OneWire_TIM_Hook(OneWire_HandleTypedef*);

void OneWire_ReadDoneCallback(OneWire_HandleTypedef*);
void OneWire_ResetDoneCallback(OneWire_HandleTypedef*);

void OneWire_ErrorHandler(OneWire_HandleTypedef*);



#endif /* INC_ONEWIRE_FIRSTTESTS_H_ */
