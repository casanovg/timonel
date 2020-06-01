/*
 *  AVR BLINK TWIS
 *  Author: Gustavo Casanova / Nicebots
 *  ...........................................
 *  File: header.h (Application headers)
 *  ........................................... 
 *  Version: 1.0 / 2019-06-22
 *  gustavo.casanova@nicebots.com
 *  ...........................................
 */

#ifndef _AVR_BLINK_TWIS_H_
#define _AVR_BLINK_TWIS_H_

#ifndef __AVR_ATtiny85__
#define __AVR_ATtiny85__
#endif

// Includes
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <stddef.h>
#include <util/delay.h>

#include "./nb-libs/cmd/nb-twi-cmd.h"
#include "./nb-libs/twis/interrupt-based/nb-usitwisl.h"

#define LED_PIN PB1
#define LED_DDR DDRB
#define LED_PORT PORTB

#endif  // _AVR_BLINK_TWIS_H_