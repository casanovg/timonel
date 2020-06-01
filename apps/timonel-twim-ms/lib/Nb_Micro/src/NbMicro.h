/*
 *  NB Micro TWI Master Library
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: NbMicro.h (Header)
 *  ........................................... 
 *  Version: 1.1.0 / 2020-05-24
 *  gustavo.casanova@gmail.com
 *  ...........................................
 *  This library handles the communication protocol with devices
 *  that implement the NB command set over a TWI (I2C) bus.
 */

#ifndef _NBMICRO_H_
#define _NBMICRO_H_

#include <Arduino.h>
#include <Wire.h>
#include <nb-twi-cmd.h>

#include "libconfig.h"

// Store of TWI addresses in use ...
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
#include <unordered_set>
static std::unordered_set<uint8_t> active_addresses;
#else   // -----
volatile static uint8_t active_addresses[TWI_DEVICE_QTY] = {0};
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM

/* 
 * ===================================================================
 * NbMicro class: Represents a slave microcontroller
 * using the NB command set connected to the TWI bus
 * ===================================================================
 */
class NbMicro {
   public:
    NbMicro(uint8_t twi_address = 0, uint8_t sda = 0, uint8_t scl = 0);
    ~NbMicro();
    uint8_t GetTwiAddress(void);
    uint8_t SetTwiAddress(uint8_t twi_address);
    uint8_t TwiCmdXmit(uint8_t twi_cmd, uint8_t twi_reply,
                       uint8_t twi_reply_arr[] = nullptr, uint8_t reply_size = 0);
    uint8_t TwiCmdXmit(uint8_t twi_cmd_arr[], uint8_t cmd_size, uint8_t twi_reply,
                       uint8_t twi_reply_arr[] = nullptr, uint8_t reply_size = 0);
    uint8_t InitMicro(void);

   protected:
    uint8_t addr_ = 0, sda_ = 0, scl_ = 0;

   private:
};

#endif  // _NBMICRO_H_
