/*
 * OneWire.c
 *
 *  Created on: Mar 18, 2025
 *      Author: johra
 */

#include "OneWire.h"

/**
* @brief Bus queue Initialization
*
* This function initializes the queue for the bus instruction
*
* @param queue: OneWire_BUS_Queue pointer
*
* @retval None
*/
void OneWire_BUS_queue_init(OneWire_BUS_Queue* queue) {
    queue->begin = 0;
    queue->end = 0;
    queue->current_load = 0;
    memset(&queue->element[0], 0, ONEWIRE_BUS_QUEUE_SIZE * sizeof(OneWire_BUS_Instruction_Data));
}

/**
* @brief Push new element on queue
*
* Pushes a new element on the queue
*
* @param queue: OneWire_BUS_Queue pointer
* @param data: data to be pushed
*
* @retval 0 if push failed (Queue full), 1 if successful
*/
uint8_t OneWire_BUS_enqueue(OneWire_BUS_Queue* queue, OneWire_BUS_Instruction_Data* data){
    uint32_t next_begin = (queue->begin + 1) % ONEWIRE_BUS_QUEUE_SIZE;
    if (next_begin == queue->end) return 0;  // Queue full


    queue->element[queue->begin] = *data;
    __DMB();             // Ensure memory update before updating the pointer
    queue->begin = next_begin;    // Atomically move tail forward
    queue->current_load++;

    return 1;  // Successfully enqueued
}

/**
* @brief Pull element from queue
*
* Pulls element from the queue and writes it into data
*
* @param queue: OneWire_BUS_Queue pointer
* @param data: data to retrieve element
*
* @retval 0 if pull failed (Queue empty), 1 if successful
*/
uint8_t OneWire_BUS_dequeue(OneWire_BUS_Queue* queue, OneWire_BUS_Instruction_Data* data){
    if (queue->begin == queue->end) return 0;  // Queue empty

    *data = queue->element[queue->end];
    __DMB();  // Ensure memory ordering before moving head
    queue->end = (queue->end + 1) % ONEWIRE_BUS_QUEUE_SIZE;
    queue->current_load--;

    return 1;
}

/**
* @brief Write a 1 bit onto the bus
*
* Pushes the needed instructions for a 1 on the bus, onto the queue
*
* @param honew: OneWire_HandleTypedef pointer
*
* @retval none
*/
void OneWire_Write1(OneWire_HandleTypedef* honew){
    OneWire_BUS_Instruction_Data instrData;

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_RESET;
    instrData.wait_time = 1;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_SET;
    instrData.wait_time = 60;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    instrData.instr = ONEWIRE_ATOMIC_DONE;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    if(honew->htim->State == HAL_TIM_STATE_READY){
        HAL_TIM_Base_Start_IT(honew->htim);
    }
}

/**
* @brief Write a 0 bit onto the bus
*
* Pushes the needed instructions for a 0 on the bus, onto the queue
*
* @param honew: OneWire_HandleTypedef pointer
*
* @retval none
*/
void OneWire_Write0(OneWire_HandleTypedef* honew){
    OneWire_BUS_Instruction_Data instrData;

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_RESET;
    instrData.wait_time = 60;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_SET;
    instrData.wait_time = 1;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    instrData.instr = ONEWIRE_ATOMIC_DONE;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    if(honew->htim->State == HAL_TIM_STATE_READY){
        HAL_TIM_Base_Start_IT(honew->htim);
    }
}

/**
* @brief Read a single bit from the bus
*
* Pushes the needed instructions to read a bit from the bus, onto the queue
*
* @param honew: OneWire_HandleTypedef pointer
* @param data_ptr: pointer where read bit should be saved to
*
* @retval none
*/
void OneWire_Read(OneWire_HandleTypedef* honew, uint8_t* data_ptr){
    OneWire_BUS_Instruction_Data instrData;

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_RESET;
    instrData.wait_time = 1;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_SET;
    instrData.wait_time = 14;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    instrData.instr = ONEWIRE_ATOMIC_READ;
    instrData.data = data_ptr;
    instrData.wait_time = 45;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    instrData.instr = ONEWIRE_ATOMIC_DONE;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    if(honew->htim->State == HAL_TIM_STATE_READY){
        HAL_TIM_Base_Start_IT(honew->htim);
    }
}

/**
* @brief OneWire lib Handle Struct Initialization
*
* This function initializes the handling struct for this lib
*
* @param honew: Handle Struct pointer
* @param htim: TIM Handle pointer, to interact with Timer
* @param port: GPIO Port of bus pin (Pin is to be defined as Open Drain)
* @param pin: GPIO Pin of bus pin (Pin is to be defined as Open Drain)
*
* @retval None
*/
void OneWire_init(OneWire_HandleTypedef* honew, TIM_HandleTypeDef* htim, GPIO_TypeDef* port, uint16_t pin){
    OneWire_BUS_queue_init(&(honew->bus_queue));
    honew->htim = htim;
    honew->gpio_port = port;
    honew->gpio_pin = pin;
}

/**
* @brief Write a full Byte on the bus
*
* Pushes the instructions for writing a whole Byte onto the queue
*
* @param honew: Handle Struct pointer
* @param data: Byte to be transmitted
*
* @retval None
*/
void OneWire_WriteByte(OneWire_HandleTypedef* honew, uint8_t data){
    for(size_t i = 0; i < 8; ++i){
        if((data >> i) & 1)
            OneWire_Write1(honew);
        else
            OneWire_Write0(honew);
    }
}

void OneWire_ReadByte(OneWire_HandleTypedef* honew, uint8_t* data_ptr){
    // Coming soon...
}

/**
* @brief Timer Interrupt Hook Function
*
* This function needs to be in the PeriodElapsedCallback Function of the configured TIM.
* It handles all the Low-Level Bus Instructions from queue
*
* @param honew: Handle Struct pointer
*
* @retval None
*/
void OneWire_TIM_Hook(OneWire_HandleTypedef* honew){
    HAL_TIM_Base_Stop_IT(honew->htim);

    OneWire_BUS_Instruction_Data data;
    if (!OneWire_BUS_dequeue(&(honew->bus_queue), &data))
        return;

    switch(data.instr){
    case ONEWIRE_ATOMIC_WRITE:
        HAL_GPIO_WritePin(honew->gpio_port, honew->gpio_pin, (GPIO_PinState)(data.data));
        break;
    case ONEWIRE_ATOMIC_READ:
        *(data.data) = HAL_GPIO_ReadPin(honew->gpio_port, honew->gpio_pin);
        break;
    case ONEWIRE_ATOMIC_DONE:

        break;
    }
    __HAL_TIM_CLEAR_FLAG(honew->htim, TIM_FLAG_UPDATE); // Clear any pending interrupt
    if (honew->bus_queue.current_load == 0)
        honew->htim->Instance->ARR = UINT16_MAX;
    else
        honew->htim->Instance->ARR = data.wait_time;
    honew->htim->Instance->CNT = 0;
    __disable_irq();
    HAL_TIM_Base_Start_IT(honew->htim);
    __enable_irq();
}
