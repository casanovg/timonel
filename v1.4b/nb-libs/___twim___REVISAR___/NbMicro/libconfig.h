/*
  libconfig.h
  =================================
  NbMicro configuration
  ---------------------------------
  Version: 0.9.0 2020-04-29
  gustavo.casanova@nicebots.com
  ---------------------------------
*/

#ifndef _NBMICRO_CONFIG_H_
#define _NBMICRO_CONFIG_H_

/////////////////////////////////////////////////////////////////////////////
////////////                  NbMicro settings                   ////////////
/////////////////////////////////////////////////////////////////////////////

// General defs
#define LOW_TWI_ADDR 8  /* Lowest allowed TWI address on slave devices */
#define HIG_TWI_ADDR 63 /* Highest allowed TWI address on slave devices */
//#define OK 0                /* No error in function execution */
//#define T_SIGNATURE 84      /* Timonel signature "T" */
// End General defs

// NbMicro::constructor defs
#define DLY_NBMICRO 500 /* Delay before canceling NbMiccro object creation (ms) */
// End NbMicro::constructor defs

// NbMicro::SetTwiAddress defs
#define ERR_ADDR_IN_USE 1 /* Error: The TWI address is already taken */
// End NbMicro::SetTwiAddress defs

// NbMicro::TwiCmdXmit defs
#define STOP_ON_REQ true  /* Config: true=master releases the bus with "stop" after a request, false=sends restart */
#define ERR_CMD_PARSE_S 1 /* Error: reply doesn't match command (single byte) */
#define ERR_CMD_PARSE_M 2 /* Error: reply doesn't match command (multi byte) */
// End NbMicro::TwiCmdXmit defs

/////////////////////////////////////////////////////////////////////////////
////////////                    End settings                     ////////////
/////////////////////////////////////////////////////////////////////////////

#endif /* _NBMICRO_CONFIG_H_ */
