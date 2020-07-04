/*
 *  BLINK WIRE SLAVE for ATmega microcontrollers
 *  Author: Gustavo Casanova / Nicebots
 *  ................................................
 *  File: blink-wires-ino-io.h (Application headers)
 *  ................................................
 *  Version: 1.0 / 2020-05-16
 *  gustavo.casanova@nicebots.com
 *  ................................................
 */

#ifndef _BLINK_WIRES_INO_IO__H_
#define _BLINK_WIRES_INO_IO__H_

// #ifndef __AVR_ATtiny85__
// #define __AVR_ATtiny85__
// #endif

// Includes
#include <Arduino.h>
#include <Wire.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <nb-twi-cmd.h>
#include <stdbool.h>
#include <util/delay.h>

#define TWI_ADDR 12
#define LONG_DELAY 0x3FFFF

//#define LED_PIN PB1 // Digispark ATtiny85
#define LED_PIN PB5  // Arduino Pro Mini
#define LED_DDR DDRB
#define LED_PORT PORTB

void RequestEvent(void);
void ReceiveEvent(uint8_t received_bytes);
void ClrScr(void);

#endif  // _BLINK_WIRES_INO_IO__H_
