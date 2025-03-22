/*
 * OneWire.c
 *
 *  Created on: Mar 18, 2025
 *      Author: Johra-Markus Singh
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
    __DMB(); // Data Memory Barrier; Ensure memory update before updating the pointer
    queue->begin = next_begin;    // Atomically move tail forward

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
    //__DMB();  // Dont need DMB, if ISR running with highest prio!
    queue->end = (queue->end + 1) % ONEWIRE_BUS_QUEUE_SIZE;

    return 1;
}

/**
* @brief Write a single bit onto the bus
*
* Pushes the needed instructions to write a 0 or 1, onto the queue
*
* @param honew: OneWire_HandleTypedef pointer
* @param data: Data to be written (0/1)
*
* @retval none
*/
void OneWire_BUS_WriteBit(OneWire_HandleTypedef* honew, uint8_t data){
    OneWire_BUS_Instruction_Data instrData;

    if (data > 1) // only 1 and 0 expected
        return;

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_RESET;
    if (data == 1){
        instrData.wait_time = 100;
    }
    else {
        instrData.wait_time = 5900;
    }
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_SET;
    if (data == 1){
        instrData.wait_time = 5900;
    }
    else {
        instrData.wait_time = 100;
    }
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
void OneWire_BUS_ReadBit(OneWire_HandleTypedef* honew, uint8_t* data_ptr){
    OneWire_BUS_Instruction_Data instrData;

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_RESET;
    instrData.wait_time = 100;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_SET;
    instrData.wait_time = 1400;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    instrData.instr = ONEWIRE_ATOMIC_READ;
    instrData.data = data_ptr;
    instrData.wait_time = 4500;
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
        OneWire_BUS_WriteBit(honew, (data >> i) & 1);
    }
}


/**
* @brief Read a full Byte from the bus
*
* Pushes the instructions for reading a Byte onto the queue
*
* @param honew: Handle Struct pointer
* @param data: Pointer to save received data
*
* @retval None
*/
void OneWire_ReadByte(OneWire_HandleTypedef* honew, uint8_t* data_ptr){
    OneWire_BUS_Instruction_Data instrData;

    for (size_t i = 0; i < 8; ++i) {
        OneWire_BUS_ReadBit(honew, &(honew->temp_byte_buffer[i]));
    }

    instrData.instr = ONEWIRE_ATOMIC_READ_DONE;
    instrData.data = data_ptr;
    instrData.wait_time = 1000;
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);
}

/**
* @brief Perform a reset pulse and read out presence
*
* Pushes the instructions for writing a reset pulse onto the queue
* and reads the presence pulse if there is one
*
* @param honew: Handle Struct pointer
* @param data: Data Pointer for presence pulse read
*
* @retval
*/
void OneWire_Reset(OneWire_HandleTypedef* honew, uint8_t* data_ptr){
    OneWire_BUS_Instruction_Data instrData;

        instrData.instr = ONEWIRE_ATOMIC_WRITE;
        instrData.data = (uint8_t*)GPIO_PIN_RESET;
        instrData.wait_time = 48000;
        OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

        instrData.instr = ONEWIRE_ATOMIC_WRITE;
        instrData.data = (uint8_t*)GPIO_PIN_SET;
        instrData.wait_time = 10000;
        OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

        instrData.instr = ONEWIRE_ATOMIC_READ;
        instrData.data = data_ptr;
        instrData.wait_time = 48000 - 10000;
        OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

        instrData.instr = ONEWIRE_ATOMIC_RESET_DONE;
        instrData.wait_time = 1000;
        OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

        if(honew->htim->State == HAL_TIM_STATE_READY){
            HAL_TIM_Base_Start_IT(honew->htim);
        }
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
//        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_2);
        HAL_GPIO_WritePin(honew->gpio_port, honew->gpio_pin, (GPIO_PinState)(data.data));
//        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_2);
        break;
    case ONEWIRE_ATOMIC_READ:
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_2);
        *(data.data) = HAL_GPIO_ReadPin(honew->gpio_port, honew->gpio_pin);
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_2);
        break;
    case ONEWIRE_ATOMIC_READ_DONE:
        *(data.data) = 0; // Reset stored byte
        for (size_t i = 0; i < 8; i++) {
            *(data.data) |= (honew->temp_byte_buffer[i] << i);
        }
        OneWire_ReadDoneCallback(honew);
        break;
    case ONEWIRE_ATOMIC_RESET_DONE:
        OneWire_ResetDoneCallback(honew);
        break;
    }
    __HAL_TIM_CLEAR_FLAG(honew->htim, TIM_FLAG_UPDATE); // Clear any pending interrupt
    honew->htim->Instance->ARR = data.wait_time;
    honew->htim->Instance->CNT = 0;
    if (honew->bus_queue.begin != honew->bus_queue.end) // if queue is not empty start tim
        HAL_TIM_Base_Start_IT(honew->htim);
}

__weak void OneWire_ReadDoneCallback(OneWire_HandleTypedef* honew){
    __NOP();
}

__weak void OneWire_ResetDoneCallback(OneWire_HandleTypedef* honew){
    __NOP();
}

void OneWire_ErrorHandler(OneWire_HandleTypedef* honew){
    __NOP();
}

