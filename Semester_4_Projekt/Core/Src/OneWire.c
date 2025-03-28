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
    memset(&queue->element[0], 0, ONEWIRE_BUS_QUEUE_SIZE * sizeof(OneWire_BUS_InstructionData));
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
uint8_t OneWire_BUS_enqueue(OneWire_BUS_Queue* queue, OneWire_BUS_InstructionData* data){
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
uint8_t OneWire_BUS_dequeue(OneWire_BUS_Queue* queue, OneWire_BUS_InstructionData* data){
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
    OneWire_BUS_InstructionData instrData;

    if (data > 1){ // only 1 and 0 expected
        honew->errno = ONEWIRE_ERROR_WRITE_BADDATA;
        OneWire_ErrorHandler(honew);
    }

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_RESET;
    instrData.wait_time = (data ? ONEWIRE_TIMINGS_WRITE1_LOW : ONEWIRE_TIMINGS_WRITE0_LOW);
    if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
        honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
        OneWire_ErrorHandler(honew);
    }

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_SET;
    instrData.wait_time = (data ? ONEWIRE_TIMINGS_WRITE1_HIGH : ONEWIRE_TIMINGS_WRITE0_HIGH);
    if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
        honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
        OneWire_ErrorHandler(honew);
    }

    if(honew->htim->State == HAL_TIM_STATE_READY){ // if TIM aint running, start it
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
    OneWire_BUS_InstructionData instrData;

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_RESET;
    instrData.wait_time = ONEWIRE_TIMINGS_READBIT_LOW;
    if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
        honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
        OneWire_ErrorHandler(honew);
    }

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    instrData.data = (uint8_t*)GPIO_PIN_SET;
    instrData.wait_time = ONEWIRE_TIMINGS_READBIT_HIGH;
    if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
        honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
        OneWire_ErrorHandler(honew);
    }

    instrData.instr = ONEWIRE_ATOMIC_READ;
    instrData.data = data_ptr;
    instrData.wait_time = ONEWIRE_TIMINGS_READBIT_HOLD;
    if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
        honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
        OneWire_ErrorHandler(honew);
    }

    if(honew->htim->State == HAL_TIM_STATE_READY){ // if TIM aint running, start it
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

    honew->search.search_state = ONEWIRE_SEARCHSTATE_IDLE;

    honew->num_devices = 0;

    honew->errno = ONEWIRE_ERROR_NONE;
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
    OneWire_BUS_InstructionData instrData;

    for (size_t i = 0; i < 8; ++i) {
        OneWire_BUS_ReadBit(honew, &(honew->temp_byte_buffer[i]));
    }

    instrData.instr = ONEWIRE_ATOMIC_READ_BYTE_DONE;
    instrData.data = data_ptr;
    instrData.wait_time = ONEWIRE_TIMINGS_READ_DONE;

    if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
        honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
        OneWire_ErrorHandler(honew);
    }
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
    OneWire_BUS_InstructionData instrData;

        instrData.instr = ONEWIRE_ATOMIC_WRITE;
        instrData.data = (uint8_t*)GPIO_PIN_RESET;
        instrData.wait_time = ONEWIRE_TIMINGS_RESET_LOW;
        if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
            honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
            OneWire_ErrorHandler(honew);
        }

        instrData.instr = ONEWIRE_ATOMIC_WRITE;
        instrData.data = (uint8_t*)GPIO_PIN_SET;
        instrData.wait_time = ONEWIRE_TIMINGS_RESET_HIGH;
        if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
            honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
            OneWire_ErrorHandler(honew);
        }

        instrData.instr = ONEWIRE_ATOMIC_READ;
        instrData.data = data_ptr;
        instrData.wait_time = ONEWIRE_TIMINGS_RESET_HOLD;
        if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
            honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
            OneWire_ErrorHandler(honew);
        }

        instrData.instr = ONEWIRE_ATOMIC_RESET_DONE;
        instrData.wait_time = ONEWIRE_TIMINGS_RESET_REC;
        if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
            honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
            OneWire_ErrorHandler(honew);
        }

        if(honew->htim->State == HAL_TIM_STATE_READY){
            HAL_TIM_Base_Start_IT(honew->htim);
        }
}

/**
* @brief Use the Read ROM command for 1-Wire communication
*
* @param honew: Handle Struct pointer
*
* @retval None
*/
void OneWire_ReadROM(OneWire_HandleTypedef* honew){
    OneWire_BUS_InstructionData instrData;

    OneWire_WriteByte(honew, ONEWIRE_CMD_ReadROM);
    for(uint8_t i = 0; i < 8; ++i)
        OneWire_ReadByte(honew, &(honew->readROM_buffer[i]));

    instrData.instr = ONEWIRE_ATOMIC_READ_ROM_DONE;
    instrData.wait_time = ONEWIRE_TIMINGS_READ_DONE;

    if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
        honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
        OneWire_ErrorHandler(honew);
    }
}

/**
* @brief Use the Skip ROM command for 1-Wire communication
*
* @param honew: Handle Struct pointer
*
* @retval None
*/
void OneWire_SkipROM(OneWire_HandleTypedef* honew){
    OneWire_WriteByte(honew, ONEWIRE_CMD_SkipROM);
}

/**
* @brief Use the Match ROM command for 1-Wire communication
*
* @param honew: Handle Struct pointer
* @param rom: ROM to be matched (uint8_t array[8] = 64byte)
*
* @retval None
*/
void OneWire_MatchROM(OneWire_HandleTypedef* honew, uint8_t rom[8]){
    OneWire_WriteByte(honew, ONEWIRE_CMD_MatchROM);
    for(uint8_t i = 0; i < 8; ++i)
        OneWire_WriteByte(honew, rom[i]);
}

// SearchROM work in progress
/**
* @brief Start a complete search ROM algorithm
*
* After finishing the SearchROMCallback is called, and the values are saved
* inside the handling structure at ROM_buffer[], at the amount of found
* devices is found at num_devices
*
* @param honew: Handle Struct pointer
*
* @retval None
*/
//void OneWire_SearchROM(OneWire_HandleTypedef* honew){
//
//    switch (honew->search.search_state){
//    case ONEWIRE_SEARCHSTATE_IDLE:
//        honew->search.last_discrepancy = 0;
//        honew->search.search_bit_index = 0;
//        honew->search.done = 0;
//        honew->search.presence_pulse = 0;
//
//        OneWire_Reset(honew, &(honew->search.presence_pulse));
//        honew->search.search_state = ONEWIRE_SEARCHSTATE_RESET;
//        break;
//    case ONEWIRE_SEARCHSTATE_RESET:
//        if (honew->search.presence_pulse == 1){
//            honew->errno = ONEWIRE_ERROR_SEARCH_NOPULSE;
//            OneWire_SearchErrorHandler(honew);
//            break;
//        }
//        OneWire_WriteByte(honew, ONEWIRE_CMD_SearchROM);
//
//        OneWire_BUS_ReadBit(honew, &(honew->search.))
//        honew->search.search_state = ONEWIRE_SEARCHSTATE_SEND_CMD;
//        ONEWIRE_SEARCHST
//        break;
//    case ONEWIRE_SEARCHSTATE_READ_BIT:
//
//        break;
//    case ONEWIRE_SEARCHSTATE_DONE:
//
//        break;
//    }
//}

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

    OneWire_BUS_InstructionData data;
    if (!OneWire_BUS_dequeue(&(honew->bus_queue), &data))
        return;

    switch(data.instr){
    case ONEWIRE_ATOMIC_WRITE:
        HAL_GPIO_WritePin(honew->gpio_port, honew->gpio_pin, (GPIO_PinState)(data.data));
        break;
    case ONEWIRE_ATOMIC_READ:
        *(data.data) = HAL_GPIO_ReadPin(honew->gpio_port, honew->gpio_pin);
        break;
    case ONEWIRE_ATOMIC_READ_BYTE_DONE:
        *(data.data) = 0; // Reset stored byte
        for (size_t i = 0; i < 8; i++) {
            *(data.data) |= (honew->temp_byte_buffer[i] << i);
        }
        OneWire_ReadDoneCallback(honew);
        break;
    case ONEWIRE_ATOMIC_READ_ROM_DONE:
        *(data.data) = 0; // Reset stored byte
        for (size_t i = 0; i < 8; i++) {
            *(data.data) |= (honew->temp_byte_buffer[i] << i);
        }
        OneWire_ReadROMCallback(honew);
        break;
    case ONEWIRE_ATOMIC_RESET_DONE:
        if (honew->search.search_state == ONEWIRE_SEARCHSTATE_RESET){

            break;
        }
        OneWire_ResetDoneCallback(honew);
        break;
    }
    // SearchROM WIP
//    if (honew->search.search_state != ONEWIRE_SEARCHSTATE_IDLE)
//        OneWire_SearchROM(honew);

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

__weak void OneWire_ReadROMCallback(OneWire_HandleTypedef* honew){
    __NOP();
}

__weak void OneWire_SearchROMCallback(OneWire_HandleTypedef* honew){
    __NOP();
}

__weak void OneWire_SearchErrorHandler(OneWire_HandleTypedef* honew){
    __NOP();
}

__weak void OneWire_ErrorHandler(OneWire_HandleTypedef* honew){
    __NOP();
}

