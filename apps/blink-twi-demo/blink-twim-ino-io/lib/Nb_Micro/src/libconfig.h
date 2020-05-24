/*
  libconfig.h
  =================================
  NbMicro configuration
  ---------------------------------
  Version: 1.1.0 / 2020-05-24
  gustavo.casanova@gmail.com
  ---------------------------------
*/

#ifndef _NBMICRO_CONFIG_H_
#define _NBMICRO_CONFIG_H_

/////////////////////////////////////////////////////////////////////////////
////////////                  NbMicro settings                   ////////////
/////////////////////////////////////////////////////////////////////////////

// General defs
#define SDA_STD_PIN 4     // I2C SDA standard pin on ESP866 boards
#define SCL_STD_PIN 5     // I2C SCL standard pin on ESP866 boards
#define LOW_TWI_ADDR 8    // Lowest allowed TWI address on slave devices
#define HIG_TWI_ADDR 63   // Highest allowed TWI address on slave devices
#define TWI_DEVICE_QTY 5  // Quantity of simultaneous TWI devices that an application supports
// End General defs

// NbMicro::constructor defs
#define DLY_NBMICRO 500  // Delay before canceling NbMiccro object creation (ms)
// End NbMicro::constructor defs

// NbMicro::SetTwiAddress defs
#define ERR_ADDR_IN_USE 1  // Error: The TWI address is already taken
// End NbMicro::SetTwiAddress defs

// NbMicro::TwiCmdXmit defs
#define STOP_ON_REQ true   // Config: true=master releases the bus with "stop" after a request, false=sends restart
#define ERR_TWI_ADDR_NA 1  // Error: Can't create NbMicro object, the TWI address is already in use
#define ERR_NO_TWI_SPAC 2  // Error: No available TWI addresses to create a new object
#define ERR_CMD_PARSE_S 3  // Error: reply doesn't match command (single byte)
#define ERR_CMD_PARSE_M 4  // Error: reply doesn't match command (multi byte)
// End NbMicro::TwiCmdXmit defs

/////////////////////////////////////////////////////////////////////////////
////////////                    End settings                     ////////////
/////////////////////////////////////////////////////////////////////////////

#endif  // _NBMICRO_CONFIG_H_
