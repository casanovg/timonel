/*
  NbMicro.cpp
  ===========
  Library header for TWI (I2C) communications with
  a microcontroller using the NB command set.
  ---------------------------
  2019-03-12 Gustavo Casanova
  ---------------------------
*/

#include "NbMicro.h"
#include "TimonelTwiM.h"

// Class constructor
NbMicro::NbMicro(byte twi_address, byte sda, byte scl) : addr_(twi_address), sda_(sda), scl_(scl) {
    if ((!in_use.insert(addr_).second) && (addr_ != 0)) {
        USE_SERIAL.printf_P("[%s] Error: The TWI address [%02d] is in use! Unable to create another device object with it ...\r\n", __func__, addr_);
        USE_SERIAL.printf_P("[%s] Execution terminated, please review the devices' TWI addresses on your code.\r\n", __func__, addr_);
        delay(2000);
        std::terminate();
        //throw std::logic_error(addr_.toString() + " is already in use");
        //exit(1);
    }
    if (!((sda_ == 0) && (scl_ == 0))) {
        USE_SERIAL.printf_P("[%s] Creating a new TWI connection with address %02d\n\r", __func__, addr_);
        Wire.begin(sda_, scl_); /* Init I2C sda_:GPIO0, scl_:GPIO2 (ESP-01) / sda_:D3, scl_:D4 (NodeMCU) */
        reusing_twi_connection_ = false;
    } else {
        USE_SERIAL.printf_P("[%s] Reusing the TWI connection with address %02d\n\r", __func__, addr_);
        reusing_twi_connection_ = true;
    }
}

// Class destructor
NbMicro::~NbMicro() {
    USE_SERIAL.printf_P("[%s] Freeing TWI address %02d ...\r\n", __func__, addr_);
    in_use.erase(addr_);
}

// Returns this object's TWI address
byte NbMicro::GetTwiAddress(void) {
    return (addr_);
}

// Sets this object's TWI address (allowed only once, if it wasn't set at object creation time)
byte NbMicro::SetObjTwiAddress(byte twi_address) {
    if (addr_ == 0) {
        addr_ = twi_address;
        USE_SERIAL.printf_P("[%s] TWI address correctly set to %d\r\n", __func__, addr_);
        return (0);
    } else {
        USE_SERIAL.printf_P("[%s] TWI address already defined, using %d\r\n", __func__, addr_);
        return (1);
    }
}

// Member Function InitMicro
byte NbMicro::InitMicro(void) {
    return (TwiCmdXmit(INITSOFT, ACKINITS));
}

// Sends a TWI command to the microcontroller (Overload A: single byte command)
byte NbMicro::TwiCmdXmit(byte twi_cmd, byte twi_reply, byte twi_reply_arr[], byte reply_size) {
    USE_SERIAL.printf_P("[%s] > Single: 0x%02X, calling multi ...\n\r", __func__, twi_cmd);
    const byte cmd_size = 1;
    byte twi_cmd_arr[cmd_size] = {twi_cmd};
    return (TwiCmdXmit(twi_cmd_arr, cmd_size, twi_reply, twi_reply_arr, reply_size));
}

// Sends a TWI command to the microcontroller (Overload B: multibyte command)
byte NbMicro::TwiCmdXmit(byte twi_cmd_arr[], byte cmd_size, byte twi_reply, byte twi_reply_arr[], byte reply_size) {
    #define SLOW_TML_DLY 3
    USE_SERIAL.printf_P("[%s] >> Multi: 0x%02X --> making actual TWI transmission ...\n\r", __func__, twi_cmd_arr[0]);
    // TWI command transmit
    for (int i = 0; i < cmd_size; i++) {
        Wire.beginTransmission(addr_);
        delay(SLOW_TML_DLY); /* Delay test for Timonel @ 8 Mhz */
        Wire.write(twi_cmd_arr[i]);
        Wire.endTransmission();
    }
    // Twi command reply
    if (reply_size == 0) {
        Wire.requestFrom(addr_, ++reply_size, true); /* True: releases the bus with a stop after a master request. */
        byte reply = Wire.read();                    /* False: sends a restart, not releasing the bus.             */
        delay(SLOW_TML_DLY); /* Delay test for Timonel @ 8 Mhz */
        if (reply == twi_reply) {
            USE_SERIAL.printf_P("[%s] Command 0x%02X parsed OK <<< %d\n\r", __func__, twi_cmd_arr[0], reply);
            return (0);
        } else {
            USE_SERIAL.printf_P("[%s] Error parsing 0x%02X command <<< %d\n\r", __func__, twi_cmd_arr[0], reply);
            return (1);
        }
    } else {
        byte reply_length = Wire.requestFrom(addr_, reply_size, true); /* True: releases the bus with a stop after a master request. */
        for (int i = 0; i < reply_size; i++) {                         /* False: sends a restart, not releasing the bus.             */
            twi_reply_arr[i] = Wire.read();
            delay(SLOW_TML_DLY); /* Delay test for Timonel @ 8 Mhz */
        }
        if ((twi_reply_arr[0] == twi_reply) && (reply_length == reply_size)) {
            //USE_SERIAL.printf_P("[%s] Multibyte command %d parsed OK <<< %d\n\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
            return (0);
        } else {
            USE_SERIAL.printf_P("[%s] Error parsing 0x%02X multibyte command <<< %d\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
            return (2);
        }
    }
}

// Class constructor
TwiBus::TwiBus(byte sda, byte scl) : sda_(sda), scl_(scl) {
    if (!((sda == 0) && (scl == 0))) {
        USE_SERIAL.printf_P("[%s] Creating a new I2C connection\n\r", __func__);
        Wire.begin(sda, scl); /* Init I2C sda:GPIO0, scl:GPIO2 (ESP-01) / sda:D3, scl:D4 (NodeMCU) */
        reusing_twi_connection_ = false;
    } else {
        USE_SERIAL.printf_P("[%s] Reusing I2C connection\n\r", __func__);
        reusing_twi_connection_ = true;
    }
}

// ScanBus (Overload A: Returns the address and mode of the first TWI device found on the bus)
byte TwiBus::ScanBus(bool *p_app_mode) {
    // Address 08 to 35: Timonel bootloader
    // Address 36 to 63: Application firmware
    USE_SERIAL.println("\n\rScanning TWI bus, looking for the first device (lowest address) ...\n\r");
    byte twi_addr = MIN_TWI_ADDR;
    while (twi_addr < MAX_TWI_ADDR) {
        Wire.beginTransmission(twi_addr);
        if (Wire.endTransmission() == 0) {
            if (twi_addr < (((MAX_TWI_ADDR + 1 - MIN_TWI_ADDR) / 2) + MIN_TWI_ADDR)) {
                USE_SERIAL.printf_P("Timonel bootloader found at address: %d (0x%X)\n\r", twi_addr, twi_addr);
                *p_app_mode = false;
            } else {
                USE_SERIAL.printf_P("Application firmware found at address: %d (0x%X)\n\r", twi_addr, twi_addr);
                *p_app_mode = true;
            }
            return (twi_addr);
        }
        delay(5);
        twi_addr++;
    }
}

// ScanBus (Overload B: Fills an array with the address, firmware, and version of all devices connected to the bus)
byte TwiBus::ScanBus(DeviceInfo dev_info_arr[], byte arr_size, byte start_twi_addr) {
    // Address 08 to 35: Timonel bootloader
    // Address 36 to 63: Application firmware
    // Each I2C slave must have a unique bootloader address that corresponds
    // to a defined application address, as shown in this table:
    // T: |08|09|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|
    // A: |36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|
    USE_SERIAL.println("\n\rScanning TWI bus, searching all the connected devices, please wait ...\n\r");
    byte twi_addr = MIN_TWI_ADDR;
    while (twi_addr <= MAX_TWI_ADDR) {
        Wire.beginTransmission(twi_addr);
        if (Wire.endTransmission() == 0) {
            if (twi_addr < (((MAX_TWI_ADDR + 1 - MIN_TWI_ADDR) / 2) + MIN_TWI_ADDR)) {
                Timonel tml(twi_addr);
                Timonel::Status sts = tml.GetStatus();
                dev_info_arr[twi_addr - start_twi_addr].addr = twi_addr;
                if (sts.signature == 84) {
                    dev_info_arr[twi_addr - start_twi_addr].firmware = "Timonel";
                } else {
                    dev_info_arr[twi_addr - start_twi_addr].firmware = "Unknown";
                }
                dev_info_arr[twi_addr - start_twi_addr].version_major = sts.version_major;
                dev_info_arr[twi_addr - start_twi_addr].version_minor = sts.version_minor;
            } else {
                dev_info_arr[twi_addr - start_twi_addr].addr = twi_addr;
                dev_info_arr[twi_addr - start_twi_addr].firmware = "Application";
                dev_info_arr[twi_addr - start_twi_addr].version_major = 0;
                dev_info_arr[twi_addr - start_twi_addr].version_minor = 0;
            }
        }
        // else {
        //  dev_info_arr[twi_addr - start_twi_addr].addr = 0;
        //  dev_info_arr[twi_addr - start_twi_addr].firmware = "No device";
        //  dev_info_arr[twi_addr - start_twi_addr].version_major = 0;
        //  dev_info_arr[twi_addr - start_twi_addr].version_minor = 0;
        // }
        delay(5);
        twi_addr++;
    }
    return (0);
}
