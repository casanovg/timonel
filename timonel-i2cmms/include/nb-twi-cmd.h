
// ********************************************************
// *  Nicebots TWI Command Set                            *
// *  ========================                            *
// *  This file defines the inter-MCU protocol            *
// *  ..................................................  *
// *  Author: Gustavo Casanova  / Nicebots                *
// *  ..................................................  *
// *  Version: 0.5                                        *
// *  2019-03-13 gustavo.casanova@nicebots.com            *
// ********************************************************

#ifndef _NB_I2C_CMD_H_
#define _NB_I2C_CMD_H_

// https://drive.google.com/open?id=1pu6hOd0GmKWU5qMEgDtN57TDMxKzHRv6

// Bootloader Commands
#define NOP 0x00 /* Command Nop - Unknown */
#define UNKNOWNC 0xFF /* Acnowledge Nop - Unknown command */
#define RESETMCU 0x80 /* Command Reset Microcontroller */
#define ACKRESET 0x7F /* Acnowledge Reset Microcontroller command */
#define INITSOFT 0x81 /* Command Initialize Firmware */
#define ACKINITS 0x7E /* Acnowledge Initialize Firmware command */
#define GETTMNLV 0x82 /* Command Get Timonel Version */
#define ACKTMNLV 0x7D /* Acnowledge Get Timonel Version command */
#define DELFLASH 0x83 /* Command Delete Flash */
#define ACKDELFL 0x7C /* Acnowledge Delete Flash command */
#define STPGADDR 0x84 /* Command Set Page Base Address */
#define AKPGADDR 0x7B /* Acnowledge Set Page Base Address command */
#define WRITPAGE 0x85 /* Command Write Data To Page Buffer */
#define ACKWTPAG 0x7A /* Acnowledge Write Data To Page Buffer command */
#define EXITTMNL 0x86 /* Command Exit Timonel (Jump To App) */
#define ACKEXITT 0x79 /* Acnowledge Exit Timonel (Jump To App) command */
#define READFLSH 0x87 /* Command Read Data From Flash Memory */
#define ACKRDFSH 0x78 /* Acnowledge Read Data From Flash Memory command */
#define BLRCMD01 0x88 /* Command Not Used Bootloader Cmd 01 */
#define ACKBLC01 0x77 /* Acnowledge Not Used Bootloader Cmd 01 command */
#define BLRCMD02 0x89 /* Command Not Used Bootloader Cmd 02 */
#define ACKBLC02 0x76 /* Acnowledge Not Used Bootloader Cmd 02 command */
#define BLRCMD03 0x8A /* Command Not Used Bootloader Cmd 03 */
#define ACKBLC03 0x75 /* Acnowledge Not Used Bootloader Cmd 03 command */
#define BLRCMD04 0x8B /* Command Not Used Bootloader Cmd 04 */
#define ACKBLC04 0x74 /* Acnowledge Not Used Bootloader Cmd 04 command */
#define BLRCMD05 0x8C /* Command Not Used Bootloader Cmd 05 */
#define ACKBLC05 0x73 /* Acnowledge Not Used Bootloader Cmd 05 command */
#define BLRCMD06 0x8D /* Command Not Used Bootloader Cmd 06 */
#define ACKBLC06 0x72 /* Acnowledge Not Used Bootloader Cmd 06 command */
#define BLRCMD07 0x8E /* Command Not Used Bootloader Cmd 07 */
#define ACKBLC07 0x71 /* Acnowledge Not Used Bootloader Cmd 07 command */
#define BLRCMD08 0x8F /* Command Not Used Bootloader Cmd 08 */
#define ACKBLC08 0x70 /* Acnowledge Not Used Bootloader Cmd 08 command */

// Hardware Peripheral Commands
#define SETIO0_0 0x90 /* Command Set Io Port 0 = 0 */
#define ACKIO0_0 0x6F /* Acnowledge Set Io Port 0 = 0 command */
#define SETIO0_1 0x91 /* Command Set Io Port 0 = 1 */
#define ACKIO0_1 0x6E /* Acnowledge Set Io Port 0 = 1 command */
#define SETIO1_0 0x92 /* Command Set Io Port 1 = 0 */
#define ACKIO1_0 0x6D /* Acnowledge Set Io Port 1 = 0 command */
#define SETIO1_1 0x93 /* Command Set Io Port 1 = 1 */
#define ACKIO1_1 0x6C /* Acnowledge Set Io Port 1 = 1 command */
#define SETIO2_0 0x94 /* Command Set Io Port 2 = 0 */
#define ACKIO2_0 0x6B /* Acnowledge Set Io Port 2 = 0 command */
#define SETIO2_1 0x95 /* Command Set Io Port 2 = 1 */
#define ACKIO2_1 0x6A /* Acnowledge Set Io Port 2 = 1 command */
#define SETIO3_0 0x96 /* Command Set Io Port 3 = 0 */
#define ACKIO3_0 0x69 /* Acnowledge Set Io Port 3 = 0 command */
#define SETIO3_1 0x97 /* Command Set Io Port 3 = 1 */
#define ACKIO3_1 0x68 /* Acnowledge Set Io Port 3 = 1 command */
#define SETIO4_0 0x98 /* Command Set Io Port 4 = 0 */
#define ACKIO4_0 0x67 /* Acnowledge Set Io Port 4 = 0 command */
#define SETIO4_1 0x99 /* Command Set Io Port 4 = 1 */
#define ACKIO4_1 0x66 /* Acnowledge Set Io Port 4 = 1 command */
#define SETIO5_0 0x9A /* Command Set Io Port 5 = 0 */
#define ACKIO5_0 0x65 /* Acnowledge Set Io Port 5 = 0 command */
#define SETIO5_1 0x9B /* Command Set Io Port 5 = 1 */
#define ACKIO5_1 0x64 /* Acnowledge Set Io Port 5 = 1 command */
#define SETIO6_0 0x9C /* Command Set Io Port 6 = 0 */
#define ACKIO6_0 0x63 /* Acnowledge Set Io Port 6 = 0 command */
#define SETIO6_1 0x9D /* Command Set Io Port 6 = 1 */
#define ACKIO6_1 0x62 /* Acnowledge Set Io Port 6 = 1 command */
#define SETIO7_0 0x9E /* Command Set Io Port 7 = 0 */
#define ACKIO7_0 0x61 /* Acnowledge Set Io Port 7 = 0 command */
#define SETIO7_1 0x9F /* Command Set Io Port 7 = 1 */
#define ACKIO7_1 0x60 /* Acnowledge Set Io Port 7 = 1 command */
#define SETIO8_0 0xA0 /* Command Set Io Port 8 = 0 */
#define ACKIO8_0 0x5F /* Acnowledge Set Io Port 8 = 0 command */
#define SETIO8_1 0xA1 /* Command Set Io Port 8 = 1 */
#define ACKIO8_1 0x5E /* Acnowledge Set Io Port 8 = 1 command */
#define SETIO9_0 0xA2 /* Command Set Io Port 9 = 0 */
#define ACKIO9_0 0x5D /* Acnowledge Set Io Port 9 = 0 command */
#define SETIO9_1 0xA3 /* Command Set Io Port 9 = 1 */
#define ACKIO9_1 0x5C /* Acnowledge Set Io Port 9 = 1 command */
#define SETIO100 0xA4 /* Command Set Io Port 10 = 0 */
#define ACKIO100 0x5B /* Acnowledge Set Io Port 10 = 0 command */
#define SETIO101 0xA5 /* Command Set Io Port 10 = 1 */
#define ACKIO101 0x5A /* Acnowledge Set Io Port 10 = 1 command */
#define SETIO110 0xA6 /* Command Set Io Port 11 = 0 */
#define ACKIO110 0x59 /* Acnowledge Set Io Port 11 = 0 command */
#define SETIO111 0xA7 /* Command Set Io Port 11 = 1 */
#define ACKIO111 0x58 /* Acnowledge Set Io Port 11 = 1 command */
#define SETIO120 0xA8 /* Command Set Io Port 12 = 0 */
#define ACKIO120 0x57 /* Acnowledge Set Io Port 12 = 0 command */
#define SETIO121 0xA9 /* Command Set Io Port 12 = 1 */
#define ACKIO121 0x56 /* Acnowledge Set Io Port 12 = 1 command */
#define SETIO130 0xAA /* Command Set Io Port 13 = 0 */
#define ACKIO130 0x55 /* Acnowledge Set Io Port 13 = 0 command */
#define SETIO131 0xAB /* Command Set Io Port 13 = 1 */
#define ACKIO131 0x54 /* Acnowledge Set Io Port 13 = 1 command */
#define SETIO140 0xAC /* Command Set Io Port 14 = 0 */
#define ACKIO140 0x53 /* Acnowledge Set Io Port 14 = 0 command */
#define SETIO141 0xAD /* Command Set Io Port 14 = 1 */
#define ACKIO141 0x52 /* Acnowledge Set Io Port 14 = 1 command */
#define SETIO150 0xAE /* Command Set Io Port 15 = 0 */
#define ACKIO150 0x51 /* Acnowledge Set Io Port 15 = 0 command */
#define SETIO151 0xAF /* Command Set Io Port 15 = 1 */
#define ACKIO151 0x50 /* Acnowledge Set Io Port 15 = 1 command */
#define SETIO160 0xB0 /* Command Set Io Port 16 = 0 */
#define ACKIO160 0x4F /* Acnowledge Set Io Port 16 = 0 command */
#define SETIO161 0xB1 /* Command Set Io Port 16 = 1 */
#define ACKIO161 0x4E /* Acnowledge Set Io Port 16 = 1 command */
#define SETIO170 0xB2 /* Command Set Io Port 17 = 0 */
#define ACKIO170 0x4D /* Acnowledge Set Io Port 17 = 0 command */
#define SETIO171 0xB3 /* Command Set Io Port 17 = 1 */
#define ACKIO171 0x4C /* Acnowledge Set Io Port 17 = 1 command */
#define SETIO180 0xB4 /* Command Set Io Port 18 = 0 */
#define ACKIO180 0x4B /* Acnowledge Set Io Port 18 = 0 command */
#define SETIO181 0xB5 /* Command Set Io Port 18 = 1 */
#define ACKIO181 0x4A /* Acnowledge Set Io Port 18 = 1 command */
#define SETIO190 0xB6 /* Command Set Io Port 19 = 0 */
#define ACKIO190 0x49 /* Acnowledge Set Io Port 19 = 0 command */
#define SETIO191 0xB7 /* Command Set Io Port 19 = 1 */
#define ACKIO191 0x48 /* Acnowledge Set Io Port 19 = 1 command */
#define SETANA00 0xB8 /* Command Set Analog Port 0 = Pwmx */
#define ACKANA00 0x47 /* Acnowledge Set Analog Port 0 = Pwmx command */
#define SETANA01 0xB9 /* Command Set Analog Port 1 = Pwmx */
#define ACKANA01 0x46 /* Acnowledge Set Analog Port 1 = Pwmx command */
#define SETANA02 0xBA /* Command Set Analog Port 2 = Pwmx */
#define ACKANA02 0x45 /* Acnowledge Set Analog Port 2 = Pwmx command */
#define SETANA03 0xBB /* Command Set Analog Port 3 = Pwmx */
#define ACKANA03 0x44 /* Acnowledge Set Analog Port 3 = Pwmx command */
#define SETANA04 0xBC /* Command Set Analog Port 4 = Pwmx */
#define ACKANA04 0x43 /* Acnowledge Set Analog Port 4 = Pwmx command */
#define SETANA05 0xBD /* Command Set Analog Port 5 = Pwmx */
#define ACKANA05 0x42 /* Acnowledge Set Analog Port 5 = Pwmx command */
#define SETANA06 0xBE /* Command Set Analog Port 6 = Pwmx */
#define ACKANA06 0x41 /* Acnowledge Set Analog Port 6 = Pwmx command */
#define SETANA07 0xBF /* Command Set Analog Port 7 = Pwmx */
#define ACKANA07 0x40 /* Acnowledge Set Analog Port 7 = Pwmx command */
#define SETANA08 0xC0 /* Command Set Analog Port 8 = Pwmx */
#define ACKANA08 0x3F /* Acnowledge Set Analog Port 8 = Pwmx command */
#define SETANA09 0xC1 /* Command Set Analog Port 9 = Pwmx */
#define ACKANA09 0x3E /* Acnowledge Set Analog Port 9 = Pwmx command */
#define SETANA10 0xC2 /* Command Set Analog Port 10 = Pwmx */
#define ACKANA10 0x3D /* Acnowledge Set Analog Port 10 = Pwmx command */
#define SETANA11 0xC3 /* Command Set Analog Port 11 = Pwmx */
#define ACKANA11 0x3C /* Acnowledge Set Analog Port 11 = Pwmx command */
#define SETANA12 0xC4 /* Command Set Analog Port 12 = Pwmx */
#define ACKANA12 0x3B /* Acnowledge Set Analog Port 12 = Pwmx command */
#define SETANA13 0xC5 /* Command Set Analog Port 13 = Pwmx */
#define ACKANA13 0x3A /* Acnowledge Set Analog Port 13 = Pwmx command */
#define SETANA14 0xC6 /* Command Set Analog Port 14 = Pwmx */
#define ACKANA14 0x39 /* Acnowledge Set Analog Port 14 = Pwmx command */
#define SETANA15 0xC7 /* Command Set Analog Port 15 = Pwmx */
#define ACKANA15 0x38 /* Acnowledge Set Analog Port 15 = Pwmx command */
#define SETANA16 0xC8 /* Command Set Analog Port 16 = Pwmx */
#define ACKANA16 0x37 /* Acnowledge Set Analog Port 16 = Pwmx command */
#define SETANA17 0xC9 /* Command Set Analog Port 17 = Pwmx */
#define ACKANA17 0x36 /* Acnowledge Set Analog Port 17 = Pwmx command */
#define SETANA18 0xCA /* Command Set Analog Port 18 = Pwmx */
#define ACKANA18 0x35 /* Acnowledge Set Analog Port 18 = Pwmx command */
#define SETANA19 0xCB /* Command Set Analog Port 19 = Pwmx */
#define ACKANA19 0x34 /* Acnowledge Set Analog Port 19 = Pwmx command */
#define READADC0 0xCC /* Command Read Adc0 */
#define ACKNADC0 0x33 /* Acnowledge Read Adc0 command */
#define READADC1 0xCD /* Command Read Adc1 */
#define ACKNADC1 0x32 /* Acnowledge Read Adc1 command */
#define READADC2 0xCE /* Command Read Adc2 */
#define ACKNADC2 0x31 /* Acnowledge Read Adc2 command */
#define READADC3 0xCF /* Command Read Adc3 */
#define ACKNADC3 0x30 /* Acnowledge Read Adc3 command */
#define READADC4 0xD0 /* Command Read Adc4 */
#define ACKNADC4 0x2F /* Acnowledge Read Adc4 command */
#define READADC5 0xD1 /* Command Read Adc5 */
#define ACKNADC5 0x2E /* Acnowledge Read Adc5 command */
#define READADC6 0xD2 /* Command Read Adc6 */
#define ACKNADC6 0x2D /* Acnowledge Read Adc6 command */
#define READADC7 0xD3 /* Command Read Adc7 */
#define ACKNADC7 0x2C /* Acnowledge Read Adc7 command */
#define READADC8 0xD4 /* Command Read Adc8 */
#define ACKNADC8 0x2B /* Acnowledge Read Adc8 command */
#define READADC9 0xD5 /* Command Read Adc9 */
#define ACKNADC9 0x2A /* Acnowledge Read Adc9 command */

// General Application Commands (Free command range for user applications)
#define INFORMAT 0xD6 /* Command Retrieve General Info */
#define ACKINFOR 0x29 /* Acnowledge Retrieve General Info command */
#define RELANDAT 0xD7 /* Command Release Analog Data */
#define ACKRELAD 0x28 /* Acnowledge Release Analog Data command */
#define FIXPOSIT 0xD8 /* Command Fix Positive Half-Cycles For Adc */
#define ACKFXPOS 0x27 /* Acnowledge Fix Positive Half-Cycles For Adc command */
#define FIXNEGAT 0xD9 /* Command Fix Negative Half-Cycles For Adc */
#define ACKFXNEG 0x26 /* Acnowledge Fix Negative Half-Cycles For Adc command */
#define READBUFF 0xDA /* Command Read Data From App Fw Buffer */
#define ACKRDBUF 0x25 /* Acnowledge Read Data From App Fw Buffer command */
#define WRITBUFF 0xDB /* Command Write Data To App Fw Buffer */
#define ACKWTBUF 0x24 /* Acnowledge Write Data To App Fw Buffer command */
#define APPCMD01 0xDC /* Command Not Used Application Cmd 01 */
#define ACKAPC01 0x23 /* Acnowledge Not Used Application Cmd 01 command */
#define APPCMD02 0xDD /* Command Not Used Application Cmd 02 */
#define ACKAPC02 0x22 /* Acnowledge Not Used Application Cmd 02 command */
#define APPCMD03 0xDE /* Command Not Used Application Cmd 03 */
#define ACKAPC03 0x21 /* Acnowledge Not Used Application Cmd 03 command */
#define APPCMD04 0xDF /* Command Not Used Application Cmd 04 */
#define ACKAPC04 0x20 /* Acnowledge Not Used Application Cmd 04 command */
#define APPCMD05 0xE0 /* Command Not Used Application Cmd 05 */
#define ACKAPC05 0x1F /* Acnowledge Not Used Application Cmd 05 command */
#define APPCMD06 0xE1 /* Command Not Used Application Cmd 06 */
#define ACKAPC06 0x1E /* Acnowledge Not Used Application Cmd 06 command */
#define APPCMD07 0xE2 /* Command Not Used Application Cmd 07 */
#define ACKAPC07 0x1D /* Acnowledge Not Used Application Cmd 07 command */
#define APPCMD08 0xE3 /* Command Not Used Application Cmd 08 */
#define ACKAPC08 0x1C /* Acnowledge Not Used Application Cmd 08 command */
#define APPCMD09 0xE4 /* Command Not Used Application Cmd 09 */
#define ACKAPC09 0x1B /* Acnowledge Not Used Application Cmd 09 command */
#define APPCMD10 0xE5 /* Command Not Used Application Cmd 10 */
#define ACKAPC10 0x1A /* Acnowledge Not Used Application Cmd 10 command */
#define APPCMD11 0xE6 /* Command Not Used Application Cmd 11 */
#define ACKAPC11 0x19 /* Acnowledge Not Used Application Cmd 11 command */
#define APPCMD12 0xE7 /* Command Not Used Application Cmd 12 */
#define ACKAPC12 0x18 /* Acnowledge Not Used Application Cmd 12 command */
#define APPCMD13 0xE8 /* Command Not Used Application Cmd 13 */
#define ACKAPC13 0x17 /* Acnowledge Not Used Application Cmd 13 command */
#define APPCMD14 0xE9 /* Command Not Used Application Cmd 14 */
#define ACKAPC14 0x16 /* Acnowledge Not Used Application Cmd 14 command */
#define APPCMD15 0xEA /* Command Not Used Application Cmd 15 */
#define ACKAPC15 0x15 /* Acnowledge Not Used Application Cmd 15 command */
#define APPCMD16 0xEB /* Command Not Used Application Cmd 16 */
#define ACKAPC16 0x14 /* Acnowledge Not Used Application Cmd 16 command */
#define APPCMD17 0xEC /* Command Not Used Application Cmd 17 */
#define ACKAPC17 0x13 /* Acnowledge Not Used Application Cmd 17 command */
#define APPCMD18 0xED /* Command Not Used Application Cmd 18 */
#define ACKAPC18 0x12 /* Acnowledge Not Used Application Cmd 18 command */
#define APPCMD19 0xEE /* Command Not Used Application Cmd 19 */
#define ACKAPC19 0x11 /* Acnowledge Not Used Application Cmd 19 command */
#define APPCMD20 0xEF /* Command Not Used Application Cmd 20 */
#define ACKAPC20 0x10 /* Acnowledge Not Used Application Cmd 20 command */
#define APPCMD21 0xF0 /* Command Not Used Application Cmd 21 */
#define ACKAPC21 0x0F /* Acnowledge Not Used Application Cmd 21 command */
#define APPCMD22 0xF1 /* Command Not Used Application Cmd 22 */
#define ACKAPC22 0x0E /* Acnowledge Not Used Application Cmd 22 command */
#define APPCMD23 0xF2 /* Command Not Used Application Cmd 23 */
#define ACKAPC23 0x0D /* Acnowledge Not Used Application Cmd 23 command */
#define APPCMD24 0xF3 /* Command Not Used Application Cmd 24 */
#define ACKAPC24 0x0C /* Acnowledge Not Used Application Cmd 24 command */
#define APPCMD25 0xF4 /* Command Not Used Application Cmd 25 */
#define ACKAPC25 0x0B /* Acnowledge Not Used Application Cmd 25 command */
#define APPCMD26 0xF5 /* Command Not Used Application Cmd 26 */
#define ACKAPC26 0x0A /* Acnowledge Not Used Application Cmd 26 command */
#define APPCMD27 0xF6 /* Command Not Used Application Cmd 27 */
#define ACKAPC27 0x09 /* Acnowledge Not Used Application Cmd 27 command */
#define APPCMD28 0xF7 /* Command Not Used Application Cmd 28 */
#define ACKAPC28 0x08 /* Acnowledge Not Used Application Cmd 28 command */
#define APPCMD29 0xF8 /* Command Not Used Application Cmd 29 */
#define ACKAPC29 0x07 /* Acnowledge Not Used Application Cmd 29 command */
#define APPCMD30 0xF9 /* Command Not Used Application Cmd 30 */
#define ACKAPC30 0x06 /* Acnowledge Not Used Application Cmd 30 command */
#define APPCMD31 0xFA /* Command Not Used Application Cmd 31 */
#define ACKAPC31 0x05 /* Acnowledge Not Used Application Cmd 31 command */
#define APPCMD32 0xFB /* Command Not Used Application Cmd 32 */
#define ACKAPC32 0x04 /* Acnowledge Not Used Application Cmd 32 command */
#define APPCMD33 0xFC /* Command Not Used Application Cmd 33 */
#define ACKAPC33 0x03 /* Acnowledge Not Used Application Cmd 33 command */
#define APPCMD34 0xFD /* Command Not Used Application Cmd 34 */
#define ACKAPC34 0x02 /* Acnowledge Not Used Application Cmd 34 command */
#define APPCMD35 0xFE /* Command Not Used Application Cmd 35 */
#define ACKAPC35 0x01 /* Acnowledge Not Used Application Cmd 35 command */

#endif /* _NB_I2C_CMD_H_ */
