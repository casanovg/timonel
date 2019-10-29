/*
 *  AVR SOS TWIS
 *  Author: Gustavo Casanova / Nicebots
 *  ...........................................
 *  File: header.h (Application headers)
 *  ........................................... 
 *  Version: 1.0 / 2019-06-22
 *  gustavo.casanova@nicebots.com
 *  ...........................................
 */

#ifndef _AVR_SOS_H_
#define _AVR_SOS_H_

#ifndef __AVR_ATtiny85__
#define __AVR_ATtiny85__
#endif

// Includes
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <stddef.h>
#include <util/delay.h>
#include "../../nb-libs/cmd/nb-twi-cmd.h"
#include "../../nb-libs/twis/interrupt-based/nb-usitwisl.h"

#define LED_PIN PB1       /* >>> WARNING: Only use PB4 to monitor  <<< */
#define LED_DDR DDRB      /* >>> monitor activity if ADC2 is NOT   <<< */
#define LED_PORT PORTB    /* >>> implemented !!!					 <<< */

#endif /* _AVR_SOS_H_ */