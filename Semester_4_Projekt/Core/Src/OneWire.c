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
    queue->current_elements = 0;
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

#ifdef ONEWIRE_DEBUG
    queue->current_elements++;
    if (queue->current_elements > queue->max_elements)
        queue->max_elements = queue->current_elements;
#endif /* ONEWIRE_DEBUG */

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

#ifdef ONEWIRE_DEBUG
    queue->current_elements--;
#endif /* ONEWIRE_DEBUG */
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
    instrData.wait_time = 1;
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
* @retval none
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
        instrData.wait_time = 1;
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
void OneWire_ReadROM(OneWire_HandleTypedef* honew, uint8_t rom[8]){
    OneWire_BUS_InstructionData instrData;

    OneWire_WriteByte(honew, ONEWIRE_CMD_ReadROM);
    for(uint8_t i = 0; i < 8; ++i)
        OneWire_ReadByte(honew, &(rom[i]));

    instrData.instr = ONEWIRE_ATOMIC_READ_ROM_DONE;
    instrData.wait_time = 1;

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

/**
* @brief Start a search ROM algorithm
*
* This function initializes the search context inside honew to perform a normal
* search ROM algorithm on the 1-Wire bus (find ROMs of existing devices)
*
* @param honew: Handle Struct pointer
*
* @retval None
*/
void OneWire_SearchStart(OneWire_HandleTypedef* honew) {
    // reset complete search context
    memset(&honew->search, 0, sizeof(OneWire_SearchContext));
    honew->search.search_active = 1;
    honew->search.device_count = 0;
    honew->search.last_discrepancy = 0;
    honew->search.last_device_flag = 0;
    honew->search.alarm_search = 0;

    // Start first search iteration
    OneWire_SearchStep(honew);
}

/**
* @brief Start an alarm search algorithm
*
* ---insert "ALARM" meme here---
* This function initializes the search context inside honew to perform an alarm search
* on the 1-Wire bus
*
* @param honew: Handle Struct pointer
*
* @retval None
*/
void OneWire_AlarmSearch(OneWire_HandleTypedef* honew) {
    // reset complete search context
    memset(&honew->search, 0, sizeof(OneWire_SearchContext));
    honew->search.search_active = 1;
    honew->search.device_count = 0;
    honew->search.last_discrepancy = 0;
    honew->search.last_device_flag = 0;
    honew->search.alarm_search = 1; // activate alarm search

    // Start first search iteration
    OneWire_SearchStep(honew);
}

/**
* @brief Actual search algorithm
*
* Here resides the actual search algorithm after beeing started with OneWire_SearchStart()
* or OneWire_AlarmSearch(). It pushes the need instructions onto the queue, and will be called for
* every device thats on the bus
*
* @param honew: Handle Struct pointer
*
* @retval None
*/
void OneWire_SearchStep(OneWire_HandleTypedef* honew) {
    OneWire_BUS_InstructionData instrData;

    if (honew->search.last_device_flag) {
        honew->search.search_active = 0;
        if (honew->search.alarm_search == 0) // normal search done
            OneWire_SearchROMCallback(honew);
        else // alarm search done
            OneWire_AlarmSearchCallback(honew);
        return; // All devices found
    }

    honew->search.bit_index = 0;
    memset(honew->search.ROM_NO, 0, 8);

    OneWire_Reset(honew, &(honew->temp_byte_buffer[0])); // save the presence pulse somewhere
    if (honew->search.alarm_search == 0)
        OneWire_WriteByte(honew, ONEWIRE_CMD_SearchROM); // Search ROM command
    else // alarm_search set to 1
        OneWire_WriteByte(honew, ONEWIRE_CMD_AlarmSearch); // Alarm Search command

    for (uint8_t bit = 0; bit < 64; bit++) {
        // Queue 2 reads (bit + complement) → stored in temp_bit_buffer[bit][0/1]
        OneWire_BUS_ReadBit(honew, &honew->search.temp_bit_buffer[bit][0]); // read bit
        OneWire_BUS_ReadBit(honew, &honew->search.temp_bit_buffer[bit][1]); // read complement

        // Queue a special SEARCH_PROCESS_BIT instruction
        instrData.instr = ONEWIRE_ATOMIC_SEARCH_PROCESS_BIT;
        instrData.data = (uint8_t*)(uint32_t)bit; // pls ignore this weird cast... no warnings like this ;P
        instrData.wait_time = 1;
        if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
            honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
            OneWire_ErrorHandler(honew);
        }

        // WERE GONNA IGNORE THAT DELAY ... OKEY!!!
        HAL_Delay(1);
    }

    // At the end, enqueue ONEWIRE_ATOMIC_SEARCH_DONE
    instrData.instr = ONEWIRE_ATOMIC_SEARCH_DONE;
    instrData.wait_time = 1;
    if (!OneWire_BUS_enqueue(&(honew->bus_queue), &instrData)){
        honew->errno = ONEWIRE_ERROR_QUEUE_FULL;
        OneWire_ErrorHandler(honew);
    }

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
        OneWire_ReadROMCallback(honew);
        break;
    case ONEWIRE_ATOMIC_RESET_DONE:
        OneWire_ResetDoneCallback(honew);
        break;
    case ONEWIRE_ATOMIC_SEARCH_PROCESS_BIT:
        uint8_t bit_index = (uint32_t)(data.data); // recover bit index
        uint8_t bit = honew->search.temp_bit_buffer[bit_index][0];
        uint8_t comp = honew->search.temp_bit_buffer[bit_index][1];

        uint8_t search_path;

        if (bit == 1 && comp == 1) {
            honew->search.search_active = 0; // No devices responding
            honew->search.search_active = 0;
            if (honew->search.alarm_search == 0) // normal search done
                OneWire_SearchROMCallback(honew);
            else // alarm search done
                OneWire_AlarmSearchCallback(honew);
            break;
        }

        if (bit != comp) {
            search_path = bit; // no conflict, use existing bit
        } else {
            // conflict — must choose
            if (bit_index == honew->search.last_discrepancy)
                search_path = 1;
            else if (bit_index > honew->search.last_discrepancy)
                search_path = 0;
            else
                search_path = (honew->search.ROM_NO[bit_index / 8] >> (bit_index % 8)) & 0x01; // This margin is too small to explain this feckery

            if (search_path == 0)
                honew->search.last_discrepancy = bit_index;
        }

        // Store bit into ROM_NO
        honew->search.ROM_NO[bit_index / 8] |= (search_path << (bit_index % 8));

        // Queue the selected path bit to write
        OneWire_BUS_WriteBit(honew, search_path);
        break;
    case ONEWIRE_ATOMIC_SEARCH_DONE:
        memcpy(honew->search.ROMs[honew->search.device_count], honew->search.ROM_NO, 8);
        honew->search.device_count++;

        // Determine if more devices remain
        if (honew->search.last_discrepancy == 0)
            honew->search.last_device_flag = 1;

        OneWire_SearchStep(honew);
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

__weak void OneWire_ReadROMCallback(OneWire_HandleTypedef* honew){
    __NOP();
}

__weak void OneWire_SearchROMCallback(OneWire_HandleTypedef* honew){
    __NOP();
}

__weak void OneWire_AlarmSearchCallback(OneWire_HandleTypedef*) {
    __NOP();
}

__weak void OneWire_ErrorHandler(OneWire_HandleTypedef* honew){
    __NOP();
}
