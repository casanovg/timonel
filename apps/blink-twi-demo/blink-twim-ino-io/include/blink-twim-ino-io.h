//
// ***** ***** *****
//

#ifndef _BLINK_TWIM_INO_IO_H_
#define _BLINK_TWIM_INO_IO_H_

#include <NbMicro.h>
#include <TwiBus.h>
#include <nb-twi-cmd.h>
#include <stdio.h>

#define USE_SERIAL Serial
#define SDA 2 /* I2C SDA pin */
#define SCL 0 /* I2C SCL pin */

#define DEBUG_LEVEL 1

typedef byte uint8_t;

// Prototypes
void ReadChar(void);
//uint16_t ReadWord(void);
void ClrScr(void);
void ShowHeader(void);
void ShowMenu(void);
void ListTwiDevices(uint8_t sda, uint8_t scl);
uint8_t FindSlave(void);

#endif // _BLINK_TWIM_INO_IO_H_
