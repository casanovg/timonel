/*
 *  NB Micro TWI Master Library
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: NbMicro.h (Header)
 *  ........................................... 
 *  Version: 1.3 / 2019-06-06
 *  gustavo.casanova@nicebots.com
 *  ...........................................
 *  This TWI (I2C) master library handles the communication protocol
 *  with slave devices that implement the NB command set over a TWI
 *  bus. In addition, the TwiBus class has methods to scan the bus in
 *  search of the existing slave devices addresses. For single-slave
 *  setups or those where all addresses are known in advance, this
 *  last one could be dropped to save memory.
 */

#ifndef _NBMICRO_H_
#define _NBMICRO_H_

#include <unordered_set>
#include "../../cmd/nb-twi-cmd.h"
#include "Arduino.h"
#include "Wire.h"
#include "libconfig.h"
#include "stdbool.h"

// Store of TWI addresses in use ...
static std::unordered_set<byte> active_addresses;

/* 
 * ===================================================================
 * Class NbMicro: Represents a microcontroller using
 * the NB command set connected to the TWI bus
 * ===================================================================
 */
class NbMicro {
   public:
    NbMicro(byte twi_address = 0, byte sda = 0, byte scl = 0);
    byte GetTwiAddress(void);
    byte SetTwiAddress(byte twi_address);
    byte TwiCmdXmit(byte twi_cmd, byte twi_reply,
                    byte twi_reply_arr[] = nullptr, byte reply_size = 0);
    byte TwiCmdXmit(byte twi_cmd_arr[], byte cmd_size, byte twi_reply,
                    byte twi_reply_arr[] = nullptr, byte reply_size = 0);

   protected:
    ~NbMicro();
    byte InitMicro(void);
    byte addr_ = 0, sda_ = 0, scl_ = 0;
    bool reusing_twi_connection_ = true;
    //std::unordered_set<byte> in_use;
    //byte addr_pool_[28] = { 0 };
   private:
};

#if ((defined MULTI_DEVICE) && (MULTI_DEVICE == true))
// Class TwiBus: Represents a Two Wire Interfase (I2C) bus
class TwiBus {
   public:
    typedef struct device_info_ {
        byte addr = 0;
        String firmware = "";
        byte version_major = 0;
        byte version_minor = 0;
    } DeviceInfo;
    TwiBus(byte sda = 0, byte scl = 0);
    byte ScanBus(bool *p_app_mode = nullptr);
    byte ScanBus(DeviceInfo dev_info_arr[] = nullptr,
                 //byte arr_size = ((HIG_TWI_ADDR + 1) - LOW_TWI_ADDR) / 2,
                 byte arr_size = HIG_TWI_ADDR + 1,
                 byte start_twi_addr = LOW_TWI_ADDR);

   private:
    byte sda_ = 0, scl_ = 0;
    bool reusing_twi_connection_ = true;
};
#endif /* MULTI_DEVICE */

#endif /* _NBMICRO_H_ */
