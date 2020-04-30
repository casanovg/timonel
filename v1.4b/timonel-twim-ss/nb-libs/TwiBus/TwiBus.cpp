/*
 *  TWI Bus Master Library
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: TwiBus.cpp (Library)
 *  ........................................... 
 *  Version: 1.4 / 2020-04-29
 *  gustavo.casanova@nicebots.com
 *  ...........................................
 *  The TwiBus class has methods to scan the bus in
 *  search of the existing slave devices addresses.
 */

#include "TwiBus.h"

/////////////////////////////////////////////////////////////////////////////
////////////                    TWIBUS CLASS                     ////////////
/////////////////////////////////////////////////////////////////////////////

// Class constructor
TwiBus::TwiBus(byte sda, byte scl) : sda_(sda), scl_(scl) {
    if (!((sda == 0) && (scl == 0))) {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Creating a new I2C connection\n\r", __func__);
#endif                        /* DEBUG_LEVEL */
        Wire.begin(sda, scl); /* Init I2C sda:GPIO0, scl:GPIO2 (ESP-01) / sda:D3, scl:D4 (NodeMCU) */
        reusing_twi_connection_ = false;
    } else {
#if ((defined DEBUG_LEVEL) && (DEBUG_LEVEL >= 1))
        USE_SERIAL.printf_P("[%s] Reusing I2C connection\n\r", __func__);
#endif /* DEBUG_LEVEL */
        reusing_twi_connection_ = true;
    }
}

// Class destructor
TwiBus::~TwiBus() {
    // Destructor
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
            if (p_app_mode != nullptr) {
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
            }
            return twi_addr;
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
    byte found_devices = 0;
    byte twi_addr = LOW_TWI_ADDR;
    while (twi_addr <= HIG_TWI_ADDR) {
        Wire.beginTransmission(twi_addr);
        if (Wire.endTransmission() == 0) {
            if (twi_addr < (((HIG_TWI_ADDR + 1 - LOW_TWI_ADDR) / 2) + LOW_TWI_ADDR)) {
                Timonel tml(twi_addr);
                Timonel::Status sts = tml.GetStatus();
                dev_info_arr[found_devices].addr = twi_addr;
                if (sts.signature == T_SIGNATURE) {
                    dev_info_arr[found_devices].firmware = L_TIMONEL;
                } else {
                    dev_info_arr[found_devices].firmware = L_UNKNOWN;
                }
                dev_info_arr[found_devices].version_major = sts.version_major;
                dev_info_arr[found_devices].version_minor = sts.version_minor;
            } else {
                dev_info_arr[found_devices].addr = twi_addr;
                dev_info_arr[found_devices].firmware = L_APP;
                dev_info_arr[found_devices].version_major = 0;
                dev_info_arr[found_devices].version_minor = 0;
            }
            found_devices++;
        }
        //delay(DLY_SCAN_BUS);
        twi_addr++;
    }
    return OK;
}
