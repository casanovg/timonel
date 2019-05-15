/*
 *  NB Micro TWI Master Library
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: NbMicro.cpp (Library)
 *  ........................................... 
 *  Version: 1.2 / 2019-01-16
 *  gustavo.casanova@nicebots.com
 *  ...........................................
 *  This TWI (I2C) master library handles the communication protocol
 *  with slave devices that implement the NB command set over a TWI
 *  bus. In addition, the TwiBus class has methods to scan the bus in
 *  search of the existing slave devices addresses. For single-slave
 *  setups or those where all addresses are known in advance, this
 *  last one could be dropped to save memory.
 */

#include "NbMicro.h"
#include "TimonelTwiM.h"

/////////////////////////////////////////////////////////////////////////////
////////////                    NBMICRO CLASS                    ////////////
/////////////////////////////////////////////////////////////////////////////

// Class constructor
NbMicro::NbMicro(byte twi_address, byte sda, byte scl) : addr_(twi_address), sda_(sda), scl_(scl) {
    if ((!active_addresses.insert(addr_).second) && (addr_ != 0)) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Error: The TWI address [%02d] is in use! Unable to create another device object with it ...\r\n", __func__, addr_);
        USE_SERIAL.printf_P("[%s] Execution terminated, please review the devices' TWI addresses on your code.\r\n", __func__);
#endif /* DEBUG_LEVEL */
        delay(DLY_NBMICRO);
        std::terminate();
        //throw std::logic_error(addr_.toString() + " is already in use");
        //exit(1);
    }
    if (!((sda_ == 0) && (scl_ == 0))) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Creating a new TWI connection with address %02d\n\r", __func__, addr_);
#endif /* DEBUG_LEVEL */
        Wire.begin(sda_, scl_); /* Init I2C sda_:GPIO0, scl_:GPIO2 (ESP-01) / sda_:D3, scl_:D4 (NodeMCU) */
        reusing_twi_connection_ = false;
    } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Reusing the TWI connection with address %02d\n\r", __func__, addr_);
#endif /* DEBUG_LEVEL */
        reusing_twi_connection_ = true;
    }
}

/* _________________________
  |                         | 
  |      GetTwiAddress      |
  |_________________________|
*/
// Return this object's TWI address
byte NbMicro::GetTwiAddress(void) {
    return addr_;
}

/* _________________________
  |                         | 
  |      SetTwiAddress      |
  |_________________________|
*/
// Set this object's TWI address (allowed only once if it wasn't set at object creation time)
byte NbMicro::SetTwiAddress(byte twi_address) {
    if (addr_ != 0) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Object TWI address already defined, using %d\r\n", __func__, addr_);
#endif /* DEBUG_LEVEL */
        return ERR_ADDR_IN_USE;
    } else {
        addr_ = twi_address;
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Object TWI address correctly set to %d\r\n", __func__, addr_);
#endif /* DEBUG_LEVEL */
        return OK;
    }
}

/* _________________________________________________
  |                                                 | 
  | TwiCmdXmit (single byte command)                |
  | - If no error                       -> return 0 |
  | - If ack error on single byte reply -> return 3 |
  | - If ack error on multi byte reply  -> return 4 |
  |_________________________________________________|
*/
// Send a TWI command to the microcontroller (Overload A: single byte command)
byte NbMicro::TwiCmdXmit(byte twi_cmd, byte twi_reply, byte twi_reply_arr[], byte reply_size) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
    USE_SERIAL.printf_P("[%s] > Single byte cmd: 0x%02X, calling multi ...\n\r", __func__, twi_cmd);
#endif /* DEBUG_LEVEL */
    const byte cmd_size = 1;
    byte twi_cmd_arr[cmd_size] = {twi_cmd};
    byte twi_errors = TwiCmdXmit(twi_cmd_arr, cmd_size, twi_reply, twi_reply_arr, reply_size);
    if (twi_errors != 0) {
        return (twi_errors + 2);
    } else {
        return OK;
    }
}

/* _________________________________________________
  |                                                 | 
  | TwiCmdXmit (multibyte command)                  |
  | - If no error                       -> return 0 |
  | - If ack error on single byte reply -> return 1 |
  | - If ack error on multi byte reply  -> return 2 |
  |_________________________________________________|
*/
// Send a TWI command to the microcontroller (Overload B: multibyte command)
byte NbMicro::TwiCmdXmit(byte twi_cmd_arr[], byte cmd_size, byte twi_reply, byte twi_reply_arr[], byte reply_size) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
    USE_SERIAL.printf_P("[%s] > Multi byte cmd: 0x%02X --> making actual TWI transmission ...\n\r", __func__, twi_cmd_arr[0]);
#endif /* DEBUG_LEVEL */
    // TWI command transmit
    for (int i = 0; i < cmd_size; i++) {
        Wire.beginTransmission(addr_);
        Wire.write(twi_cmd_arr[i]);
        Wire.endTransmission();
    }
    // TWI command reply (one byte expected)
    if (reply_size == 0) {
        Wire.requestFrom(addr_, ++reply_size, STOP_ON_REQ); /* True: releases the bus with a stop after a master request. */
        byte reply = Wire.read();                           /* False: sends a restart, not releasing the bus.             */
        if (reply == twi_reply) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
            USE_SERIAL.printf_P("[%s] > Command 0x%02X parsed OK <<< 0x%02X (single byte reply)\n\r", __func__, twi_cmd_arr[0], reply);
#endif /* DEBUG_LEVEL */
            return OK;
        } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
            USE_SERIAL.printf_P("[%s] > Error parsing 0x%02X command <<< 0x%02X (single byte reply)\n\r", __func__, twi_cmd_arr[0], reply);
#endif /* DEBUG_LEVEL */
            return ERR_CMD_PARSE_S; /* Error: reply doesn't match command (single byte) */
        }
    }
    // TWI command reply (multiple bytes expected)
    else {
        byte reply_length = Wire.requestFrom(addr_, reply_size, STOP_ON_REQ); /* True: releases the bus with a stop after a master request. */
        for (int i = 0; i < reply_size; i++) {                                /* False: sends a restart, not releasing the bus.             */
            twi_reply_arr[i] = Wire.read();
        }
        if ((twi_reply_arr[0] == twi_reply) && (reply_length == reply_size)) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 3))
            USE_SERIAL.printf_P("[%s] >>> Command 0x%02X parsed OK <<< 0x%02X (multibyte reply)\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
#endif /* DEBUG_LEVEL */
            return OK;
        } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 2))
            USE_SERIAL.printf_P("[%s] > Error parsing 0x%02X command <<< 0x%02X (multibyte reply)\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
#endif /* DEBUG_LEVEL */
            return ERR_CMD_PARSE_M; /* Error: reply doesn't match command (multi byte) */
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
////////////             NbMicro internal functions              ////////////
/////////////////////////////////////////////////////////////////////////////

// Class destructor
NbMicro::~NbMicro() {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("[%s] Freeing TWI address %02d ...\r\n", __func__, addr_);
#endif /* DEBUG_LEVEL */
    active_addresses.erase(addr_);
}

// Function InitMicro (Initializes the microcontroller firmware)
byte NbMicro::InitMicro(void) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("[%s] Initializing Micro %02d ...\r\n", __func__, addr_);
#endif /* DEBUG_LEVEL */
    return (TwiCmdXmit(INITSOFT, ACKINITS));
}


#if ((defined MULTI_DEVICE) && (MULTI_DEVICE == true))
    #pragma GCC warning "TwiBus device discovery functions code included in TWI master!"
/////////////////////////////////////////////////////////////////////////////
////////////                    TWIBUS CLASS                     ////////////
/////////////////////////////////////////////////////////////////////////////

// Class constructor
TwiBus::TwiBus(byte sda, byte scl) : sda_(sda), scl_(scl) {
    if (!((sda == 0) && (scl == 0))) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Creating a new I2C connection\n\r", __func__);
#endif /* DEBUG_LEVEL */
        Wire.begin(sda, scl); /* Init I2C sda:GPIO0, scl:GPIO2 (ESP-01) / sda:D3, scl:D4 (NodeMCU) */
        reusing_twi_connection_ = false;
    } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Reusing I2C connection\n\r", __func__);
#endif /* DEBUG_LEVEL */
        reusing_twi_connection_ = true;
    }
}

/* _________________________
  |                         | 
  |        ScanBus A        |
  |_________________________|
*/
// ScanBus (Overload A: Return the address and mode of the first TWI device found on the bus)
byte TwiBus::ScanBus(bool *p_app_mode) {
    // Address 08 to 35: Timonel bootloader (app mode = false)
    // Address 36 to 63: Application firmware (app mode = true)
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("\n\r[%s] Scanning TWI bus, looking for the first device (lowest address) ...\n\r", __func__);
#endif /* DEBUG_LEVEL */
    byte twi_addr = LOW_TWI_ADDR;
    while (twi_addr < HIG_TWI_ADDR) {
        Wire.beginTransmission(twi_addr);
        if (Wire.endTransmission() == 0) {
            if (twi_addr < (((HIG_TWI_ADDR + 1 - LOW_TWI_ADDR) / 2) + LOW_TWI_ADDR)) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                USE_SERIAL.printf_P("[%s] Timonel bootloader found at address: %d (0x%X)\n\r", __func__, twi_addr, twi_addr);
#endif /* DEBUG_LEVEL */
                *p_app_mode = false;
            } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                USE_SERIAL.printf_P("[%s] Application firmware found at address: %d (0x%X)\n\r", __func__, twi_addr, twi_addr);
#endif /* DEBUG_LEVEL */
                *p_app_mode = true;
            }
            return (twi_addr);
        }
        delay(DLY_SCAN_BUS);
        twi_addr++;
    }
    return OK;
}

/* _________________________
  |                         | 
  |        ScanBus B        |
  |_________________________|
*/
// ScanBus (Overload B: Fills an array with the address, firmware and version of all devices connected to the bus)
byte TwiBus::ScanBus(DeviceInfo dev_info_arr[], byte arr_size, byte start_twi_addr) {
    // Address 08 to 35: Timonel bootloader
    // Address 36 to 63: Application firmware
    // Each I2C slave must have a unique bootloader address that corresponds
    // to a defined application address, as shown in this table:
    // T: |08|09|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|
    // A: |36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
    USE_SERIAL.printf_P("\n\r[%s] Scanning TWI bus, searching all the connected devices, please wait ...\n\r", __func__);
#endif /* DEBUG_LEVEL */
    byte twi_addr = LOW_TWI_ADDR;
    while (twi_addr <= HIG_TWI_ADDR) {
        Wire.beginTransmission(twi_addr);
        if (Wire.endTransmission() == 0) {
            if (twi_addr < (((HIG_TWI_ADDR + 1 - LOW_TWI_ADDR) / 2) + LOW_TWI_ADDR)) {
                Timonel tml(twi_addr);
                Timonel::Status sts = tml.GetStatus();
                dev_info_arr[twi_addr - start_twi_addr].addr = twi_addr;
                if (sts.signature == T_SIGNATURE) {
                    dev_info_arr[twi_addr - start_twi_addr].firmware = L_TIMONEL;
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
                    USE_SERIAL.printf_P("\n\r[%s] Scanning TWI bus, searching all the connected devices, please wait ...\n\r", __func__);
#endif /* DEBUG_LEVEL */
                } else {
                    dev_info_arr[twi_addr - start_twi_addr].firmware = L_UNKNOWN;
                }
                dev_info_arr[twi_addr - start_twi_addr].version_major = sts.version_major;
                dev_info_arr[twi_addr - start_twi_addr].version_minor = sts.version_minor;
            } else {
                dev_info_arr[twi_addr - start_twi_addr].addr = twi_addr;
                dev_info_arr[twi_addr - start_twi_addr].firmware = L_APP;
                dev_info_arr[twi_addr - start_twi_addr].version_major = 0;
                dev_info_arr[twi_addr - start_twi_addr].version_minor = 0;
            }
        }
        delay(DLY_SCAN_BUS);
        twi_addr++;
    }
    return OK;
}
#else
    #pragma GCC warning "TwiBus device discovery functions code included in TWI master!"
#endif /* MULTI_DEVICE */