/*
  libconfig.h
  =================================
  Timonel TWI Master configuration
  ---------------------------------
  Version: 1.0.1 / 2020-05-07
  gustavo.casanova@gmail.com
  ---------------------------------
*/

#ifndef _TML_TWIMCONFIG_H_
#define _TML_TWIMCONFIG_H_

/////////////////////////////////////////////////////////////////////////////
////////////                TimonelTwiM settings                 ////////////
/////////////////////////////////////////////////////////////////////////////

// General defs
#define DEBUG_LEVEL 0      /* Debug level: 0 = No debug info over serial terminal, 1+ = Progressively increasing verbosity */
#define USE_SERIAL Serial  /* Console output */
#define FEATURES_CODE 253  /* Enabled features (NOTE: This must match the bootloader, If you aren't sure, use 253 (default) */
#define EXT_FEATURES 15    /* Enabled extended features (NOTE: This must match the bootloader, If you aren't sure, use 15 */
#define LOW_TML_ADDR 8     /* Lowest allowed TWI address for Timonel devices */
#define HIG_TML_ADDR 35    /* Highest allowed TWI address for Timonel devices */
#define T_SIGNATURE 84     /* Timonel signature "T" (Uppercase means clock tweaking made at compile time*/
#define MST_PACKET_SIZE 32 /* Master-to-Slave Xmit data block size: always even values, min = 2, max = 32 */
#define SLV_PACKET_SIZE 32 /* Slave-to-Master Xmit data block size: always even values, min = 2, max = 32 */
#define SPM_PAGESIZE 64    /* Tiny85 flash page buffer size */
#define OK 0               /* No error in function execution */
// End General defs

// Timonel::QueryStatus defs
#define CMD_ACK_POS 0 /* Command acknowledge reply position */
// *** Status reply (10 bytes)
#define S_REPLY_LENGTH 12 /* Timonel status reply lenght (1 bit ACK + 8 status bytes + 1 checksum byte) */
#define S_SIGNATURE 1     /* Status: signature byte position */
#define S_MAJOR 2         /* Status: major number byte position */
#define S_MINOR 3         /* Status: minor number byte position */
#define S_FEATURES 4      /* Status: available features code byte position */
#define S_EXT_FEATURES 5  /* Status: available features code byte position */
#define S_BOOT_ADDR_MSB 6 /* Status: Timonel start address MSB position */
#define S_BOOT_ADDR_LSB 7 /* Status: Timonel start address LSB position */
#define S_APPL_ADDR_MSB 8 /* Status: Application address MSB position */
#define S_APPL_ADDR_LSB 9 /* Status: Application address LSB position */
#define S_LOW_FUSE 10     /* Status: Low fuse setting */
#define S_OSCCAL 11       /* Status: AVR low fuse value byte position */
// *** Features byte (8 bits)
#define F_ENABLE_LED_UI 0  /* Features 1 (1)  : LED UI enabled */
#define F_AUTO_PAGE_ADDR 1 /* Features 2 (2)  : Automatic trampoline and addr handling */
#define F_APP_USE_TPL_PG 2 /* Features 3 (4)  : Application can use trampoline page */
#define F_CMD_SETPGADDR 3  /* Features 4 (8)  : Set page address command enabled */
#define F_TWO_STEP_INIT 4  /* Features 5 (16) : Two-step initialization required */
#define F_USE_WDT_RESET 5  /* Features 6 (32) : Reset by watchdog timer enabled */
#define F_TIMEOUT_EXIT 6   /* Features 7 (64) : If not initialized, exit to app after timeout */
#define F_CMD_READFLASH 7  /* Features 8 (128): Read flash command enabled */
// *** Extended features byte (8 bits)
#define F_AUTO_CLK_TWEAK 0  /* Ext features 1 (1)  : Auto clock tweaking by reading low fuse */
#define F_FORCE_ERASE_PG 1  /* Ext features 2 (2)  : Erase each page before writing new data */
#define F_CLEAR_BIT_7_R31 2 /* Ext features 3 (4)  : Prevent code first instruction from being skipped */
#define F_CHECK_PAGE_IX 3   /* Ext features 4 (8)  : Check that the page index is < SPM_PAGESIZE */
// Extended features 5 to 8 not used
// End Timonel::QueryStatus defs

// Timonel::FillSpecialPage defs
#define RST_PAGE 1       /* Config: 1=Reset page (addr: 0) */
#define TPL_PAGE 2       /* Config: 2=Trampoline page (addr: TIMONEL_START - 64) */
#define DLY_SET_ADDR 100 /* Delay after setting a memory address */
#define DLY_RETURN 100   /* Delay before return control to caller */
// End Timonel::FillSpecialPage defs

// Timonel::DumpMemory defs
#define MCU_TOTAL_MEM 8192  /* Config: Microcontroller flash memory size */
#define VALUES_PER_LINE 32  /* Config: Memory positions values to display per line */
#define D_CMD_LENGTH 4      /* Config: READFLSH command lenght (1 cmd byte + 2 addr bytes + 1 rx size byte + 1 checksum byte) */
#define D_REPLY_OVRHD 2     /* Config: READFLSH reply overhead: extra bytes added to the reply: 1 ack + 1 checksum */
#define MAXCKSUMERRORS 3    /* Config: DumpMemory max count of errors accepted */
#define ERR_NOT_SUPP 1      /* Error: function not supported in current setup */
#define ERR_CMD_PARSE_D 2   /* Error: reply doesn't match DumpMemory command */
#define ERR_ADDR_PARSE 3    /* Error: requested address misinterpreted by Timonel device */
#define ERR_CHECKSUM_D 4    /* Error: Too much checksum errors */
#define DLY_1_SECOND 1000   /* 1 second delay */
#define DLY_PKT_REQUEST 150 /* Delay between data packet requests */
// End Timonel::DumpMemory

// Timonel::SendDataPacket defs
#define ERR_TX_PKT_CHKSUM 1 /* Error: Received checksum doesn't match transmitted packet */
// End Timonel::SendDataPacket defs

// Timonel::UploadApplication defs
#define DLY_PKT_SEND 10  /* Delay after sending a data packet */
#define DLY_FLASH_PG 100 /* Delay to allow memory page flashing */
#define TRAMPOLINE_LEN 2 /* Trampoline length: two-byte address to jump to the app */
#define ERR_SETADDRESS 1 /* Error: AUTO_PAGE_ADDR and CMD_SETPGADDR are disabled, can't set page addresses */
#define ERR_APP_OVF_AU 2 /* Error: the payload doesn't fit in AVR memory (auto page addr calculation) */
#define ERR_APP_OVF_MC 3 /* Error: the payload doesn't fit in AVR memory (page addr calculated by TWI master) */
#define ERR_AUTO_CALC 4  /* Error: AUTO_PAGE_ADDR is disabled and the addr handling cade is not included in TWI master */
// End Timonel::UploadApplication defs

// Timonel::DeleteApplication defs
#define DLY_DEL_INIT 750 /* Delay to allow deleting the app before initializing Timonel */
// End Timonel::DeleteApplication defs

/////////////////////////////////////////////////////////////////////////////
////////////                    End settings                     ////////////
/////////////////////////////////////////////////////////////////////////////

#endif /* _TML_TWIMCONFIG_H_ */
