// ********************************************************
// *  Nicebots TWI Command Set                            *
// *  ========================                            *
// *  This file defines the comm protocol over TWI (I2C)  *
// *  ..................................................  *
// *  Author: Gustavo Casanova  / Nicebots                *
// *  ..................................................  *
// *  Version: 0.7.0                                      *
// *  2020-06-21 gustavo.casanova@nicebots.com            *
// ********************************************************

#ifndef NB_TWI_CMD_H
#define NB_TWI_CMD_H

// https://github.com/casanovg/nb-twi-cmd/raw/master/extras/Nicebots-Pluggie-I2C-Commands.xlsx

#define NO_OP 0x00 /* Command No Operation - Unknown */
#define UNKNOWNC 0xFF /* Acknowledge No Operation - Unknown command */
#define RESETMCU 0x80 /* Command Reset Microcontroller */
#define ACKRESET 0x7F /* Acknowledge Reset Microcontroller command */
#define INITSOFT 0x81 /* Command Initialize Firmware */
#define ACKINITS 0x7E /* Acknowledge Initialize Firmware command */
#define GETTMNLV 0x82 /* Command Get Timonel Version */
#define ACKTMNLV 0x7D /* Acknowledge Get Timonel Version command */
#define DELFLASH 0x83 /* Command Delete Flash */
#define ACKDELFL 0x7C /* Acknowledge Delete Flash command */
#define STPGADDR 0x84 /* Command Set Page Base Address */
#define AKPGADDR 0x7B /* Acknowledge Set Page Base Address command */
#define WRITPAGE 0x85 /* Command Write Data To Page Buffer */
#define ACKWTPAG 0x7A /* Acknowledge Write Data To Page Buffer command */
#define EXITTMNL 0x86 /* Command Exit Timonel (Jump To App) */
#define ACKEXITT 0x79 /* Acknowledge Exit Timonel (Jump To App) command */
#define READFLSH 0x87 /* Command Read Data From Flash Memory */
#define ACKRDFSH 0x78 /* Acknowledge Read Data From Flash Memory command */
#define READDEVS 0x88 /* Command Read Device Signature */
#define ACKRDEVS 0x77 /* Acknowledge Read Device Signature command */

#define SETIO1_0 0x92 /* Command Set Io Port 1 = 0 */
#define ACKIO1_0 0x6D /* Acknowledge Set Io Port 1 = 0 command */
#define SETIO1_1 0x93 /* Command Set Io Port 1 = 1 */
#define ACKIO1_1 0x6C /* Acknowledge Set Io Port 1 = 1 command */

#define SETANA03 0xBB /* Command Set Analog Port 3 = Pwmx */
#define ACKANA03 0x44 /* Acknowledge Set Analog Port 3 = Pwmx command */

#define READADC2 0xCE /* Command Read Adc2 */
#define ACKNADC2 0x31 /* Acknowledge Read Adc2 command */

#define INFORMAT 0xD6 /* Command Retrieve General Info */
#define ACKINFOR 0x29 /* Acknowledge Retrieve General Info command */
#define RELANDAT 0xD7 /* Command Release Analog Data */
#define ACKRELAD 0x28 /* Acknowledge Release Analog Data command */
#define FIXPOSIT 0xD8 /* Command Fix Positive Half-Cycles For Adc */
#define ACKFXPOS 0x27 /* Acknowledge Fix Positive Half-Cycles For Adc command */
#define FIXNEGAT 0xD9 /* Command Fix Negative Half-Cycles For Adc */
#define ACKFXNEG 0x26 /* Acknowledge Fix Negative Half-Cycles For Adc command */
#define READBUFF 0xDA /* Command Read Data From App Fw Buffer */
#define ACKRDBUF 0x25 /* Acknowledge Read Data From App Fw Buffer command */
#define WRITBUFF 0xDB /* Command Write Data To App Fw Buffer */
#define ACKWTBUF 0x24 /* Acknowledge Write Data To App Fw Buffer command */

#endif  // NB_TWI_CMD_H
