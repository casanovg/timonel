/*
  libconfig.h
  =================================
  Timonel TWI Master configuration
  ---------------------------------
  2019-02-07 Gustavo Casanova
  ---------------------------------
*/

#ifndef _TML_TWIMCONFIG_H_
#define _TML_TWIMCONFIG_H_

/////////////////////////////////////////////////////////////////////////////
////////////                TimonelTWIM settings                 ////////////
/////////////////////////////////////////////////////////////////////////////

// General defs
#define DEBUG_LEVEL 0       /* Debug level 0: No debug info over serial terminal */
#define USE_SERIAL Serial   /* Console output */
#define FEATURES_CODE 162   /* Features enebled (NOTE: This must match the bootloader, If you aren't sure, keep 255 to include all!) */
#define LOW_TML_ADDR 8      /* Lowest allowed TWI address for Timonel devices */
#define HIG_TML_ADDR 35     /* Highest allowed TWI address for Timonel devices */
#define T_SIGNATURE 84      /* Timonel signature "T" */
#define MST_PACKET_SIZE 16  /* Master-to-Slave Xmit data block size: always even values, min = 2, max = 8 */
#define SLV_PACKET_SIZE 16  /* Slave-to-Master Xmit data block size: always even values, min = 2, max = 8 */
#define PAGE_SIZE 64        /* Tiny85 flash page buffer size */
#define OK 0                /* No error in function execution */
// End General defs

// Timonel::QueryStatus defs
#define CMD_ACK_POS 0       /* Command acknowledge reply position */
#define T_SIGNATURE 84      /* Timonel signature "T" */
// *** Status reply (10 bytes)
#define S_REPLY_LENGTH 10   /* Timonel status reply lenght (1 bit ACK + 8 status bytes + 1 checksum byte) */
#define S_SIGNATURE 1       /* Status: signature byte position */
#define S_MAJOR 2           /* Status: major number byte position */
#define S_MINOR 3           /* Status: minor number byte position */
#define S_FEATURES 4        /* Status: available features code byte position */
#define S_BOOT_ADDR_MSB 5   /* Status: Timonel start address MSB position */
#define S_BOOT_ADDR_LSB 6   /* Status: Timonel start address LSB position */
#define S_APPL_ADDR_MSB 7   /* Status: Application address MSB position */
#define S_APPL_ADDR_LSB 8   /* Status: Application address LSB position */
#define S_OSCCAL 9          /* Status: AVR low fuse value byte position */
#define S_CHECK_EMPTY_FL 9  /* Status: Check empty flash value byte position */
// *** Features byte (8 bits)
#define F_ENABLE_LED_UI 0   /* Features 1 (1)  : LED UI enabled */
#define F_AUTO_TPL_CALC 1   /* Features 2 (2)  : Automatic trampoline and addr handling */
#define F_APP_USE_TPL_PG 2  /* Features 3 (4)  : Application can use trampoline page */
#define F_CMD_STPGADDR 3    /* Features 4 (8)  : Set page address command enabled */
#define F_TWO_STEP_INIT 4   /* Features 5 (16) : Two-step initialization required */
#define F_USE_WDT_RESET 5   /* Features 6 (32) : Reset by watchdog timer enabled */
#define F_CHECK_EMPTY_FL 6  /* Features 7 (64) : Empty flash checking enabled */
#define F_CMD_READFLASH 7   /* Features 8 (128): Read flash command enabled */
// End Timonel::QueryStatus defs

// Timonel::FillSpecialPage defs
#define RST_PAGE 1          /* Config: 1=Reset page (addr: 0) */
#define TPL_PAGE 2          /* Config: 2=Trampoline page (addr: TIMONEL_START - 64) */
#define DLY_SET_ADDR 100    /* Delay after setting a memory address */
#define DLY_RETURN 100      /* Delay before return control to caller */
// End Timonel::FillSpecialPage defs

// Timonel::DumpMemory defs
#define MCU_TOTAL_MEM 8192  /* Config: Microcontroller flash memory size */
#define VALUES_PER_LINE 32  /* Config: Memory positions values to display per line */
#define D_CMD_LENGTH 5      /* Config: DumpMemory command lenght (1 cmd byte + 2 addr bytes + 1 rx size byte + 1 checksum byte) */
#define MAXCKSUMERRORS 3    /* Config: DumpMemory max count of errors accepted */
#define ERR_NOT_SUPP 1      /* Error: function not supported in current setup */
#define ERR_CMD_PARSE_D 2   /* Error: reply doesn't match DumpMemory command */
#define ERR_CHECKSUM_D 3    /* Error: Too much checksum errors */
#define DLY_1_SECOND 1000   /* 1 second delay */
#define DLY_PKT_REQUEST 150 /* Delay between data packet requests */
// End Timonel::DumpMemory

// Timonel::SendDataPacket defs
#define ERR_TX_PKT_CHKSUM 1 /* Error: Received checksum doesn't match transmitted packet */
// End Timonel::SendDataPacket defs

// Timonel::UploadApplication defs
#define DLY_PKT_SEND 10     /* Delay after sending a data packet */
#define DLY_FLASH_PG 100    /* Delay to allow memory page flashing */
// End Timonel::UploadApplication defs

/////////////////////////////////////////////////////////////////////////////
////////////                    End settings                     ////////////
/////////////////////////////////////////////////////////////////////////////

#endif /* _TML_TWIMCONFIG_H_ */