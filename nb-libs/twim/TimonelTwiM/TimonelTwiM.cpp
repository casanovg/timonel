/*
  TimonelTwiM.cpp
  ===============
  Library code for uploading firmware to a microcontroller
  that runs the Timonel I2C bootloader.
  ---------------------------
  2018-12-13 Gustavo Casanova
  ---------------------------
*/

#include "TimonelTwiM.h"

// Class constructor
Timonel::Timonel(const byte twi_address, const byte sda, const byte scl) : NbMicro(twi_address, sda, scl) {
    if ((addr_ > 0) && (addr_ < 36)) {
        USE_SERIAL.printf_P("[%s] Bootloader instance created with address %02d.\r\n", __func__, addr_);
        BootloaderInit(0);
    }
}

// Member Function QueryStatus (Retrieves the bootloader running parameters from the microcontroller)
byte Timonel::QueryStatus(void) {
    USE_SERIAL.printf_P("[%s] Querying Timonel device %02d to get status ...\r\n", __func__, addr_);
    byte twi_reply_arr[V_CMD_LENGTH] = {0}; /* Status received from I2C slave */
    byte twi_cmd_err = TwiCmdXmit(GETTMNLV, ACKTMNLV, twi_reply_arr, V_CMD_LENGTH);
    if ((twi_cmd_err == 0) && (twi_reply_arr[CMD_ACK_POS] == ACKTMNLV) && (twi_reply_arr[V_SIGNATURE] == T_SIGNATURE)) {
        status_.signature = twi_reply_arr[V_SIGNATURE];
        status_.version_major = twi_reply_arr[V_MAJOR];
        status_.version_minor = twi_reply_arr[V_MINOR];
        status_.features_code = twi_reply_arr[V_FEATURES];
        status_.bootloader_start = (twi_reply_arr[V_BOOT_ADDR_MSB] << 8) + twi_reply_arr[V_BOOT_ADDR_LSB];
        status_.application_start = (twi_reply_arr[V_APPL_ADDR_LSB] << 8) + twi_reply_arr[V_APPL_ADDR_MSB];
        status_.trampoline_addr = (~(((twi_reply_arr[V_APPL_ADDR_MSB] << 8) | twi_reply_arr[V_APPL_ADDR_LSB]) & 0xFFF));
        status_.trampoline_addr++;
        status_.trampoline_addr = ((((status_.bootloader_start >> 1) - status_.trampoline_addr) & 0xFFF) << 1);
    } else {
        //USE_SERIAL.printf_P("\n\r[%s] Error: parsing %02d command! <<< %d\n\r", __func__, GETTMNLV, twi_reply_arr[0]);
        return (ERR_01);
    }
    return (OK);
}

// Returns a struct with the Timonel bootloader running status
Timonel::Status Timonel::GetStatus(void) {
    USE_SERIAL.printf_P("[%s] Getting Timonel device %02d status ...\r\n", __func__, addr_);
    QueryStatus();
    return (status_);
}

// Member Function BootloaderInit
byte Timonel::BootloaderInit(const word time) {
    delay(time);
    USE_SERIAL.printf_P("[%s] Timonel device %02d * Initialization Step 1 required by features *\r\n", __func__, addr_);
    byte step1_outcome = QueryStatus(); /* Timonel initialization: STEP 1 */
    byte step2_outcome = 0;
    if ((status_.features_code & 0x10) == 0x10) {
        USE_SERIAL.printf_P("[%s] Timonel device %02d * Initialization Step 2 required by features *\r\n", __func__, addr_);
        step2_outcome = InitMicro(); /* Timonel initialization: STEP 2 (only if Timonel has this feature enabled) */
    }
    if ((step1_outcome + step1_outcome) == ACKINITS) {
        return (OK);
    } else {
        return (1);
    }
}

// Member Function WritePageBuff
byte Timonel::WritePageBuff(const byte data_array[]) {
    const byte cmd_size = MST_DATA_SIZE + 2;
    const byte reply_size = 2;
    byte twi_cmd[cmd_size] = {0};
    byte twi_reply_arr[reply_size] = {0};
    byte checksum = 0;
    twi_cmd[0] = WRITPAGE;
    for (int i = 1; i < cmd_size - 1; i++) {
        twi_cmd[i] = data_array[i - 1];
        checksum += (byte)data_array[i - 1]; /* Data checksum accumulator (mod 256) */
    }
    twi_cmd[cmd_size - 1] = checksum;
    byte wrt_errors = TwiCmdXmit(twi_cmd, cmd_size, ACKWTPAG, twi_reply_arr, reply_size);
    if (twi_reply_arr[0] == ACKWTPAG) {
        if (twi_reply_arr[1] != checksum) {
            USE_SERIAL.printf_P("[%s] Data parsed with {{{ERROR}}} <<< Checksum = 0x%02X\r\n", __func__, twi_reply_arr[1]);
            if (wrt_errors++ > 0) { /* Checksum error detected ... */
                USE_SERIAL.printf_P("\n\r[%s] Checksum Errors, Exiting!\r\n", __func__);
                exit(wrt_errors);
            }
        }
    } else {
        USE_SERIAL.printf_P("[%s] Error parsing 0x%02X command! <<< 0x%02X\r\n", __func__, twi_cmd[0], twi_reply_arr[0]);
        if (wrt_errors++ > 0) { /* Opcode error detected ... */
            USE_SERIAL.printf_P("\n\r[%s] Opcode Reply Errors, Exiting!\n\r", __func__);
            exit(wrt_errors);
        }
    }
    return (wrt_errors);
}

// Uploads a user application to a microcontroller running Timonel
byte Timonel::UploadApplication(byte payload[], int payload_size, const int start_address) {
    byte packet = 0;                               /* Byte amount to be sent in a single I2C data packet */
    byte padding = 0;                              /* Amount of padding bytes to match the page size */
    byte page_end = 0;                             /* Byte counter to detect the end of flash mem page */
    byte page_count = 1;                           /* Current page counter */
    byte upl_errors = 0;                           /* Upload error counter */
    byte data_packet[MST_DATA_SIZE] = {0xFF};      /* Payload data packet to be sent to Timonel */
    if ((status_.features_code & 0x08) != false) { /* If CMD_STPGADDR is enabled */
        if (start_address >= PAGE_SIZE) {          /* If application start address is not 0 */
            USE_SERIAL.printf_P("\n\r[%s] Application doesn't start at 0, fixing reset vector to jump to Timonel ...\n\r", __func__);
            FillSpecialPage(1);
            SetPageAddress(start_address); /* Calculate and fill reset page */
            delay(100);
        }
        if ((status_.features_code & 0x02) == false) {                  /* if AUTO_TPL_CALC is disabled in Timonel device */
            if (payload_size <= status_.bootloader_start - PAGE_SIZE) { /* if the user application does NOT USE the trampoline page */
                USE_SERIAL.printf_P("\n\r[%s] Application doesn't use trampoline page ...\n\r", __func__);
                FillSpecialPage(2, payload[1], payload[0]); /* Calculate and fill trampoline page */
                SetPageAddress(start_address);
            } else {
                if (payload_size <= status_.bootloader_start) { /* if the user application does USE the trampoline page, replace its last two bytes with the trampoline bytes */
                    USE_SERIAL.printf_P("\n\r[%s] Application uses trampoline page ...\n\r", __func__);
                    word tpl = CalculateTrampoline(status_.bootloader_start, payload[1] | payload[0]);
                    payload[payload_size - 2] = (byte)(tpl & 0xFF);
                    payload[payload_size - 1] = (byte)((tpl >> 8) & 0xFF);
                } else { /* if the application overlaps the bootloader */
                    USE_SERIAL.printf_P("\n\r[%s] Application doesn't fit in flash memory ...\n\r", __func__);
                    USE_SERIAL.printf_P("[%s] Bootloafer start: %d\n\r", __func__, status_.bootloader_start);
                    USE_SERIAL.printf_P("[%s] Application size: %d\n\r", __func__, payload_size);
                    USE_SERIAL.printf_P("[%s] ----------------------\n\r", __func__);
                    USE_SERIAL.printf_P("[%s]         Overflow: %d\n\r", __func__, payload_size - status_.bootloader_start);
                    return (2);
                }
            }
            delay(100);
        }
    }
    if ((payload_size % PAGE_SIZE) != 0) { /* If the payload doesn't use an exact number of pages, resize it by adding padding data */
        padding = ((((int)(payload_size / PAGE_SIZE) + 1) * PAGE_SIZE) - payload_size);
        payload_size += padding;
    }
    USE_SERIAL.printf_P("\n\r[%s] Writing payload to flash, starting at 0x%04X ...\n\r", __func__, start_address);
    for (int i = 0; i < payload_size; i++) {
        if (i < (payload_size - padding)) {
            data_packet[packet] = payload[i]; /* If there are data to fill the page, use them ... */
        } else {
            data_packet[packet] = 0xFF; /* If there are no more data, complete the page with padding (0xff) */
        }
        if (packet++ == (MST_DATA_SIZE - 1)) { /* When a data packet is completed to be sent ... */
            for (int j = 0; j < MST_DATA_SIZE; j++) {
                USE_SERIAL.printf_P(".");
            }
            upl_errors += WritePageBuff(data_packet); /* Send a data packet to Timonel through TWI */
            packet = 0;
            // Data packet 8 = delay 10; // Data packet 16 = delay 20; 
            delay(15); /* ###### DELAY BETWEEN PACKETS SENT TO PAGE ###### */
        }
        if (upl_errors > 0) {
            //DeleteFlash();
            BootloaderInit(2000);
#if ESP8266
            ESP.restart();
#else
            resetFunc();
#endif /* ESP8266 */
            return (upl_errors);
        }
        if (page_end++ == (PAGE_SIZE - 1)) { /* When a page end is detected ... */

            USE_SERIAL.printf_P(" P%d ", page_count);

            // Data packet 8 = delay 100; // Data packet 16 = delay 150; 
            if ((status_.features_code & 0x08) != false) { /* If CMD_STPGADDR is enabled in Timonel, add a 100 ms */
                delay(100);                                /* delay to allow memory flashing, then set the next   */
                USE_SERIAL.printf_P("\n\r");               /* page address before sending new data.               */
                SetPageAddress(start_address + (page_count * PAGE_SIZE));
            }

            // Data packet 8 = delay 100; // Data packet 16 = delay 150;
            delay(100); /* ###### DELAY BETWEEN PAGE WRITINGS ... ###### */
            page_count++;

            if (i < (payload_size - 1)) {
                page_end = 0;
            }
        }
    }
    if (upl_errors == 0) {
        USE_SERIAL.printf_P("\n\r[%s] Application was successfully transferred, please select 'run app' command to start it ...\n\r", __func__);
    } else {
        USE_SERIAL.printf_P("\n\r[%s] Communication errors detected during firmware transfer, please retry !!! ErrCnt: %d\n\r", __func__, upl_errors);
        //DeleteFlash();
        BootloaderInit(2000);
#if ESP8266
        ESP.restart();
#else
        resetFunc();
#endif /* ESP8266 */
    }
    return (upl_errors);
}

// Member Function SetPageAddres (Set the start memory address of the next page to load)
byte Timonel::SetPageAddress(const word page_addr) {
    const byte cmd_size = 4;
    const byte reply_size = 2;
    byte twi_cmd_arr[cmd_size] = {STPGADDR, 0, 0, 0};
    byte twi_reply_arr[reply_size];
    twi_cmd_arr[1] = ((page_addr & 0xFF00) >> 8);             /* Flash page address MSB */
    twi_cmd_arr[2] = (page_addr & 0xFF);                      /* Flash page address LSB */
    twi_cmd_arr[3] = (byte)(twi_cmd_arr[1] + twi_cmd_arr[2]); /* Checksum */
    //USE_SERIAL.printf_P("\n\n\r[%s] Setting flash page address on Timonel >>> %d (STPGADDR)\n\r", __func__, twi_cmd_arr[0]);
    byte twi_cmd_err = TwiCmdXmit(twi_cmd_arr, cmd_size, AKPGADDR, twi_reply_arr, reply_size);
    if (twi_cmd_err == 0) {
        //USE_SERIAL.printf_P("[%s] Command %d parsed OK <<< %d\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
        if (twi_reply_arr[1] == twi_cmd_arr[3]) {
            //USE_SERIAL.printf_P("[%s] Operands %d and %d parsed OK by Timonel <<< Flash Page Address Check = %d\n\r", __func__, twi_cmd_arr[1], twi_cmd_arr[2], twi_reply_arr[1]);
            USE_SERIAL.printf_P("[%s] Address %04X (%02X) (%02X) parsed OK by Timonel <<< Check = %d\n\r", __func__, page_addr, twi_cmd_arr[1], twi_cmd_arr[2], twi_reply_arr[1]);
        } else {
            USE_SERIAL.printf_P("[%s] Operand %d parsed with {{{ERROR}}} <<< Timonel Check = %d\r\n", __func__, twi_cmd_arr[1], twi_reply_arr[1]);
        }
    } else {
        USE_SERIAL.printf_P("[%s] Error parsing %d command! <<< %d\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
    }
    return (twi_cmd_err);
}

// Member Function FillSpecialPage
byte Timonel::FillSpecialPage(const byte page_type, const byte app_reset_msb, const byte app_reset_lsb) {
    word address = 0x0000;
    //byte special_page[64] = { 0xFF };
    byte special_page[64] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    byte packet = 0; /* Byte amount to be sent in a single I2C data packet */
    byte data_packet[MST_DATA_SIZE] = {0xFF};
    // Function mode: 1=reset vector page, 2=trampoline page
    switch (page_type) {
        case 1: { /* Reset Vector Page (0) */
            special_page[0] = (0xC0 + ((((status_.bootloader_start / 2) - 1) >> 8) & 0xFF));
            special_page[1] = (((status_.bootloader_start / 2) - 1) & 0xFF);
            break;
        }
        case 2: { /* Trampoline Page (Timonel start - 64)*/
            //address = 0xE00; //status_.bootloader_start - PAGE_SIZE;
            address = status_.bootloader_start - (PAGE_SIZE);
            //word tpl = (((~((status_.bootloader_start >> 1) - ((((app_reset_msb << 8) | app_reset_lsb) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
            word tpl = CalculateTrampoline(status_.bootloader_start, ((app_reset_msb << 8) | app_reset_lsb));
            special_page[PAGE_SIZE - 1] = (byte)((tpl >> 8) & 0xFF);
            special_page[PAGE_SIZE - 2] = (byte)(tpl & 0xFF);
            break;
        }
        default: {
            // ---
            break;
        }
    }
    byte twi_errors = SetPageAddress(address);
    delay(100);
    for (byte i = 0; i < PAGE_SIZE; i++) {
        data_packet[packet] = special_page[i];
        if (packet++ == (MST_DATA_SIZE - 1)) {
            for (int j = 0; j < MST_DATA_SIZE; j++) {
                USE_SERIAL.printf_P("|");
            }
            twi_errors += WritePageBuff(data_packet); /* Send data to Timonel through I2C */
            packet = 0;
            delay(10);
        }
    }
    USE_SERIAL.printf_P(" [Special page type %d]\n\n\r", page_type);
    delay(100);
}

// Asks Timonel to stop executing and run the user application
byte Timonel::RunApplication(void) {
    USE_SERIAL.printf_P("\n\r[%s] Exit bootloader & run application >>> %d\r\n", __func__, EXITTMNL);
    return (TwiCmdXmit(EXITTMNL, ACKEXITT));
}

// Makes Timonel delete the user application
byte Timonel::DeleteApplication(void) {
    USE_SERIAL.printf_P("\n\r[%s] Delete Flash Memory >>> %d\r\n", __func__, DELFLASH);
    return (TwiCmdXmit(DELFLASH, ACKDELFL));
}

// Member Function CalculateTrampoline
word Timonel::CalculateTrampoline(word bootloader_start, word application_start) {
    return (((~((bootloader_start >> 1) - ((application_start + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
}

// Displays the microcontroller's entire flash memory contents
byte Timonel::DumpMemory(const word flash_size, const byte slave_data_size, const byte values_per_line) {
    if ((status_.features_code & 0x80) == false) {
        USE_SERIAL.printf_P("\n\r[%s] Function not supported by current Timonel features ...\r\n", __func__, DELFLASH);
        return (3);
    }
    const byte cmd_size = 5;
    byte twi_cmd_arr[cmd_size] = {READFLSH, 0, 0, 0, 0};
    byte twi_reply_arr[slave_data_size + 2];
    byte checksum_errors = 0;
    int j = 1;
    twi_cmd_arr[3] = slave_data_size;
    USE_SERIAL.printf_P("\n\r[%s] Dumping Flash Memory ...\n\n\r", __func__);
    USE_SERIAL.printf_P("Addr %04X: ", 0);
    for (word address = 0; address < flash_size; address += slave_data_size) {
        twi_cmd_arr[1] = ((address & 0xFF00) >> 8);                                                 /* Flash page address high byte */
        twi_cmd_arr[2] = (address & 0xFF);                                                          /* Flash page address low byte */
        twi_cmd_arr[4] = (byte)(twi_cmd_arr[0] + twi_cmd_arr[1] + twi_cmd_arr[2] + twi_cmd_arr[3]); /* READFLSH Checksum */
        byte twi_cmd_err = TwiCmdXmit(twi_cmd_arr, cmd_size, ACKRDFSH, twi_reply_arr, slave_data_size + 2);
        if (twi_cmd_err == 0) {
            byte checksum = 0;
            for (byte i = 1; i < (slave_data_size + 1); i++) {
                USE_SERIAL.printf_P("%02X", twi_reply_arr[i]); /* Memory values */
                if (j == values_per_line) {
                    USE_SERIAL.printf_P("\n\r");
                    if ((address + slave_data_size) < flash_size) {
                        USE_SERIAL.printf_P("Addr %04X: ", address + slave_data_size); /* Page address */
                    }
                    j = 0;
                } else {
                    USE_SERIAL.printf_P(" "); /* Space between values */
                }
                j++;
                checksum += (byte)twi_reply_arr[i];
            }
            if (checksum != twi_reply_arr[slave_data_size + 1]) {
                USE_SERIAL.printf_P("\n\r   ### Checksum ERROR! ###   %d\n\r", checksum);
                //USE_SERIAL.printf_P("%d\n\r", checksum + 1);
                //USE_SERIAL.printf_P(" <-- calculated, received --> %d\n\r", twi_reply_arr[slave_data_size + 1]);
                if (checksum_errors++ == MAXCKSUMERRORS) {
                    USE_SERIAL.printf_P("[%s] Too many Checksum ERRORS, stopping! \n\r", __func__);
                    delay(1000);
                    return (2);
                }
            }
        } else {
            USE_SERIAL.printf_P("[%s] DumpFlashMem Error parsing %d command <<< %d\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
            return (1);
        }
        delay(150); /* Verify if this delay matters on multi-slave setups: 50 --> 250 */
    }
    USE_SERIAL.printf_P("\n\r");
    return (OK);
}
