# Microcontroller Programming Project / 4th Semester
This Project was made during a lecture in Microcontroller-Programming
for a Bachelor's Degree in Applied Electronics, made by *Johra-Markus Singh* and *Sebastian Heide*

## Key Components
- LCD 20x4 with PCF8574 Port Expander
- DS18B20 Temperature Sensor
- STM32F411 Discovery-Board (using HAL)
- Custom I/O-Board for the Discovery Board

## DS18B20 Library
The *DS18B20* Library for this project was programmed with it's own unique 1-Wire (here called OneWire) library.
It is programmed to be non-blocking, and uses a unique Queue-System, which pulls the information for the 1-Wire Bus (GPIO-Pin),
at exact timings using Hardware Timers. The downside for this is
that it uses a lot of memory, but using the *Debug* Mode, the Queue Size can be narrowed down, to save
some of it.

## Bugs
This Project and their libraries are not perfect, and are definitly not completly finished,
as it was just a little course during 1 Semester.

1. The **Search ROM Algorithm** for the 1-Wire Bus, and therefor the multi-device mode,
was not entirely finished using the non-blocking functions of the lib.
Therefore there is no *Alarm Search* functionality, as it would use the *Search ROM* algorithm 

2. As far as it was tested, the EEPROM of the DS18B20 will also not be written sometimes.
It can be pushed into the Scratchpad, but using the Copy Scratchpad command, does not always
copy it into the EEPROM. No idea why.

***

For any further questions regarding this project you can contact: <johra.singh@hotmail.com>
