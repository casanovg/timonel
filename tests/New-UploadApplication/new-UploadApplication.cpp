/*
 *  Timonel TWI Master Library
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: TimonelTwiM.cpp (Library)
 *  ........................................... 
 *  Version: 1.3 / 2019-06-06
 *  gustavo.casanova@nicebots.com
 *  ...........................................
 *  This TWI (I2C) master library interacts with a microcontroller
 *  running the Timonel bootloader. It inherits from the NbMicro
 *  class, which handles the actual communication over the TWI bus
 *  using the NB command set.
 */

#include "TimonelTwiM.h"

/* _________________________
  |                         | 
  |    UploadApplication    |
  |_________________________|
*/
// Upload an user application to a microcontroller running Timonel
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
#pragma GCC warning "Address manipulation code included in Timonel::UploadApplication!"    
    if (!((status_.features_code >> F_AUTO_PAGE_ADDR) & true)) {
        // .............................................................................
        // If AUTO_PAGE_ADDR is disabled, the TWI master calculates the pages addresses
        // .............................................................................
        if (!((status_.features_code >> F_CMD_SETPGADDR) & true)) {
            // If AUTO_PAGE_ADDR and CMD_SETPGADDR are both disabled, its impossible to upload data to the bootloader
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
            USE_SERIAL.printf_P("[%s] AUTO_PAGE_ADDR and CMD_SETPGADDR are disabled, can't set page addresses ...\n\r", __func__);
#endif /* DEBUG_LEVEL */
            return ERR_SETADDRESS;
        }
        if (start_address >= SPM_PAGESIZE) {
            // If the application starts at an address other than 0 ...
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
#endif /* DEBUG_LEVEL */
            twi_errors += FillSpecialPage(TPL_PAGE, payload[1], payload[0]); /* Calculate and fill trampoline page */
            twi_errors += SetPageAddress(start_address);
        } else {
            // If the application overlaps the trampoline bytes, exit with error!
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
            USE_SERIAL.printf_P("[%s] Warning! Payload (%d bytes) doesn't fit in AVR flash memory ...\n\r", __func__, payload_size);
            USE_SERIAL.printf_P("[%s] Trampoline page is available for the application!\n\r", __func__);
            USE_SERIAL.printf_P("[%s] Trampoline: %d (Timonel start: %d)\n\r", __func__, status_.bootloader_start - TRAMPOLINE_LEN, status_.bootloader_start);
            USE_SERIAL.printf_P("[%s]   App size: %d\n\r", __func__, payload_size);
            USE_SERIAL.printf_P("[%s] --------------------------------------\n\r", __func__);
            USE_SERIAL.printf_P("[%s]   Overflow: %d bytes\n\r", __func__, (payload_size - (status_.bootloader_start + TRAMPOLINE_LEN)));
#endif /* DEBUG_LEVEL */
            return ERR_APP_OVF_AU;
        }
        delay(DLY_FLASH_PG);
        // .............................................................................
    } else {
#endif /* FEATURES_CODE >> !(F_AUTO_PAGE_ADDR) */
        // .............................................................................
        // If AUTO_PAGE_ADDR is enabled, the bootloader calculates the pages addresses
        // .............................................................................
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Writing payload to flash, starting at 0x%04X (auto-calculated addresses) ...\n\r", __func__, start_address);
#endif /* DEBUG_LEVEL */
#if ((defined FEATURES_CODE) && ((FEATURES_CODE >> F_APP_USE_TPL_PG) & true))
        if ((status_.features_code >> F_APP_USE_TPL_PG) & true) {
            // If APP_USE_TPL_PG is enabled, we have to allow application sizes up to TIMONEL_START - 2 (TRAMPOLINE_LEN)
            if (payload_size <= status_.bootloader_start - TRAMPOLINE_LEN) {
                // If the user application fits in memory (can use also the trampoline page)
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                USE_SERIAL.printf_P("[%s] Payload (%d bytes) fits in AVR memory with current setup (can use trampoline page), uploading ...\n\r", __func__, payload_size);
#endif /* DEBUG_LEVEL */
            } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                USE_SERIAL.printf_P("[%s] Warning! Payload (%d bytes) doesn't fit in AVR flash memory with current Timonel setup ...\n\r", __func__, payload_size);
                USE_SERIAL.printf_P("[%s] Trampoline page is available for the application!\n\r", __func__);                
                USE_SERIAL.printf_P("[%s] Trampoline: %d (Timonel start: %d)\n\r", __func__, status_.bootloader_start - TRAMPOLINE_LEN, status_.bootloader_start);
                USE_SERIAL.printf_P("[%s]   App size: %d\n\r", __func__, payload_size);
                USE_SERIAL.printf_P("[%s] --------------------------------------\n\r", __func__);
                USE_SERIAL.printf_P("[%s]   Overflow: %d bytes\n\r", __func__, (payload_size - (status_.bootloader_start - TRAMPOLINE_LEN)));
#endif /* DEBUG_LEVEL */
                return ERR_APP_OVF_MC;
            }
        } else {
#endif /* FEATURES_CODE >> F_APP_USE_TPL_PG */
            // If APP_USE_TPL_PG is NOT enabled, we have to allow application sizes up to TIMONEL_START - SPM_PAGESIZE)
            if (payload_size <= status_.bootloader_start - SPM_PAGESIZE) {
                // If the user application fits in memory (using also the trampoline page)
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                USE_SERIAL.printf_P("[%s] Payload (%d bytes) fits in AVR memory with current setup (can't use trampoline page), uploading ...\n\r", __func__, payload_size);
#endif /* DEBUG_LEVEL */
            } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                USE_SERIAL.printf_P("[%s] Warning! Payload (%d bytes) doesn't fit in AVR flash memory with current Timonel setup ...\n\r", __func__, payload_size);
                USE_SERIAL.printf_P("[%s] Trampoline page is NOT available for the application!\n\r", __func__);                
                USE_SERIAL.printf_P("[%s] Trampoline: %d (Timonel start: %d)\n\r", __func__, status_.bootloader_start - TRAMPOLINE_LEN, status_.bootloader_start);
                USE_SERIAL.printf_P("[%s]   App size: %d\n\r", __func__, payload_size);
                USE_SERIAL.printf_P("[%s] --------------------------------------\n\r", __func__);
                USE_SERIAL.printf_P("[%s]   Overflow: %d bytes\n\r", __func__, (payload_size - (status_.bootloader_start - SPM_PAGESIZE)));
#endif /* DEBUG_LEVEL */
                return ERR_APP_OVF_MC;
            }            
#if ((defined FEATURES_CODE) && ((FEATURES_CODE >> F_APP_USE_TPL_PG) & true))
        }
#endif /* FEATURES_CODE >> F_APP_USE_TPL_PG */        
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

                USE_SERIAL.printf_P(".");

            }

            twi_errors += SendDataPacket(data_packet); /* Send a data packet to Timonel through TWI */
            packet_ix = 0;

            // REMEMBER REMOVING THIS BLOCK OR RISING ITS DEBUG LEVEL !!!
            //USE_SERIAL.printf_P("\n\r[%s] Last data packet transmission result: -> %d\n\r", __func__, twi_errors);

            // ......................................................................
            // With data packets sizes of 8 to 32 bytes 10 ms is OK (See SLV_PACKET_SIZE)
            delay(DLY_PKT_SEND); /* ###### DELAY BETWEEN PACKETS SENT TO COMPLETE A PAGE ###### */
            
        }
        
        if (twi_errors > 0) {
            // Safety payload deletion due to TWI transmission errors
            twi_errors += DeleteApplication();

            USE_SERIAL.printf_P("\n\r[%s] Upload error: safety payload deletion triggered, please RESET TWI master!\n\r", __func__);

            return twi_errors;
        }
        
        if (page_end++ == (SPM_PAGESIZE - 1)) {
            // When a page end is detected ...

            USE_SERIAL.printf_P(" P%d ", page_count);

            if (!((status_.features_code >> F_AUTO_PAGE_ADDR) & true)) {
                // If AUTO_PAGE_ADDR is not enabled, add a 100 ms delay to allow memory flashing, then set the next page address */
                delay(DLY_FLASH_PG);
                twi_errors += SetPageAddress(start_address + (page_count * SPM_PAGESIZE));
            }

            // ......................................................................
            // With Timonel running at 8 and 16 MHz, 100 ms is OK for flashing the page to memory
            delay(DLY_FLASH_PG); /* ###### DELAY AFTER SENDING A FULL PAGE TO ALLOW SAFE WRITING ###### */
            page_count++;
            
            if (i < (payload_size - 1)) {
                // If we reached the payload end, reset the page end counter
                page_end = 0;
            }
            
        }
    }
    
    // .....................................
    // .......... Upload loop end ..........
    // .....................................     
    
    if (twi_errors == 0) {

        USE_SERIAL.printf_P("\n\r[%s] Application was successfully uploaded, please select 'run app' command to start it ...\n\r", __func__);

    } else {

        USE_SERIAL.printf_P("\n\r[%s] %d errors detected during upload, safety payload deletion triggered, please RESET TWI master!\n\r", __func__, twi_errors);
        twi_errors += DeleteApplication();
    }
    
    return twi_errors;
    
}
