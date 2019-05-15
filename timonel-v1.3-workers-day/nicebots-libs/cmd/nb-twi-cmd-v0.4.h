/*
 *  Nicebots I2C Command Set
 *  ................................
 *  Author: Gustavo Casanova
 *  ................................
 *  Version: 0.4 / 2018-10-17
 *  gustavo.casanova@nicebots.com
 */

// https://drive.google.com/open?id=1n_uIBW8hUX71mKudwf0kw07vjplpwsV9

#ifndef _NB_I2C_CMD_H_
#define _NB_I2C_CMD_H_

// Hardware Commands
#define STDPB1_1        0xE9                /* Command to Set ATtiny85 PB1 = 1 */
#define AKDPB1_1        0x16                /* Acknowledge PB1 = 1 Command */
#define STDPB1_0        0xE1                /* Command to Set ATtiny85 PB1 = 0 */
#define AKDPB1_0        0x1E                /* Acknowledge PB1 = 0 Command */
#define STANAPB3        0xFB                /* Command to Set ATtiny85 PB3 = PWMx */
#define ACKANPB3        0x04                /* Acknowledge PB3 = PWMx Command */
#define READADC2        0xDA                /* Command to Read ATtiny85 ADC2 */
#define ACKNADC2        0x25                /* Acknowledge Read ADC2 Command */

// General Commands
#define NOP             0x00                /* Command NOP */
#define UNKNOWNC        0xFF                /* No-Ack / UNKNOWNC */
#define RESETINY        0x80                /* Command to Reset ATtiny85 */
#define ACKRESTY        0x7F                /* Acknowledge Reset Command */
#define INITTINY        0x81                /* Command to Initialize ATtiny85 */
#define ACKINITY        0x7E                /* Acknowledge Initialize Command */
#define GET_INFO        0x82                /* Command to Read Generic Info */
#define ACK_GETI        0x7D                /* Acknowledge Read Info Command */
#define REL_ANDT        0x83                /* Command to Release Analog Data on Hold */
#define ACK_RELD        0x7C                /* Acknowledge Release Data Command */
#define FIXPOSIT        0x84                /* Fix Positive half-cycles for ADC Vrms calculations */
#define ACKFXPOS        0x7B                /* Acknowledge Fix Positive Command */
#define FIXNEGAT        0x85                /* Fix Negative half-cycles for ADC Vrms calculations */
#define ACKFXNEG        0x7A                /* Acknowledge Fix Negative Command */

// Transfer & Self-Programming Commands
#define READBUFF        0xA1                /* Command to Read 10-bit Data Buffer */
#define ACKRDBUF        0x5E                /* Acknowledge Read 10-bit Data Buffer Command */
#define WRITBUFF        0xA2                /* Command to Write 10-bit Data Buffer */
#define ACKWTBUF        0x5D                /* Acknowledge Write 10-bit Data Buffer Command */
#define GETTMNLV        0xA3                /* Command to Get the Timonel version (bootloader) */
#define ACKTMNLV        0x5C                /* Acknowledge Get Timonel version Command */
#define DELFLASH        0xA4                /* Command to Delete Flash memory */
#define ACKDELFL        0x5B                /* Acknowledge Delete Flash Command */
#define STPGADDR        0xA5                /* Command to Set Page Base Address */
#define AKPGADDR        0x5A                /* Acknowledge Set Page Address Command */
#define READPAGE        0xA6                /* Command to Read AVR 8-bit Page Buffer */
#define ACKRDPAG        0x59                /* Acknowledge Read AVR 8-bit Page Buffer Command */
#define WRITPAGE        0xA7                /* Command to Write AVR 8-bit Page Buffer */
#define ACKWTPAG        0x58                /* Acknowledge Write AVR 8-bit Page Buffer Command */
#define EXITTMNL        0xA8                /* Command to Exit Timonel (run application)    */
#define ACKEXITT        0x57                /* Acknowledge Exit Timonel Command */
#define READFLSH        0x56                /* Command to Read Data from AVR Flash Memory */
#define ACKRDFSH        0xA9                /* Acknowledge Read from AVR Flash Memory Command */

#endif /* _NB_I2C_CMD_H_ */
