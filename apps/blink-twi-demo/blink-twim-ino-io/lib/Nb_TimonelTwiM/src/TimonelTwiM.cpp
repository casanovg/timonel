/*
 *  Timonel TWI Master Library
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: TimonelTwiM.cpp (Library)
 *  ........................................... 
 *  Version: 1.0.1 / 2020-05-07
 *  gustavo.casanova@gmail.com
 *  ...........................................
 *  This library enables uploading firmware to a microcontroller
 *  running the Timonel bootloader. It inherits from the NbMicro
 *  class to implement the NB command set over the TWI (I2C) bus.
 */

#include "TimonelTwiM.h"

#include <Arduino.h>

// Class constructor
Timonel::Timonel(const byte twi_address, const byte sda, const byte scl) : NbMicro(twi_address, sda, scl) {
    if ((addr_ >= LOW_TML_ADDR) && (addr_ <= HIG_TML_ADDR)) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Bootloader instance created with TWI address %02d.\r\n", __func__, addr_);
#endif /* DEBUG_LEVEL */
        BootloaderInit();
    } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Bootloader instance created without TWI address.\r\n", __func__);
#endif /* DEBUG_LEVEL */
    }
}

// Class destructor
Timonel::~Timonel() {
    // Destructor
}

/* _________________________
  |                         | 
  |        GetStatus        |
  |_________________________|
*/
// Return a struct with the Timonel bootloader running status
Timonel::Status Timonel::GetStatus(void) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("[%s] Getting Timonel device %02d status ...\r\n", __func__, addr_);
    byte twi_errors = QueryStatus();
    if (twi_errors > 0) {
        USE_SERIAL.printf_P("[%s] Error getting Timonel %02d status <<< %d \r\n", __func__, addr_, twi_errors);
    }
#else
    QueryStatus();
#endif /* DEBUG_LEVEL */
    return status_;
}

/* _________________________
  |                         | 
  |      SetTwiAddress      |
  |_________________________|
*/
// Set this object's TWI address (allowed only once, if it wasn't set at object creation time)
byte Timonel::SetTwiAddress(byte twi_address) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("[%s] Set Timonel object TWI address to -> %02d ...\r\n", __func__, twi_address);
#endif /* DEBUG_LEVEL */
    byte twi_errors = 0;
    twi_errors += NbMicro::SetTwiAddress(twi_address);
    twi_errors += BootloaderInit();
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    if (twi_errors > 0) {
        USE_SERIAL.printf_P("[%s] Error getting Timonel %02d status <<< %d \r\n", __func__, addr_, twi_errors);
    }
#endif /* DEBUG_LEVEL */
    QueryStatus();
    return twi_errors;
}

/* _________________________
  |                         | 
  |     RunApplication      |
  |_________________________|
*/
// Ask Timonel to stop executing and run the user application
byte Timonel::RunApplication(void) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("\n\r[%s] Exit bootloader & run application >>> 0x%02X\r\n", __func__, EXITTMNL);
#endif /* DEBUG_LEVEL */
    return (TwiCmdXmit(EXITTMNL, ACKEXITT));
}

/* _________________________
  |                         | 
  |    DeleteApplication    |
  |_________________________|
*/
// Ask Timonel to delete the user application
byte Timonel::DeleteApplication(void) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("\n\r[%s] Delete Flash Memory >>> 0x%02X\r\n", __func__, DELFLASH);
#endif /* DEBUG_LEVEL */
    byte twi_errors = TwiCmdXmit(DELFLASH, ACKDELFL);
    delay(DLY_DEL_INIT); /* Long delay (~750 ms) to allow complete erasing before trying to initialize */
    twi_errors += BootloaderInit();
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    if (twi_errors > 0) {
        USE_SERIAL.printf_P("\n\n\r###################################################\n\r");
        USE_SERIAL.printf_P("# [%s] # WARNING !!!\n\r# Timonel couldn't be initialized after delete!\n\r", __func__);
        USE_SERIAL.printf_P("# Maybe it's taking too long to delete the memory,\n\r");
        USE_SERIAL.printf_P("# try increasing delay(DLY_DEL_INIT) ...\n\r");
        USE_SERIAL.printf_P("###################################################\n\n\r");
    }
#endif /* DEBUG_LEVEL */
    return twi_errors;
}

/* _________________________
  |                         | 
  |    UploadApplication    |
  |_________________________|
*/
// Upload a user application to a microcontroller running Timonel
byte Timonel::UploadApplication(byte payload[], int payload_size, const int start_address) {
    // .....................................
    // ...... Function initialization ......
    // .....................................
    byte packet_ix = 0;                         /* TWI (I2C) data packet internal byte index */
    byte padding = 0;                           /* Padding byte quantity to add to match the page size */
    byte page_end = 0;                          /* Byte counter to detect the end of flash mem page */
    byte page_count = 1;                        /* Current page counter */
    byte twi_errors = 0;                        /* Upload error counter */
    byte data_packet[MST_PACKET_SIZE] = {0xFF}; /* Payload data packet to be sent to Timonel */
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("\n\r");
#endif /* DEBUG_LEVEL */
#if (!((defined FEATURES_CODE) && ((FEATURES_CODE >> F_AUTO_PAGE_ADDR) & true)))
    //#pragma GCC warning "Address handling code included in Timonel::UploadApplication!"
    if (!((status_.features_code >> F_AUTO_PAGE_ADDR) & true)) {
        // .............................................................................
        // If AUTO_PAGE_ADDR is disabled, the TWI master calculates the pages addresses
        // .............................................................................
        if (!((status_.features_code >> F_CMD_SETPGADDR) & true)) {
            // If AUTO_PAGE_ADDR and CMD_SETPGADDR are both disabled, its impossible to upload data to the bootloader
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
            USE_SERIAL.printf_P("[%s] AUTO_PAGE_ADDR and CMD_SETPGADDR are disabled, can't set page addresses ...\n\n\r", __func__);
#endif /* DEBUG_LEVEL */
            return ERR_SETADDRESS;
        }
        if (start_address >= SPM_PAGESIZE) {
            // If the application is to be flashed at an address other than 0 ...
            // NOTES:
            // 1) Any address different than a 64-bit page start address will be converted
            //    by Timonel to the start address of the page it belongs to by using this mask:
            //    [  page_addr &= ~(SPM_PAGESIZE - 1);  ].
            // 2) Uploading applications on pages that start at addresses other than 0 is only
            //    possible when the TWI master calculates the addresses (AUTO_PAGE_ADDR disabled).
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
            USE_SERIAL.printf_P("[%s] Application doesn't start at 0, fixing reset vector to jump to Timonel ...\n\r", __func__);
#endif /* DEBUG_LEVEL */
            twi_errors += FillSpecialPage(RST_PAGE);
            twi_errors += SetPageAddress(start_address); /* Calculate and fill reset page */
            delay(DLY_FLASH_PG);
        }
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Writing payload to flash, starting at 0x%04X (addresses set by TWI master) ...\n\r", __func__, start_address);
#endif /* DEBUG_LEVEL */
        if (payload_size <= status_.bootloader_start - TRAMPOLINE_LEN) {
            // If the user application fits in memory (can use also the trampoline page)
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
            USE_SERIAL.printf_P("[%s] Payload (%d bytes) fits in AVR memory (trampoline page available), uploading ...\n\r", __func__, payload_size);
#endif                                                                       /* DEBUG_LEVEL */
            twi_errors += FillSpecialPage(TPL_PAGE, payload[1], payload[0]); /* Calculate and fill trampoline page */
            twi_errors += SetPageAddress(start_address);
        } else {
            // If the application overlaps the trampoline bytes, exit with error!
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
            USE_SERIAL.printf_P("[%s] Warning! Payload (%d bytes) doesn't fit in AVR flash memory ...\n\r", __func__, payload_size);
            USE_SERIAL.printf_P("[%s] Trampoline page is available for the application.\n\r", __func__);
            USE_SERIAL.printf_P("[%s] Trampoline: %d (Timonel start: %d)\n\r", __func__, status_.bootloader_start - TRAMPOLINE_LEN, status_.bootloader_start);
            USE_SERIAL.printf_P("[%s]   App size: %d\n\r", __func__, payload_size);
            USE_SERIAL.printf_P("[%s] --------------------------------------\n\r", __func__);
            USE_SERIAL.printf_P("[%s]   Overflow: %d bytes\n\n\r", __func__, payload_size - (status_.bootloader_start - TRAMPOLINE_LEN));
#endif /* DEBUG_LEVEL */
            return ERR_APP_OVF_AU;
        }
        delay(DLY_FLASH_PG);
        // .............................................................................
    } else {
#else
    if (!((status_.features_code >> F_AUTO_PAGE_ADDR) & true)) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] WARNING! AUTO_PAGE_ADDR is disabled in Timonel, please setup TWI master to support it!\n\n\r", __func__);
#endif  /* DEBUG_LEVEL */
        return ERR_AUTO_CALC;
    }
#endif  /* FEATURES_CODE >> !(F_AUTO_PAGE_ADDR) */
        // .............................................................................
        // If AUTO_PAGE_ADDR is enabled, the bootloader calculates the pages addresses
        // .............................................................................
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Writing payload to flash, starting at 0x%04X (auto-calculated addresses) ...\n\r", __func__, start_address);
#endif /* DEBUG_LEVEL */
#if ((defined FEATURES_CODE) && ((FEATURES_CODE >> F_APP_USE_TPL_PG) & true))
        if ((status_.features_code >> F_APP_USE_TPL_PG) & true) {
            // If APP_USE_TPL_PG is enabled, allow application sizes up to TIMONEL_START - TRAMPOLINE_LEN
            if (payload_size <= status_.bootloader_start - TRAMPOLINE_LEN) {
                // If the user application fits in memory (can use also the trampoline page)
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                USE_SERIAL.printf_P("[%s] Payload (%d bytes) fits in AVR memory (trampoline page available), uploading ...\n\r", __func__, payload_size);
#endif /* DEBUG_LEVEL */
            } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                USE_SERIAL.printf_P("[%s] Warning! Payload (%d bytes) doesn't fit in AVR flash memory with current Timonel setup ...\n\r", __func__, payload_size);
                USE_SERIAL.printf_P("[%s] Trampoline page is available for the application.\n\r", __func__);
                USE_SERIAL.printf_P("[%s] Trampoline: %d (Timonel start: %d)\n\r", __func__, status_.bootloader_start - TRAMPOLINE_LEN, status_.bootloader_start);
                USE_SERIAL.printf_P("[%s]   App size: %d\n\r", __func__, payload_size);
                USE_SERIAL.printf_P("[%s] --------------------------------------\n\r", __func__);
                USE_SERIAL.printf_P("[%s]   Overflow: %d bytes\n\n\r", __func__, payload_size - (status_.bootloader_start - TRAMPOLINE_LEN));
#endif /* DEBUG_LEVEL */
                return ERR_APP_OVF_MC;
            }
        } else {
#endif /* FEATURES_CODE >> F_APP_USE_TPL_PG */
            // If APP_USE_TPL_PG is NOT enabled, allow application sizes up to TIMONEL_START - SPM_PAGESIZE
            if (payload_size <= status_.bootloader_start - SPM_PAGESIZE) {
                // If the user application fits in memory (using also the trampoline page)
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                USE_SERIAL.printf_P("[%s] Payload (%d bytes) fits in AVR memory (trampoline page NOT available), uploading ...\n\r", __func__, payload_size);
#endif /* DEBUG_LEVEL */
            } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                USE_SERIAL.printf_P("[%s] Warning! Payload (%d bytes) doesn't fit in AVR flash memory with current Timonel setup ...\n\r", __func__, payload_size);
                USE_SERIAL.printf_P("[%s] Trampoline page is NOT available for the application.\n\r", __func__);
                USE_SERIAL.printf_P("[%s] Trampoline: %d (Timonel start: %d)\n\r", __func__, status_.bootloader_start - TRAMPOLINE_LEN, status_.bootloader_start);
                USE_SERIAL.printf_P("[%s]   App size: %d\n\r", __func__, payload_size);
                USE_SERIAL.printf_P("[%s] --------------------------------------\n\r", __func__);
                USE_SERIAL.printf_P("[%s]   Overflow: %d bytes\n\n\r", __func__, (payload_size - (status_.bootloader_start - SPM_PAGESIZE)));
#endif /* DEBUG_LEVEL */
                return ERR_APP_OVF_MC;
            }
#if ((defined FEATURES_CODE) && ((FEATURES_CODE >> F_APP_USE_TPL_PG) & true))
        }
#endif  /* FEATURES_CODE >> F_APP_USE_TPL_PG */
        // .............................................................................
#if (!((defined FEATURES_CODE) && ((FEATURES_CODE >> F_AUTO_PAGE_ADDR) & true)))
    }
#endif /* FEATURES_CODE >> !(F_AUTO_PAGE_ADDR) */

    if ((payload_size % SPM_PAGESIZE) != 0) {
        // If the payload doesn't use an exact number of pages, resize it to fit padding data
        padding = ((((int)(payload_size / SPM_PAGESIZE) + 1) * SPM_PAGESIZE) - payload_size);
        payload_size += padding;
    }
    // .....................................
    // ...... Application upload loop ......
    // .....................................
    for (int i = 0; i < payload_size; i++) {
        if (i < (payload_size - padding)) {
            // If there are payload unsent data, place them in a data packet
            data_packet[packet_ix] = payload[i];
        } else {
            // If there is no more payload data and the last a data packet
            // is incomplete, add padding data at the end of it (0xFF)
            data_packet[packet_ix] = 0xFF;
        }
        if (packet_ix++ == (MST_PACKET_SIZE - 1)) {
            // If a data packet is complete, dispatch it one byte at a time
            for (int j = 0; j < MST_PACKET_SIZE; j++) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                USE_SERIAL.printf_P(".");
#endif /* DEBUG_LEVEL */
            }
            twi_errors += SendDataPacket(data_packet); /* Send a data packet to Timonel through TWI */
            packet_ix = 0;
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))  // REMEMBER REMOVING THIS BLOCK OR RISING ITS DEBUG LEVEL !!!
            USE_SERIAL.printf_P("\n\r[%s] Last data packet transmission result: -> %d\n\r", __func__, twi_errors);
#endif /* DEBUG_LEVEL */
            // ......................................................................
            // With data packets sizes of 8 to 32 bytes 10 ms is OK (See SLV_PACKET_SIZE)
            delay(DLY_PKT_SEND); /* ###### DELAY BETWEEN PACKETS SENT TO COMPLETE A PAGE ###### */
        }
        if (twi_errors > 0) {
            // Safety payload deletion due to TWI transmission errors
            twi_errors += DeleteApplication();
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
            USE_SERIAL.printf_P("\n\r[%s] Upload error: safety payload deletion triggered, please RESET TWI master!\n\n\r", __func__);
#endif /* DEBUG_LEVEL */
            return twi_errors;
        }
        if (page_end++ == (SPM_PAGESIZE - 1)) {
            // When a page end is detected ...
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
            USE_SERIAL.printf_P(" P%d ", page_count);
#endif /* DEBUG_LEVEL */
#if (!((defined FEATURES_CODE) && ((FEATURES_CODE >> F_AUTO_PAGE_ADDR) & true)))
            if (!((status_.features_code >> F_AUTO_PAGE_ADDR) & true)) {
                // If AUTO_PAGE_ADDR is not enabled, add a 100 ms delay to allow memory flashing, then set the next page address */
                delay(DLY_FLASH_PG);
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
                USE_SERIAL.printf_P("\n\r");
#endif /* DEBUG_LEVEL */
                twi_errors += SetPageAddress(start_address + (page_count * SPM_PAGESIZE));
            }
#endif /* FEATURES_CODE >> F_CMD_SETPGADDR */
            // ......................................................................
            // With Timonel running at 8 and 16 MHz, 100 ms is OK for flashing the page to memory
            delay(DLY_FLASH_PG); /* ###### DELAY AFTER SENDING A FULL PAGE TO ALLOW SAFE WRITING ###### */
            page_count++;
            if (i < (payload_size - 1)) {
                // If the payload end is reached, reset the page end counter
                page_end = 0;
            }
        }
    }
    // .....................................
    // .......... Upload loop end ..........
    // .....................................
    if (twi_errors == 0) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("\n\r[%s] Application was successfully uploaded to AVR flash memory ...\n\n\r", __func__);
#endif /* DEBUG_LEVEL */
    } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("\n\r[%s] %d errors detected during upload, safety payload deletion triggered, please RESET TWI master!\n\n\r", __func__, twi_errors);
#endif /* DEBUG_LEVEL */
        twi_errors += DeleteApplication();
    }
    return twi_errors;
}

/* _________________________
  |                         | 
  |       DumpMemory        |
  |_________________________|
*/
// Display the microcontroller's entire flash memory contents over a serial connection
#if ((defined FEATURES_CODE) && ((FEATURES_CODE >> F_CMD_READFLASH) & true))
//#pragma GCC warning "Timonel::DumpMemory function code included in TWI master!"
byte Timonel::DumpMemory(const word flash_size, const byte rx_packet_size, const byte values_per_line) {
    if (!((status_.features_code >> F_CMD_READFLASH) & true)) {
        USE_SERIAL.printf_P("\n\r[%s] Function not supported by current Timonel (TWI %d) features ...\r\n", __func__, addr_);
        return ERR_NOT_SUPP;
    }
    const byte cmd_size = D_CMD_LENGTH;
    byte twi_cmd_arr[cmd_size] = {READFLSH, 0, 0, 0};
    byte twi_reply_arr[rx_packet_size + D_REPLY_OVRHD];
    byte checksum_errors = 0;
    byte line_ix = 1;
    twi_cmd_arr[3] = rx_packet_size; /* Requested packet size */
    USE_SERIAL.printf_P("\n\r[%s] Dumping Timonel (TWI %d) flash memory ...\n\n\r", __func__, addr_);
    USE_SERIAL.printf_P("Addr %04X: ", 0);
    for (word address = 0; address < flash_size; address += rx_packet_size) {
        twi_cmd_arr[1] = ((address & 0xFF00) >> 8); /* Flash page address high byte */
        twi_cmd_arr[2] = (address & 0xFF);          /* Flash page address low byte */
        byte twi_errors = TwiCmdXmit(twi_cmd_arr, cmd_size, ACKRDFSH, twi_reply_arr, rx_packet_size + D_REPLY_OVRHD);
        if (twi_errors == 0) {
            byte expected_checksum = 0;
            for (byte i = 1; i < (rx_packet_size + 1); i++) {
                USE_SERIAL.printf_P("%02X", twi_reply_arr[i]); /* Memory values */
                if (line_ix == values_per_line) {
                    USE_SERIAL.printf_P("\n\r");
                    if ((address + rx_packet_size) < flash_size) {
                        USE_SERIAL.printf_P("Addr %04X: ", address + rx_packet_size); /* Page address */
                    }
                    line_ix = 0;
                } else {
                    USE_SERIAL.printf_P(" "); /* Space between values */
                }
                line_ix++;
                expected_checksum += (byte)twi_reply_arr[i];
            }
            expected_checksum += (byte)twi_cmd_arr[1];
            expected_checksum += (byte)twi_cmd_arr[2];
            byte received_checksum = twi_reply_arr[rx_packet_size + 1];
            if (expected_checksum != received_checksum) {
                USE_SERIAL.printf_P("\n\r   ### Checksum ERROR! ###   Expected:%d - Received:%d\n\r", expected_checksum, received_checksum);
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
                USE_SERIAL.printf_P("%d\n\r", expected_checksum + 1);
                USE_SERIAL.printf_P(" <-- calculated, received --> %d\n\r", twi_reply_arr[rx_packet_size + 1]);
#endif /* DEBUG_LEVEL */
                if (checksum_errors++ == MAXCKSUMERRORS) {
                    USE_SERIAL.printf_P("[%s] Too many Checksum ERRORS [ %d ], stopping! \n\r", __func__, checksum_errors);
                    delay(DLY_1_SECOND);
                    return ERR_CHECKSUM_D;
                }
            }
        } else {
            USE_SERIAL.printf_P("[%s] Error parsing 0x%02X command <<< 0x%02X\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
            return ERR_CMD_PARSE_D;
        }
        delay(DLY_PKT_REQUEST);
    }
    USE_SERIAL.printf_P("\n\r[%s] Timonel (TWI %d) flash memory dump successful!", __func__, addr_);
    if (checksum_errors > 0) {
        USE_SERIAL.printf_P(" Checksum errors: %d", checksum_errors);
    }
    USE_SERIAL.printf_P("\n\n\r");
    return OK;
}
#else
#pragma GCC warning "Timonel::DumpMemory function code NOT INCLUDED in TWI master!"
#endif /* FEATURES_CODE >> F_CMD_READFLASH */

/////////////////////////////////////////////////////////////////////////////
////////////                 Internal functions                  ////////////
/////////////////////////////////////////////////////////////////////////////

// Function BootloaderInit (Initializes Timonel in 1 or 2 steps, as required by its features)
byte Timonel::BootloaderInit(void) {
    byte twi_errors = 0;
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("[%s] Timonel device %02d * Initialization Step 1 required by features *\r\n", __func__, addr_);
#endif /* DEBUG_LEVEL */
    // Timonel initialization: STEP 1
    twi_errors += QueryStatus();
#if ((defined FEATURES_CODE) && ((FEATURES_CODE >> F_TWO_STEP_INIT) & true))
    //#pragma GCC warning "Two-step initialization code included in Timonel::BootloaderInit!"
    // If TWO_STEP_INIT feature is enabled in Timonel device
    if ((status_.features_code >> F_TWO_STEP_INIT) & true) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Timonel device %02d * Initialization Step 2 required by features *\r\n", __func__, addr_);
#endif /* DEBUG_LEVEL */
        // Timonel initialization: STEP 2 (Only if it has this feature enabled)
        twi_errors += NbMicro::InitMicro();
    }
#else
#pragma GCC warning "Two-step initialization code NOT INCLUDED in Timonel::BootloaderInit!"
#endif /* FEATURES_CODE >> F_TWO_STEP_INIT */
    return twi_errors;
}

// Function QueryStatus (Retrieves the bootloader running parameters from the microcontroller)
byte Timonel::QueryStatus(void) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("[%s] Querying Timonel device %02d to get status ...\r\n", __func__, addr_);
#endif                                        /* DEBUG_LEVEL */
    byte twi_reply_arr[S_REPLY_LENGTH] = {0}; /* Status received from I2C slave */
    byte twi_errors = TwiCmdXmit(GETTMNLV, ACKTMNLV, twi_reply_arr, S_REPLY_LENGTH);
    if (twi_errors != 0) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Error querying Timonel device %02d to get status (%d) ...\r\n", __func__, addr_, twi_errors);
#endif /* DEBUG_LEVEL */
        return twi_errors;
    } else {
        if ((twi_reply_arr[CMD_ACK_POS] == ACKTMNLV) && (twi_reply_arr[S_SIGNATURE] == T_SIGNATURE)) {
            status_.signature = twi_reply_arr[S_SIGNATURE];
            status_.version_major = twi_reply_arr[S_MAJOR];
            status_.version_minor = twi_reply_arr[S_MINOR];
            status_.features_code = twi_reply_arr[S_FEATURES];
            status_.ext_features_code = twi_reply_arr[S_EXT_FEATURES];
            status_.bootloader_start = (twi_reply_arr[S_BOOT_ADDR_MSB] << 8) + twi_reply_arr[S_BOOT_ADDR_LSB];
            status_.application_start = (twi_reply_arr[S_APPL_ADDR_LSB] << 8) + twi_reply_arr[S_APPL_ADDR_MSB];
            status_.trampoline_addr = (~(((twi_reply_arr[S_APPL_ADDR_MSB] << 8) | twi_reply_arr[S_APPL_ADDR_LSB]) & 0xFFF));
            status_.trampoline_addr++;
            status_.trampoline_addr = ((((status_.bootloader_start >> 1) - status_.trampoline_addr) & 0xFFF) << 1);
            status_.low_fuse_setting = twi_reply_arr[S_LOW_FUSE];
            status_.oscillator_cal = twi_reply_arr[S_OSCCAL];
        }
        return OK;
    }
}

// Function SendDataPacket (Sends a data packet, a memory page fraction, to Timonel)
byte Timonel::SendDataPacket(const byte data_packet[]) {
    const byte cmd_size = MST_PACKET_SIZE + 2;
    const byte reply_size = 2;
    byte twi_cmd[cmd_size] = {0};
    byte twi_reply_arr[reply_size] = {0};
    byte checksum = 0;
    twi_cmd[0] = WRITPAGE;
    for (int i = 1; i < cmd_size - 1; i++) {
        twi_cmd[i] = data_packet[i - 1];
        checksum += (byte)data_packet[i - 1]; /* Data checksum accumulator (mod 256) */
    }
    twi_cmd[cmd_size - 1] = checksum;
    byte twi_errors = TwiCmdXmit(twi_cmd, cmd_size, ACKWTPAG, twi_reply_arr, reply_size);
    if (twi_reply_arr[0] == ACKWTPAG) {
        if (twi_reply_arr[1] != checksum) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
            USE_SERIAL.printf_P("[%s] Checksum ERROR! Expected value: 0x%02X <<< Received = 0x%02X\r\n", __func__, checksum, twi_reply_arr[1]);
#endif /* DEBUG_LEVEL */
            return (twi_errors + ERR_TX_PKT_CHKSUM);
        }
    } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Error parsing 0x%02X command! <<< 0x%02X\r\n", __func__, twi_cmd[0], twi_reply_arr[0]);
#endif                          /* DEBUG_LEVEL */
        if (twi_errors++ > 0) { /* Opcode error detected ... */
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
            USE_SERIAL.printf_P("\n\r[%s] Opcode Reply Errors, Exiting!\n\r", __func__);
#endif /* DEBUG_LEVEL */
            return twi_errors;
        }
    }
    return twi_errors;
}

// Function SetPageAddres (Sets the start address of a flash memory page)
#if (!((defined FEATURES_CODE) && ((FEATURES_CODE >> F_AUTO_PAGE_ADDR) & true)))
//#pragma GCC warning "Timonel::SetPageAddress, FillSpecialPage and CalculateTrampoline functions code included in TWI master!"
byte Timonel::SetPageAddress(const word page_addr) {
    const byte cmd_size = 4;
    const byte reply_size = 2;
    byte twi_cmd_arr[cmd_size] = {STPGADDR, 0, 0, 0};
    byte twi_reply_arr[reply_size];
    twi_cmd_arr[1] = ((page_addr & 0xFF00) >> 8);             /* Flash page address MSB */
    twi_cmd_arr[2] = (page_addr & 0xFF);                      /* Flash page address LSB */
    twi_cmd_arr[3] = (byte)(twi_cmd_arr[1] + twi_cmd_arr[2]); /* Checksum */
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL == 1))
    USE_SERIAL.printf_P(" (a:%02X%02X) ", twi_cmd_arr[1], twi_cmd_arr[2]);
#elif ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
    USE_SERIAL.printf_P("\n\n\r[%s] >> Setting flash page address on Timonel >>> %d (STPGADDR)\n\r", __func__, twi_cmd_arr[0]);
#endif /* DEBUG_LEVEL */
    byte twi_errors = TwiCmdXmit(twi_cmd_arr, cmd_size, AKPGADDR, twi_reply_arr, reply_size);
    if (twi_errors == 0) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
        USE_SERIAL.printf_P("[%s] >> Command %d parsed OK <<< %d\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
#endif /* DEBUG_LEVEL */
        if (twi_reply_arr[1] == twi_cmd_arr[3]) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
            USE_SERIAL.printf_P("[%s] >> Operands %d and %d parsed OK by Timonel <<< Flash Page Address Check = %d\n\r", __func__, twi_cmd_arr[1], twi_cmd_arr[2], twi_reply_arr[1]);
#endif /* DEBUG_LEVEL */
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
            USE_SERIAL.printf_P("[%s] Address %04X (%02X) (%02X) parsed OK by Timonel <<< Check = %d\n\r", __func__, page_addr, twi_cmd_arr[1], twi_cmd_arr[2], twi_reply_arr[1]);
#endif /* DEBUG_LEVEL */
        } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
            USE_SERIAL.printf_P("[%s] Operand %d parsed with ERROR <<< Timonel Check = %d\r\n", __func__, twi_cmd_arr[1], twi_reply_arr[1]);
#endif /* DEBUG_LEVEL */
        }
    } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
        USE_SERIAL.printf_P("[%s] Error parsing 0x%02X command! <<< %02X\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
#endif /* DEBUG_LEVEL */
    }
    return twi_errors;
}

// Function FillSpecialPage (Fills a reset or trampoline page, as required by Timonel features)
byte Timonel::FillSpecialPage(const byte page_type, const byte app_reset_msb, const byte app_reset_lsb) {
    word address = 0x0000;
    byte packet_ix = 0; /* TWI (I2C) data packet internal byte index */
    byte twi_errors = 0;
    byte data_packet[MST_PACKET_SIZE] = {0xFF};
    byte special_page[64] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    // Special page selector
    switch (page_type) {
        case RST_PAGE: { /* Special page type 1: Reset Vector Page (addr: 0) */
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
            USE_SERIAL.printf_P("[%s] Type %d: Modifying the first 2 bytes (restart vector) to point to this bootloader ...\n\r", __func__, page_type);
#endif /* DEBUG_LEVEL */
            special_page[0] = (0xC0 + ((((status_.bootloader_start / 2) - 1) >> 8) & 0xFF));
            special_page[1] = (((status_.bootloader_start / 2) - 1) & 0xFF);
            break;
        }
        case TPL_PAGE: { /* Special page type 2: Trampoline Page (addr: TIMONEL_START - 64) */
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
            USE_SERIAL.printf_P("[%s] Type %d: Calculate the trampoline and fill the page with 0xFF + 2-byte jump address ...\n\r", __func__, page_type);
#endif /* DEBUG_LEVEL */
            address = status_.bootloader_start - (SPM_PAGESIZE);
            word tpl = CalculateTrampoline(status_.bootloader_start, ((app_reset_msb << 8) | app_reset_lsb));
            special_page[SPM_PAGESIZE - 1] = (byte)((tpl >> 8) & 0xFF);
            special_page[SPM_PAGESIZE - 2] = (byte)(tpl & 0xFF);
            break;
        }
        default: {
            // Nothing ...
            break;
        }
    }
    twi_errors += SetPageAddress(address);
    delay(DLY_SET_ADDR);
    for (byte i = 0; i < SPM_PAGESIZE; i++) {
        data_packet[packet_ix] = special_page[i];
        if (packet_ix++ == (MST_PACKET_SIZE - 1)) {
            for (int j = 0; j < MST_PACKET_SIZE; j++) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                USE_SERIAL.printf_P("|");
#endif /* DEBUG_LEVEL */
            }
            twi_errors += SendDataPacket(data_packet); /* Send data to Timonel through I2C */
            packet_ix = 0;
            delay(DLY_PKT_SEND);
        }
    }
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("\r\n");
#endif /* DEBUG_LEVEL */
    delay(DLY_RETURN);
    return twi_errors;
}

// Function CalculateTrampoline (Calculates the application trampoline address)
word Timonel::CalculateTrampoline(word bootloader_start, word application_start) {
    return (((~((bootloader_start >> 1) - ((application_start + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
}
#else
#pragma GCC warning "Timonel::SetPageAddress, FillSpecialPage and CalculateTrampoline functions code NOT INCLUDED in TWI master!"
#endif /* FEATURES_CODE >> F_AUTO_PAGE_ADDR */
