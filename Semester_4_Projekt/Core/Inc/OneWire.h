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

/* @brief Enables special debug features (0 or 1)
 *
 * Setting this to 1 enables special variables:
 * honew->queue->current_elements
 * honew->queue->max_elements
 *
 * max_elements can be used to determine how much the queue fills up, to narrow
 * down the really needed size for the bus queue (see ONEWIRE_BUS_QUEUE_SIZE)
 */
#define ONEWIRE_DEBUG 0

/* @brief max size of the bus queue
 *
 * The bus queue is static, so this define sets the maximum elements for
 * the bus queue. If it is too small honew->errno will be set to
 * ONEWIRE_ERROR_QUEUE_FULL (see OneWire_ErrorTable)
 * It can be narrowed down with "honew->queue->max_elements" with setting
 * ONEWIRE_DEBUG to 1
 */
#define ONEWIRE_BUS_QUEUE_SIZE 1024

/* @brief max amount of devices on the bus
 *
 * When using the SearchRom algorithm the found ROMs are saved into an array
 * this array is sized with this define
 */
#define ONEWIRE_MAX_DEVICES 10

/* @brief ONEWIRE_TIMINGS_xxx is used to modify how the write/read slots are timed
 *
 * This value is put into the ARR of the timer that is declared in OneWire_init()
 * The PRR value of the timer can be chosen.
 * See datasheet for expected values
 *
 * The next diagrams should visualize how the timing defines are to be used
 * ] = Bus is kept at level until this point (uC waits until this point for next instruction)
 * ! = Bus is read at this spot
 *
 * Be aware that the bus is open-drain so if the uC sets
 * the bus to H, the slave can still pull the bus low!
 *
 *
 *       Write 0 Slot
 *   ^
 * H |___            _________
 *   |   |          |
 *   |   |    (1)   | (2)
 *   |   |<-------->|<--->]
 * L |   |__________|
 *   +------------------------>
 *
 *   (1) ONEWIRE_TIMINGS_WRITE0_LOW
 *   (2) ONEWIRE_TIMINGS_WRITE0_HIGH
 *
 * ==================================
 *
 *       Write 1 Slot
 *   ^
 * H |___       _______________
 *   |   |     |
 *   |   | (3) |   (4)
 *   |   |<--->|<-------->]
 * L |   |_____|
 *   +------------------------>
 *
 *   (3) ONEWIRE_TIMINGS_WRITE1_LOW
 *   (4) ONEWIRE_TIMINGS_WRITE1_HIGH
 *
 * ==================================
 *
 *       Read Bit Slot
 * (Slave can pull down at !)
 *
 *   ^
 * H |___       _______________
 *   |   |     |
 *   |   | (5) | (6)    (7)
 *   |   |<--->|<--->!<----->]
 * L |   |_____|
 *   +------------------------>
 *
 *   (5) ONEWIRE_TIMINGS_READBIT_LOW
 *   (6) ONEWIRE_TIMINGS_READBIT_HIGH
 *   (7) ONEWIRE_TIMINGS_READBIT_HOLD
 *
 * ==================================
 *
 *       Reset Pulse
 * (Slave should pull down at !)
 *
 *   ^
 * H |___          ___________
 *   |   |        |
 *   |   |   (8)  | (9)  (10)
 *   |   |<------>|<--->!<-->]
 * L |   |________|
 *   +------------------------>
 *
 *   (8) ONEWIRE_TIMINGS_RESET_LOW
 *   (9) ONEWIRE_TIMINGS_RESET_HIGH
 *   (10) ONEWIRE_TIMINGS_RESET_HOLD
 *
 */

#define ONEWIRE_TIMINGS_WRITE0_LOW      5900                // (1)
#define ONEWIRE_TIMINGS_WRITE0_HIGH     100                 // (2)
#define ONEWIRE_TIMINGS_WRITE1_LOW      100                 // (3)
#define ONEWIRE_TIMINGS_WRITE1_HIGH     5900                // (4)
#define ONEWIRE_TIMINGS_READBIT_LOW     100                 // (5)
#define ONEWIRE_TIMINGS_READBIT_HIGH    1400                // (6)
#define ONEWIRE_TIMINGS_READBIT_HOLD    4500                // (7)
#define ONEWIRE_TIMINGS_RESET_LOW       48000               // (8)
#define ONEWIRE_TIMINGS_RESET_HIGH      6000                // (9)
#define ONEWIRE_TIMINGS_RESET_HOLD      (48000 - 6000)      // (10)

// --- MODIFY BELOW AT OWN RISK ---

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
    ONEWIRE_ATOMIC_SEARCH_PROCESS_BIT,
    ONEWIRE_ATOMIC_SEARCH_DONE,
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

#ifdef ONEWIRE_DEBUG
    uint32_t current_elements;
    uint32_t max_elements;
#endif /* ONEWIRE_DEBUG */
} OneWire_BUS_Queue;

typedef enum {
    ONEWIRE_CMD_SearchROM   = 0xF0,
    ONEWIRE_CMD_ReadROM     = 0x33,
    ONEWIRE_CMD_MatchROM    = 0x55,
    ONEWIRE_CMD_SkipROM     = 0xCC,
    ONEWIRE_CMD_AlarmSearch = 0xEC,
} OneWire_CMD;

typedef struct {
    uint8_t ROM_NO[8];                    // Current ROM candidate
    uint8_t ROMs[ONEWIRE_MAX_DEVICES][8]; // Found ROMs
    uint8_t device_count;
    uint8_t temp_bit_buffer[64][2];
    uint8_t last_discrepancy;
    uint8_t last_device_flag;
    uint8_t search_index;
    uint8_t bit_index;
    uint8_t bit_value;
    uint8_t search_active;
    uint8_t alarm_search;
} OneWire_SearchContext;

typedef struct {
    OneWire_BUS_Queue bus_queue;
    TIM_HandleTypeDef* htim;
    GPIO_TypeDef* gpio_port;
    uint16_t gpio_pin;
    uint8_t temp_byte_buffer[8];
    OneWire_SearchContext search;
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
void OneWire_ReadROM(OneWire_HandleTypedef*, uint8_t[8]);
void OneWire_SkipROM(OneWire_HandleTypedef*);
void OneWire_MatchROM(OneWire_HandleTypedef*, uint8_t[8]);
void OneWire_SearchStart(OneWire_HandleTypedef*);
void OneWire_SearchStep(OneWire_HandleTypedef*);
void OneWire_AlarmSearch(OneWire_HandleTypedef*);

void OneWire_TIM_Hook(OneWire_HandleTypedef*);

void OneWire_ReadDoneCallback(OneWire_HandleTypedef*);
void OneWire_ResetDoneCallback(OneWire_HandleTypedef*);
void OneWire_ReadROMCallback(OneWire_HandleTypedef*);
void OneWire_SearchROMCallback(OneWire_HandleTypedef*);
void OneWire_AlarmSearchCallback(OneWire_HandleTypedef*);

void OneWire_ErrorHandler(OneWire_HandleTypedef*);


#endif /* INC_ONEWIRE_FIRSTTESTS_H_ */
