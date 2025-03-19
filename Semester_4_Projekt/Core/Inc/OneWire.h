/*
 * OneWire.h
 *
 *  Created on: Mar 18, 2025
 *      Author: johra
 */

#ifndef INC_ONEWIRE_FIRSTTESTS_H_
#define INC_ONEWIRE_FIRSTTESTS_H_

#include "stm32f4xx_hal.h"
#include <string.h>

#define ONEWIRE_BUS_QUEUE_SIZE 1024

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
} OneWire_HandleTypedef;

void OneWire_BUS_queue_init(OneWire_BUS_Queue*);
uint8_t OneWire_BUS_enqueue(OneWire_BUS_Queue*, OneWire_BUS_Instruction_Data*);
uint8_t OneWire_BUS_dequeue(OneWire_BUS_Queue*, OneWire_BUS_Instruction_Data*);

void OneWire_Write1(OneWire_HandleTypedef*);
void OneWire_Write0(OneWire_HandleTypedef*);
void OneWire_Read(OneWire_HandleTypedef*, uint8_t*);

void OneWire_init(OneWire_HandleTypedef*,  TIM_HandleTypeDef*, GPIO_TypeDef*, uint16_t);
void OneWire_WriteByte(OneWire_HandleTypedef*, uint8_t);
void OneWire_ReadByte(OneWire_HandleTypedef*,uint8_t*);
void OneWire_TIM_Hook(OneWire_HandleTypedef*);


#endif /* INC_ONEWIRE_FIRSTTESTS_H_ */
