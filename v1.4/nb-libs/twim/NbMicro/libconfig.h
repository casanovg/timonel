/*
  libconfig.h
  =================================
  NbMicro & TwiBus configuration
  ---------------------------------
  Version: 1.4 2019-08-09
  gustavo.casanova@nicebots.com
  ---------------------------------
*/

#ifndef _NBMICRO_CONFIG_H_
#define _NBMICRO_CONFIG_H_

/////////////////////////////////////////////////////////////////////////////
////////////              NbMicro & TwiBus settings              ////////////
/////////////////////////////////////////////////////////////////////////////

// General defs
//#define DEBUG_LEVEL 0     /* Level 0: No debug info over serial terminal */
//#define USE_SERIAL Serial /* Console output */
#define MULTI_DEVICE true   /* Enable multi device discovery code */
#define LOW_TWI_ADDR 8      /* Lowest allowed TWI address on slave devices */
#define HIG_TWI_ADDR 63     /* Highest allowed TWI address on slave devices */
#define OK 0                /* No error in function execution */
#define T_SIGNATURE 84      /* Timonel signature "T" */
// End General defs

// NbMicro::constructor defs
#define DLY_NBMICRO 500     /* Delay before canceling NbMiccro object creation (ms) */
// End NbMicro::constructor defs

// NbMicro::SetTwiAddress defs
#define ERR_ADDR_IN_USE 1   /* Error: The TWI address is already taken */
// End NbMicro::SetTwiAddress defs

// NbMicro::TwiCmdXmit defs
#define STOP_ON_REQ true    /* Config: true=master releases the bus with "stop" after a request, false=sends restart */
#define ERR_CMD_PARSE_S 1   /* Error: reply doesn't match command (single byte) */
#define ERR_CMD_PARSE_M 2   /* Error: reply doesn't match command (multi byte) */
// End NbMicro::TwiCmdXmit defs

// TwiBus::ScanBus defs
#define DLY_SCAN_BUS 1      /* TWI scanner pass delay */
#define L_TIMONEL "Timonel" /* Literal: Timonel */
#define L_UNKNOWN "Unknown" /* Literal: Unknown */
#define L_APP "Application" /* Literal: Application */
//  End TwiBus::ScanBus defs



/////////////////////////////////////////////////////////////////////////////
////////////                    End settings                     ////////////
/////////////////////////////////////////////////////////////////////////////

#endif /* _NBMICRO_CONFIG_H_ */
