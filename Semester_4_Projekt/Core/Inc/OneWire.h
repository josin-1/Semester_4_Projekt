/*
 * OneWire.h
 *
 *  Created on: Mar 18, 2025
 *      Author: Johra-Markus Singh
 */

#ifndef INC_ONEWIRE_FIRSTTESTS_H_
#define INC_ONEWIRE_FIRSTTESTS_H_

#include "stm32f4xx_hal.h"
#include <string.h>

#define ONEWIRE_BUS_QUEUE_SIZE 1024

#define ONEWIRE_MAX_DEVICES 2

#define ONEWIRE_TIMINGS_WRITE0_LOW      100
#define ONEWIRE_TIMINGS_WRITE0_HIGH     5900
#define ONEWIRE_TIMINGS_WRITE1_LOW      5900
#define ONEWIRE_TIMINGS_WRITE1_HIGH     100
#define ONEWIRE_TIMINGS_READBIT_LOW     100
#define ONEWIRE_TIMINGS_READBIT_HIGH    1400
#define ONEWIRE_TIMINGS_READBIT_HOLD    4500
#define ONEWIRE_TIMINGS_RESET_LOW       48000
#define ONEWIRE_TIMINGS_RESET_HIGH      10000
#define ONEWIRE_TIMINGS_RESET_HOLD      (48000 - 10000)
#define ONEWIRE_TIMINGS_RESET_REC       1000
#define ONEWIRE_TIMINGS_READ_DONE       1000


typedef enum {
    ONEWIRE_ERROR_NONE = 0,
    ONEWIRE_ERROR_WRITE_BADDATA,
    ONEWIRE_ERROR_READ_NULLPTR,
    ONEWIRE_ERROR_QUEUE_FULL,
    ONEWIRE_ERROR_QUEUE_EMPTY,
    ONEWIRE_ERROR_SEARCH_NODEV,
} OneWire_ErrorTable;

typedef enum {
    ONEWIRE_ATOMIC_WRITE = 0,
    ONEWIRE_ATOMIC_READ,
    ONEWIRE_ATOMIC_READ_BYTE_DONE,
    ONEWIRE_ATOMIC_READ_ROM_DONE,
    ONEWIRE_ATOMIC_RESET_DONE,
} OneWire_BUS_InstructionSet ;

typedef struct {
    OneWire_BUS_InstructionSet instr;
    uint8_t* data;
    uint32_t wait_time;
} OneWire_BUS_InstructionData;

typedef struct {
    OneWire_BUS_InstructionData element[ONEWIRE_BUS_QUEUE_SIZE];
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

// SearchROM WIP
//typedef enum {
//    ONEWIRE_SEARCHSTATE_IDLE = 0,
//    ONEWIRE_SEARCHSTATE_RESET,
//    ONEWIRE_SEARCHSTATE_SEND_CMD,
//    ONEWIRE_SEARCHSTATE_READ_BIT,
//    ONEWIRE_SEARCHSTATE_DONE,
//} OneWire_SearchState;

//typedef struct {
//    uint8_t search_state;
//    uint8_t last_discrepancy;
//    uint8_t search_bit_index;
//    uint8_t id_bit;
//    uint8_t comp_id_bit;
//    uint8_t search_direction;
//    uint8_t presence_pulse;
//} OneWire_SearchTypedef;

typedef struct {
    OneWire_BUS_Queue bus_queue;
    TIM_HandleTypeDef* htim;
    GPIO_TypeDef* gpio_port;
    uint16_t gpio_pin;

    uint8_t temp_byte_buffer[8];
    uint8_t readROM_buffer[8];

//    OneWire_SearchTypedef search;
//    uint8_t ROM_buffer[8][ONEWIRE_MAX_DEVICES];
    uint8_t num_devices;

    OneWire_ErrorTable errno;
} OneWire_HandleTypedef;

void OneWire_BUS_queue_init(OneWire_BUS_Queue*);
uint8_t OneWire_BUS_enqueue(OneWire_BUS_Queue*, OneWire_BUS_InstructionData*);
uint8_t OneWire_BUS_dequeue(OneWire_BUS_Queue*, OneWire_BUS_InstructionData*);
void OneWire_BUS_WriteBit(OneWire_HandleTypedef*, uint8_t);
void OneWire_BUS_ReadBit(OneWire_HandleTypedef*, uint8_t*);

void OneWire_init(OneWire_HandleTypedef*,  TIM_HandleTypeDef*, GPIO_TypeDef*, uint16_t);
void OneWire_WriteByte(OneWire_HandleTypedef*, uint8_t);
void OneWire_ReadByte(OneWire_HandleTypedef*, uint8_t*);
void OneWire_Reset(OneWire_HandleTypedef*, uint8_t*);
void OneWire_ReadROM(OneWire_HandleTypedef*);
void OneWire_SkipROM(OneWire_HandleTypedef*);
void OneWire_MatchROM(OneWire_HandleTypedef*, uint8_t[8]);

// SearchROM WIP
//void OneWire_SearchROM(OneWire_HandleTypedef*);

void OneWire_TIM_Hook(OneWire_HandleTypedef*);

void OneWire_ReadDoneCallback(OneWire_HandleTypedef*);
void OneWire_ResetDoneCallback(OneWire_HandleTypedef*);
void OneWire_ReadROMCallback(OneWire_HandleTypedef*);
void OneWire_SearchROMCallback(OneWire_HandleTypedef*);

void OneWire_SearchErrorHandler(OneWire_HandleTypedef*);
void OneWire_ErrorHandler(OneWire_HandleTypedef*);



#endif /* INC_ONEWIRE_FIRSTTESTS_H_ */
