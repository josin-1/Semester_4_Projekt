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

    if (data <= 2) // only 1 and 0 expected
        return;

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    if (data == 1){
        instrData.data = (uint8_t*)GPIO_PIN_RESET;
        instrData.wait_time = 100;
    }
    else {
        instrData.data = (uint8_t*)GPIO_PIN_RESET;
        instrData.wait_time = 5900;
    }
    OneWire_BUS_enqueue(&(honew->bus_queue), &instrData);

    instrData.instr = ONEWIRE_ATOMIC_WRITE;
    if (data == 0){
        instrData.data = (uint8_t*)GPIO_PIN_SET;
        instrData.wait_time = 5900;
    }
    else {
        instrData.data = (uint8_t*)GPIO_PIN_SET;
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
        OneWire_BUS_WriteBit(honew, (data >> i) & 1);
    }
}

void OneWire_ReadByte(OneWire_HandleTypedef* honew, uint8_t* data_ptr){
    // Coming soon...
}

uint8_t OneWire_Reset(OneWire_HandleTypedef* honew){
    // pull bus low for atleast 420us or sth like this and read presence pulse
    // still needs to be implemented
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
    honew->htim->Instance->ARR = data.wait_time;
    honew->htim->Instance->CNT = 0;
    if (honew->bus_queue.begin != honew->bus_queue.end) // if queue is not empty start tim
        HAL_TIM_Base_Start_IT(honew->htim);
}




//--------------------------------------------------------------------------
// Find the 'first' devices on the 1-Wire bus
// Return 1  : device found, ROM number in ROM_NO buffer
//        0 : no device present
//
uint8_t OneWire_First(OneWire_HandleTypedef* honew)
{
   // reset the search state
   honew->LastDiscrepancy = 0;
   honew->LastDeviceFlag = 0;
   honew->LastFamilyDiscrepancy = 0;

   return OneWire_Search(honew);
}

//--------------------------------------------------------------------------
// Find the 'next' devices on the 1-Wire bus
// Return 1 : device found, ROM number in ROM_NO buffer
//        0 : device not found, end of search
//
uint8_t OneWire_Next(OneWire_HandleTypedef* honew)
{
   // leave the search state alone
   return OneWire_Search(honew);
}

//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return 1 : device found, ROM number in ROM_NO buffer
//        0 : device not found, end of search
//
uint8_t OneWire_Search(OneWire_HandleTypedef* honew)
{
   int id_bit_number;
   int last_zero, rom_byte_number, search_result;
   int id_bit, cmp_id_bit;
   int crc8;
   uint8_t rom_byte_mask, search_direction;

   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = 0;
   crc8 = 0;

   // if the last call was not the last one
   if (!(honew->LastDeviceFlag))
   {
      // 1-Wire reset
      if (!OneWire_Reset(honew))
      {
         // reset the search
         honew->LastDiscrepancy = 0;
         honew->LastDeviceFlag = 0;
         honew->LastFamilyDiscrepancy = 0;
         return 0;
      }

      // issue the search command
      OneWire_WriteByte(honew, 0xF0);

      // loop to do the search
      do
      {
         // read a bit and its complement
         OneWire_BUS_ReadBit(honew, &id_bit);
         OneWire_BUS_ReadBit(honew, &cmp_id_bit);

         // check for no devices on 1-wire
         if ((id_bit == 1) && (cmp_id_bit == 1))
            break;
         else
         {
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit)
               search_direction = id_bit;  // bit write value for search
            else
            {
               // if this discrepancy if before the Last Discrepancy
               // on a previous next then pick the same as last time
               if (id_bit_number < honew->LastDiscrepancy)
                  search_direction = ((honew->ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
               else
                  // if equal to last pick 1, if not then pick 0
                  search_direction = (id_bit_number == honew->LastDiscrepancy);

               // if 0 was picked then record its position in LastZero
               if (search_direction == 0)
               {
                  last_zero = id_bit_number;

                  // check for Last discrepancy in family
                  if (last_zero < 9)
                      honew->LastFamilyDiscrepancy = last_zero;
               }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1)
                honew->ROM_NO[rom_byte_number] |= rom_byte_mask;
            else
                honew->ROM_NO[rom_byte_number] &= ~rom_byte_mask;

            // serial number search direction write bit
            OneWire_BUS_WriteBit(honew, search_direction);

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (rom_byte_mask == 0)
            {
                //OneWire_crc8();  // accumulate the CRC
                rom_byte_number++;
                rom_byte_mask = 1;
            }
         }
      }
      while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

      // if the search was successful then
      if (!((id_bit_number < 65) || (crc8 != 0)))
      {
         // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
          honew->LastDiscrepancy = last_zero;

         // check for last device
         if (honew->LastDiscrepancy == 0)
             honew->LastDeviceFlag = 1;

         search_result = 1;
      }
   }

   // if no device found then reset counters so next 'search' will be like a first
   if (!search_result || !(honew->ROM_NO[0]))
   {
      honew->LastDiscrepancy = 0;
      honew->LastDeviceFlag = 0;
      honew->LastFamilyDiscrepancy = 0;
      search_result = 0;
   }

   return search_result;
}

//--------------------------------------------------------------------------
// Verify the device with the ROM number in ROM_NO buffer is present.
// Return 1  : device verified present
//        0 : device not present
//
uint8_t OneWire_Verify(OneWire_HandleTypedef* honew)
{
   uint8_t rom_backup[ONEWIRE_MAX_DEVICES];
   uint8_t i,rslt,ld_backup,ldf_backup,lfd_backup;

   // keep a backup copy of the current state
   for (i = 0; i < 8; i++)
      rom_backup[i] = honew->ROM_NO[i];
   ld_backup = honew->LastDiscrepancy;
   ldf_backup = honew->LastDeviceFlag;
   lfd_backup = honew->LastFamilyDiscrepancy;

   // set search to find the same device
   honew->LastDiscrepancy = 64;
   honew->LastDeviceFlag = 0;

   if (OneWire_Search(honew))
   {
      // check if same device found
      rslt = 1;
      for (i = 0; i < 8; i++)
      {
         if (rom_backup[i] != honew->ROM_NO[i])
         {
            rslt = 0;
            break;
         }
      }
   }
   else
     rslt = 0;

   // restore the search state
   for (i = 0; i < 8; i++)
       honew->ROM_NO[i] = rom_backup[i];
   honew->LastDiscrepancy = ld_backup;
   honew->LastDeviceFlag = ldf_backup;
   honew->LastFamilyDiscrepancy = lfd_backup;

   // return the result of the verify
   return rslt;
}

//--------------------------------------------------------------------------
// Setup the search to find the device type 'family_code' on the next call
// to OWNext() if it is present.
//
void OneWire_TargetSetup(OneWire_HandleTypedef* honew, unsigned char family_code)
{
   int32_t i;

   // set the search state to find SearchFamily type devices
   honew->ROM_NO[0] = family_code;
   for (i = 1; i < 8; i++)
       honew->ROM_NO[i] = 0;
   honew->LastDiscrepancy = 64;
   honew->LastFamilyDiscrepancy = 0;
   honew->LastDeviceFlag = 0;
}

//--------------------------------------------------------------------------
// Setup the search to skip the current device type on the next call
// to OWNext().
//
void OneWire_FamilySkipSetup(OneWire_HandleTypedef* honew)
{
   // set the Last discrepancy to last family discrepancy
    honew->LastDiscrepancy = honew->LastFamilyDiscrepancy;
    honew->LastFamilyDiscrepancy = 0;

   // check for end of list
   if (honew->LastDiscrepancy == 0)
       honew->LastDeviceFlag = 1;
}


//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current
// global 'crc8' value.
// Returns current global crc8 value
//
uint8_t OneWire_crc8(const uint8_t *data, uint8_t cnt){

// https://www.mikrocontroller.net/topic/387139?goto=4890827#4890827
    static const uint8_t crc_table[16] = {
            0x00, 0x9D, 0x23, 0xBE, 0x46, 0xDB, 0x65, 0xF8,
            0x8C, 0x11, 0xAF, 0x32, 0xCA, 0x57, 0xE9, 0x74};

    uint8_t crc, i, tmp, zerocheck;

    // nibble based CRC calculation,
    // good trade off between speed and memory usage

    // first byte is not changed, since CRC is initialized with 0
    crc = *data++;
    zerocheck = crc;
    cnt--;

    for(; cnt>0; cnt--) {
        tmp = *data++;                        // next byte
        zerocheck |= tmp;

        i = crc & 0x0F;
        crc = (crc >> 4) | (tmp << 4);        // shift in next nibble
        crc ^= pgm_read_byte(&crc_table[i]);  // apply polynom

        i = crc & 0x0F;
        crc = (crc >> 4) | (tmp & 0xF0);      // shift in next nibble
        crc ^= pgm_read_byte(&crc_table[i]);  // apply polynom
    }

    if (!zerocheck) {        // all data was zero, this is an error!
       return 0xFF;
    } else {
       return crc;
    }
}
