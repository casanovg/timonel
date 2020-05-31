/*
 *  NB Micro TWI Master Library
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: NbMicro.cpp (Library)
 *  ........................................... 
 *  Version: 1.1.0 / 2020-05-24
 *  gustavo.casanova@gmail.com
 *  ...........................................
 *  This library handles the communication protocol with devices
 *  that implement the NB command set over a TWI (I2C) bus.
 */

#include "NbMicro.h"

#include <Arduino.h>

/////////////////////////////////////////////////////////////////////////////
////////////                    NbMicro class                    ////////////
/////////////////////////////////////////////////////////////////////////////

/* _________________________
  |                         | 
  |       Constructor       |
  |_________________________|
*/
NbMicro::NbMicro(uint8_t twi_address, uint8_t sda, uint8_t scl) : addr_(twi_address), sda_(sda), scl_(scl) {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
    // If the TWI address is not in use, create a new NbMicro object (ESP micros)
    if ((!active_addresses.insert(addr_).second) && (addr_ != 0)) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Error:     The TWI address [%02d] is in use! Unable to create another device object with it ...\r\n", __func__, addr_);
        USE_SERIAL.printf_P("[%s] Execution terminated, please review the devices' TWI addresses on your code.\r\n", __func__);
#endif  // DEBUG_LEVEL
        delay(DLY_NBMICRO);
        std::terminate();
    }
    // Start the TWI driver (ESP micros)
    // If SDA and SCL pins are not specified, use the default ones
    if (!((sda_ == 0) && (scl_ == 0))) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Creating a new TWI connection with address %02d\n\r", __func__, addr_);
#endif                           // DEBUG_LEVEL
        Wire.begin(sda_, scl_);  // Init I2C sda_:GPIO0, scl_:GPIO2 (ESP-01) / sda_:D3, scl_:D4 (NodeMCU)
    } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Reusing the TWI connection with address %02d\n\r", __func__, addr_);
#endif                                         // DEBUG_LEVEL
        Wire.begin(SDA_STD_PIN, SCL_STD_PIN);  // Init I2C with default SDA and SCL pins
    }
#else   // -----
    // If the TWI address is not in use, create a new NbMicro object (AVR micros)
    for (uint8_t i = 0; i < TWI_DEVICE_QTY; i++) {
        if (active_addresses[i] == addr_) {
            abort();
        }
        if (active_addresses[i] == 0) {
            active_addresses[i] = addr_;
            break;
        }
    }
    // Start the TWI driver using the default SDA and SCL pins (AVR micros)
    Wire.begin();  // Init I2C sda:PC4/18/A4, scl:PC5/19/A5 (ATmega)
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
}

/* _________________________
  |                         | 
  |      GetTwiAddress      |
  |_________________________|
*/
// Return this object's TWI address
uint8_t NbMicro::GetTwiAddress(void) {
    return addr_;
}

/* _________________________
  |                         | 
  |      SetTwiAddress      |
  |_________________________|
*/
// Set this object's TWI address (allowed only once if it wasn't set at object creation time)
uint8_t NbMicro::SetTwiAddress(uint8_t twi_address) {
    if (addr_ != 0) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Object TWI address already defined, using %d\r\n", __func__, addr_);
#endif  // DEBUG_LEVEL
        return ERR_ADDR_IN_USE;
    } else {
        addr_ = twi_address;
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Object TWI address correctly set to %d\r\n", __func__, addr_);
#endif  // DEBUG_LEVEL
        return 0;
    }
}

/* _________________________________________________
  |                                                 | 
  | TwiCmdXmit (single byte command)                |
  | - If no error                       -> return 0 |
  | - If ack error on single byte reply -> return 3 |
  | - If ack error on multi byte reply  -> return 4 |
  |_________________________________________________|
*/
// Send a TWI command to the microcontroller (Overload A: single byte command)
uint8_t NbMicro::TwiCmdXmit(uint8_t twi_cmd, uint8_t twi_reply, uint8_t twi_reply_arr[], uint8_t reply_size) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
    USE_SERIAL.printf_P("[%s] > Single byte cmd: 0x%02X, calling multi ...\n\r", __func__, twi_cmd);
#endif  // DEBUG_LEVEL
    const uint8_t cmd_size = 1;
    uint8_t twi_cmd_arr[cmd_size] = {twi_cmd};
    uint8_t twi_errors = TwiCmdXmit(twi_cmd_arr, cmd_size, twi_reply, twi_reply_arr, reply_size);
    if (twi_errors != 0) {
        return (twi_errors + 2);
    } else {
        return 0;
    }
}

/* _________________________________________________
  |                                                 | 
  | TwiCmdXmit (multibyte command)                  |
  | - If no error                       -> return 0 |
  | - If ack error on single byte reply -> return 1 |
  | - If ack error on multi byte reply  -> return 2 |
  |_________________________________________________|
*/
// Send a TWI command to the microcontroller (Overload B: multibyte command)
uint8_t NbMicro::TwiCmdXmit(uint8_t twi_cmd_arr[], uint8_t cmd_size, uint8_t twi_reply, uint8_t twi_reply_arr[], uint8_t reply_size) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
    USE_SERIAL.printf_P("[%s] > Multi byte cmd: 0x%02X --> making actual TWI transmission ...\n\r", __func__, twi_cmd_arr[0]);
#endif  // DEBUG_LEVEL
    // TWI command transmit
    for (int i = 0; i < cmd_size; i++) {
        Wire.beginTransmission(addr_);
        Wire.write(twi_cmd_arr[i]);
        Wire.endTransmission();
    }
    // TWI command reply (one byte expected)
    if (reply_size == 0) {
        reply_size++;
        //Wire.requestFrom(addr_, ++reply_size, STOP_ON_REQ); // True: releases the bus with a stop after a master request.
        Wire.requestFrom(addr_, ++reply_size);  // True: releases the bus with a stop after a master request.
        uint8_t reply = Wire.read();            // False: sends a restart, not releasing the bus.
        if (reply == twi_reply) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
            USE_SERIAL.printf_P("[%s] > Command 0x%02X parsed OK <<< 0x%02X (single byte reply)\n\r", __func__, twi_cmd_arr[0], reply);
#endif  // DEBUG_LEVEL
            return 0;
        } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
            USE_SERIAL.printf_P("[%s] > Error parsing 0x%02X command <<< 0x%02X (single byte reply)\n\r", __func__, twi_cmd_arr[0], reply);
#endif                               // DEBUG_LEVEL
            return ERR_CMD_PARSE_S;  // Error: reply doesn't match command (single byte)
        }
    }
    // TWI command reply (multiple bytes expected)
    else {
        //byte reply_length = Wire.requestFrom(addr_, reply_size, STOP_ON_REQ); // True: releases the bus with a stop after a master request.
        uint8_t reply_length = Wire.requestFrom(addr_, reply_size);  // True: releases the bus with a stop after a master request.
        for (int i = 0; i < reply_size; i++) {                       // False: sends a restart, not releasing the bus.
            twi_reply_arr[i] = Wire.read();
        }
        if ((twi_reply_arr[0] == twi_reply) && (reply_length == reply_size)) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 3))
            USE_SERIAL.printf_P("[%s] >>> Command 0x%02X parsed OK <<< 0x%02X (multibyte reply)\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
#endif  // DEBUG_LEVEL
            return 0;
        } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
            USE_SERIAL.printf_P("[%s] > Error parsing 0x%02X command <<< 0x%02X (multibyte reply)\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
#endif                               // DEBUG_LEVEL
            return ERR_CMD_PARSE_M;  // Error: reply doesn't match command (multi byte)
        }
    }
}

/* _________________________
  |                         | 
  |       Destructor        |
  |_________________________|
*/
NbMicro::~NbMicro() {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("[%s] Freeing TWI address %02d ...\r\n", __func__, addr_);
#endif  // DEBUG_LEVEL
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
    active_addresses.erase(addr_);
#else   // -----
    for (uint8_t i = 0; i < TWI_DEVICE_QTY; i++) {
        if (active_addresses[i] == addr_) {
            active_addresses[i] = 0;
            break;
        }
    }
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
}

/* _________________________
  |                         | 
  |        InitMicro        |
  |_________________________|
*/
uint8_t NbMicro::InitMicro(void) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("[%s] Initializing Micro %02d ...\r\n", __func__, addr_);
#endif  // DEBUG_LEVEL
    // Initialize the microcontroller firmware
    return (TwiCmdXmit(INITSOFT, ACKINITS));
}
