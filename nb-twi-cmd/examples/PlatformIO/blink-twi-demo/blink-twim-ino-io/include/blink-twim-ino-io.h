/*
 *  Blink TWI Master for Arduino (PlatformIO)
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: blink-twim-ino-io.h (Header)
 *  ........................................... 
 *  Version: 1.0 / 2020-05-29
 *  gustavo.casanova@gmail.com
 *  ...........................................
 *  This library allows scanning the TWI (I2C) bus in search
 *  of connected devices addresses and data. If a device found
 *  is running Timonel, it returns its version number.
 */

#ifndef _BLINK_TWIM_INO_IO_H_
#define _BLINK_TWIM_INO_IO_H_

#include <Arduino.h>
#include <NbMicro.h>
#include <TwiBus.h>
#include <nb-twi-cmd.h>
#include <stdio.h>

#define USE_SERIAL Serial
#define SDA 21  // I2C SDA pin - ESP8266 2 - ESP32 21
#define SCL 22  // I2C SCL pin - ESP8266 0 - ESP32 22

// #define DEBUG_LEVEL 1

typedef byte uint8_t;

// Prototypes
void ReadChar(void);
//uint16_t ReadWord(void);
void ClrScr(void);
void ShowHeader(void);
void ShowMenu(void);
void ListTwiDevices(uint8_t sda, uint8_t scl);
uint8_t FindSlave(void);
//void resetFunc(void);
void (*resetFunc)(void) = 0;

#endif  // _BLINK_TWIM_INO_IO_H_
