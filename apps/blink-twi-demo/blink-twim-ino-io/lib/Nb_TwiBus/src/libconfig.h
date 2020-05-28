/*
  libconfig.h
  =================================
  TwiBus configuration
  ---------------------------------
  Version: 1.1.0 / 2020-05-25
  gustavo.casanova@gmail.com
  ---------------------------------
*/

#ifndef _TWIBUS_CONFIG_H_
#define _TWIBUS_CONFIG_H_

/////////////////////////////////////////////////////////////////////////////
////////////                   TwiBus settings                   ////////////
/////////////////////////////////////////////////////////////////////////////

// General defs
#define DETECT_TIMONEL false  // Enable Timonel bootloader detection
#define SDA_STD_PIN 4        // I2C SDA standard pin on ESP866 boards
#define SCL_STD_PIN 5        // I2C SCL standard pin on ESP866 boards
#define LOW_TWI_ADDR 8       // Lowest allowed TWI address on slave devices
#define HIG_TWI_ADDR 63      // Highest allowed TWI address on slave devices
#define DLY_SCAN_BUS 1       // TWI scanner pass delay
#define L_TIMONEL "Timonel"  // Literal: Timonel
#define L_UNKNOWN "Unknown"  // Literal: Unknown
#define L_APP "Application"  // Literal: Application
// End general defs

/////////////////////////////////////////////////////////////////////////////
////////////                    End settings                     ////////////
/////////////////////////////////////////////////////////////////////////////

#endif /* _TWIBUS_CONFIG_H_ */