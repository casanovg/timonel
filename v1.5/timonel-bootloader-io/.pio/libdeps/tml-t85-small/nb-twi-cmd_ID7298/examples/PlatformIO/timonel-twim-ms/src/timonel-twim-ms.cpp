/*
  Timonel-TwiM-MS.cpp
  ===================
  Timonel libraries test program (Multi Slave) v1.5
  ----------------------------------------------------------------------------
  This demo shows how to control and update several Tiny85 microcontrollers
  running the Timonel bootloader from an ESP8266 master.
  It uses a serial console configured at 9600 N 8 1 for feedback.
  ----------------------------------------------------------------------------
  2020-06-03 Gustavo Casanova
  ----------------------------------------------------------------------------
*/

/*
 Working routine:
 ----------------
   1) Scans the TWI bus in search of all devices running Timonel.
   2) Creates an array of Timonel objects, one per device.
   3) Deletes existing firmware of each device.
   4) Uploads "avr-blink-twis.hex" application payload to each device.
   5) Launches application on each device, let it run 10 seconds.
   6) Sends reset command to the application: led blinking should stop on all devices.
   7) Repeats the routine 3 times.
*/

#include "timonel-twim-ms.h"

#include <NbMicro.h>
#include <TimonelTwiM.h>
#include <TwiBus.h>

#include "payload.h"

#define USE_SERIAL Serial
#define SDA 2  // I2C SDA pin - ESP8266 2 - ESP32 21
#define SCL 0  // I2C SCL pin - ESP8266 0 - ESP32 22
#define MAX_TWI_DEVS 28
#define LOOP_COUNT 3
#define T_SIGNATURE 84

// Global variables
uint8_t slave_address = 0;
uint8_t block_rx_size = 0;
bool app_mode = false;
uint8_t timonels = 0;
uint8_t applications = 0;
void (*resetFunc)(void) = 0;

// Setup block
void setup() {
    // Initialize the serial port for debugging
    USE_SERIAL.begin(9600);
    ClrScr();
    PrintLogo();
    ShowHeader();
    /*  ____________________
       |                    | 
       |    Routine loop    |
       |____________________|
    */    
    for (uint8_t loop = 0; loop < LOOP_COUNT; loop++) {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
        USE_SERIAL.printf_P("\n\rPASS %d OF %d ...\n\r", loop + 1, LOOP_COUNT);
#else   // -----
        USE_SERIAL.print("\n\rPASS ");
        USE_SERIAL.print(loop + 1);
        USE_SERIAL.print(" OF ");
        USE_SERIAL.println(LOOP_COUNT);
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
        // The bus device scanning it has to be made as fast as possible since each
        // discovered Timonel has to be initialized before launching the user apps
        TwiBus twi_bus(SDA, SCL);
        TwiBus::DeviceInfo dev_info_arr[HIG_TWI_ADDR - LOW_TWI_ADDR + 1];
        // Scanning the TWI bus in search of devices ...
        uint8_t tml_count = 0;
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
        USE_SERIAL.printf_P("\n\r");
#else   // -----
        USE_SERIAL.println("");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
        while (tml_count == 0) {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
            USE_SERIAL.printf_P("\r\x1b[5mScanning TWI bus ...\x1b[0m");
#else   // -----
            USE_SERIAL.print("\r\x1b[5mScanning TWI bus ...\x1b[0m");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
            twi_bus.ScanBus(dev_info_arr, HIG_TWI_ADDR - LOW_TWI_ADDR + 1, LOW_TWI_ADDR);
            uint8_t arr_size = (sizeof(dev_info_arr) / sizeof(dev_info_arr[0]));
            for (uint8_t i = 0; i < arr_size; i++) {
                if (dev_info_arr[i].firmware == "Timonel") {
                    tml_count++;
                }
            }
            if (tml_count > 0) {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\rTimonel devices found: %d\n\r", tml_count);
#else   // -----
                USE_SERIAL.print("\rTimonel devices found: ");
                USE_SERIAL.println(tml_count);
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
            } else {
                if (dev_info_arr[0].addr) {
                    USE_SERIAL.printf_P("\rDevice found at address %d NOT responding, resetting master ...\n\r", dev_info_arr[0].addr);
                    NbMicro *micro = new NbMicro(dev_info_arr[0].addr, SDA, SCL);
                    micro->TwiCmdXmit(RESETMCU, ACKRESET);
                    delete micro;
                    delay(5000);
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                    ESP.restart();
#else   // -----
                //resetFunc();
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                }
            }
            delay(1000);
        }
        Timonel *tml_pool[tml_count];
        //
        // **************************************************
        // * Create and initialize bootloader objects found *
        // **************************************************
        for (uint8_t i = 0; i <= (tml_count); i++) {
            if (dev_info_arr[i].firmware == "Timonel") {
                tml_pool[i] = new Timonel(dev_info_arr[i].addr, SDA, SCL);
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\n\rGetting status of Timonel device %d\n\r", dev_info_arr[i].addr);
#else   // -----
                USE_SERIAL.print("\n\rGetting status of Timonel device ");
                USE_SERIAL.println(dev_info_arr[i].addr);
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                Timonel::Status sts = tml_pool[i]->GetStatus();
                if ((sts.features_code >> F_APP_AUTORUN) & true) {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                    USE_SERIAL.printf_P("\n\r ***************************************************************************************\n\r");
                    USE_SERIAL.printf_P(" * WARNING! The Timonel bootloader with TWI address %02d has the \"APP_AUTORUN\" feature. *\n\r", dev_info_arr[i].addr);
                    USE_SERIAL.printf_P(" * enabled. This TWI master firmware can't control it properly! Please recompile it    *\n\r");
                    USE_SERIAL.printf_P(" * using a configuration with that option disabled (e.g. \"tml-t85-small\").             *\n\r");
                    USE_SERIAL.printf_P(" ***************************************************************************************\n\r");
#else   // -----
                    USE_SERIAL.println("\n\r ***************************************************************************************");
                    USE_SERIAL.print(" * WARNING! The Timonel bootloader with TWI address ");
                    USE_SERIAL.print(dev_info_arr[i].addr);
                    USE_SERIAL.println(" has the \"APP_AUTORUN\" feature. *");
                    USE_SERIAL.println(" * enabled. This TWI master firmware can't control it properly! Please recompile it    *");
                    USE_SERIAL.println(" * using a configuration with that option disabled (e.g. \"tml-t85-small\").             *");
                    USE_SERIAL.println(" ***************************************************************************************");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                }
            }
        }
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
        USE_SERIAL.printf_P("\n\r");
        ThreeStarDelay();
        USE_SERIAL.printf_P("\n\n\r");
#else   // -----
        USE_SERIAL.println("");
        ThreeStarDelay();
        USE_SERIAL.println("\n");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
        //
        // *********************************************
        // * Delete user applications from all devices *
        // *********************************************
        for (uint8_t i = 0; i <= (tml_count); i++) {
            if (dev_info_arr[i].firmware == "Timonel") {
                delay(10);
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("Deleting application on device %d ", dev_info_arr[i].addr);
#else   // -----
                USE_SERIAL.print("Deleting application on device ");
                USE_SERIAL.print(dev_info_arr[i].addr);
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                uint8_t errors = tml_pool[i]->DeleteApplication();
                // delay(500);
                // tml_pool[i]->TwiCmdXmit(RESETMCU, ACKRESET);
                // delay(500);
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                if (errors == 0) {
                    USE_SERIAL.printf_P("OK!\n\r");
                } else {
                    USE_SERIAL.printf_P("Error! (%d)\n\r", errors);
                    //Wire.begin(SDA, SCL);
                }
                delay(1000);
                USE_SERIAL.printf_P("\n\rGetting status of device %d\n\r", dev_info_arr[i].addr);
#else   // -----
                if (errors == 0) {
                    USE_SERIAL.println(" OK!");
                } else {
                    USE_SERIAL.print(" Error! ");
                    USE_SERIAL.println(errors);
                    //Wire.begin();
                }
                delay(1000);
                USE_SERIAL.print("\n\rGetting status of device ");
                USE_SERIAL.println(dev_info_arr[i].addr);
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                tml_pool[i]->GetStatus();
                PrintStatus(*tml_pool[i]);
            }
        }
        ThreeStarDelay();
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
        USE_SERIAL.printf_P("\n\r");
#else   // -----
        USE_SERIAL.println("");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
        //
        // ***************************************************
        // * Upload and run user applications on all devices *
        // ***************************************************
        for (uint8_t i = 0; i <= (tml_count); i++) {
            if (dev_info_arr[i].firmware == "Timonel") {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\n\rUploading application to device %d, \x1b[5mPLEASE WAIT\x1b[0m ...", dev_info_arr[i].addr);
#else   // -----
                USE_SERIAL.print("\n\rUploading application to device ");
                USE_SERIAL.print(dev_info_arr[i].addr);
                USE_SERIAL.print("\x1b[5m PLEASE WAIT\x1b[0m ...");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                uint8_t errors = tml_pool[i]->UploadApplication(payload, sizeof(payload));
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                if (errors == 0) {
                    USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b successful!      \n\r");
                } else {
                    USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b error! (%d)           \n\r", errors);
                }
#else   // -----
                if (errors == 0) {
                    USE_SERIAL.println("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b successful!      ");
                } else {
                    USE_SERIAL.print("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b error! ");
                    USE_SERIAL.print(errors);
                    USE_SERIAL.println("             ");
                }
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                delay(10);
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\n\rGetting status of device %d\n\r", dev_info_arr[i].addr);
#else   // -----
                USE_SERIAL.print("\n\rGetting status of device ");
                USE_SERIAL.println(dev_info_arr[i].addr);
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                tml_pool[i]->GetStatus();
                PrintStatus(*tml_pool[i]);
                delay(10);
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                // // If the Timonel features support it, dump the device memory
                // Timonel::Status sts = tml_pool[i]->GetStatus();
                // if ((sts.features_code >> F_CMD_READFLASH) & true) {
                //     USE_SERIAL.printf_P("\n\rDumping device %d flash memory\n\r", dev_info_arr[i].addr);
                //     tml_pool[i]->DumpMemory(MCU_TOTAL_MEM, SLV_PACKET_SIZE, 32);
                // }
                USE_SERIAL.printf_P("Running application on device %d\n\r", dev_info_arr[i].addr);
#else   // -----
                USE_SERIAL.print("\n\rRunning application on device ");
                USE_SERIAL.println(dev_info_arr[i].addr);
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                errors = tml_pool[i]->RunApplication();
                delay(500);
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                if (errors == 0) {
                    USE_SERIAL.printf_P("User application should be running\n\r");
                } else {
                    USE_SERIAL.printf_P("Bootloader exit to app error! (%d)           \n\r", errors);
                }
#else   // -----
                if (errors == 0) {
                    USE_SERIAL.println("User application should be running");
                } else {
                    USE_SERIAL.print("Bootloader exit to app error! ");
                    USE_SERIAL.println(errors);
                }
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM

                delay(10);
                delete tml_pool[i];
                delay(1500);
            }
        }
        //
        // ************************************************************************
        // * Reset applications and prepare for another cycle, then clean objects *
        // ************************************************************************
        if (loop < LOOP_COUNT - 1) {
            uint8_t dly = 10;
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
            USE_SERIAL.printf_P("\n\rLetting application run %d seconds before resetting and starting next cycle   ", dly);
            while (dly--) {
                USE_SERIAL.printf_P("\b\b| ");
                delay(250);
                USE_SERIAL.printf_P("\b\b/ ");
                delay(250);
                USE_SERIAL.printf_P("\b\b- ");
                delay(250);
                USE_SERIAL.printf_P("\b\b\\ ");
                delay(250);
            }
            USE_SERIAL.printf_P("\b\b* ");
            USE_SERIAL.printf_P("\n\n\r");
#else   // -----
            USE_SERIAL.print("\n\rLetting application run ");
            USE_SERIAL.print(dly);
            USE_SERIAL.print(" seconds before resetting and starting next cycle   ");
            while (dly--) {
                USE_SERIAL.print("\b\b| ");
                delay(250);
                USE_SERIAL.print("\b\b/ ");
                delay(250);
                USE_SERIAL.print("\b\b- ");
                delay(250);
                USE_SERIAL.print("\b\b\\ ");
                delay(250);
            }
            USE_SERIAL.print("\b\b* ");
            USE_SERIAL.print("\n\n\r");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
            // Resetting devices
            // NOTE: All devices share the same application, since the application TWI address is
            // set at compile time, this is shared across all devices when the app is running.
            // Once discovered, the app TWI address is used to send the reset command to all devices.
            uint8_t app_addr = twi_bus.ScanBus();
            //NbMicro *micro = new NbMicro(0, SDA, SCL);
            NbMicro *micro = new NbMicro(app_addr, SDA, SCL);
            //micro->SetTwiAddress(app_addr); /* NOTE: All devices share the same TWI application address (44) */
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
            USE_SERIAL.printf_P("Resetting devices running application at address %d\n\r", micro->GetTwiAddress());
#else   // -----
            USE_SERIAL.print("Resetting devices running application at address ");
            USE_SERIAL.println(micro->GetTwiAddress());
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
            micro->TwiCmdXmit(RESETMCU, ACKRESET);
            delay(1000);
            delete micro;
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
            Wire.begin(SDA, SCL);
#else   // -----
            Wire.begin();
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
        } else {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
            USE_SERIAL.printf_P("\n\rCycle completed %d of %d passes! Letting application run ...\n\n\r", LOOP_COUNT, LOOP_COUNT);
#else   // -----
            USE_SERIAL.print("\n\rCycle completed ");
            USE_SERIAL.print(LOOP_COUNT);
            USE_SERIAL.print(" of ");
            USE_SERIAL.print(LOOP_COUNT);
            USE_SERIAL.println(" passes! Letting application run ...\n");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
        }
        // for (uint8_t i = 0; i < tml_count; i++) {
        //     delete tml_pool[i];
        // }
    }
    delay(3000);
}

// Main loop
void loop() {
    // Nothing
}

// Determine if there is a user application update available
bool CheckApplUpdate(void) {
    return false;
}

// Function clear screen
void ClrScr() {
    USE_SERIAL.write(27);     // ESC command
    USE_SERIAL.print("[2J");  // clear screen command
    USE_SERIAL.write(27);     // ESC command
    USE_SERIAL.print("[H");   // cursor to home command
}

// Function PrintLogo
void PrintLogo(void) {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
    USE_SERIAL.printf_P("        _                         _\n\r");
    USE_SERIAL.printf_P("    _  (_)                       | |\n\r");
    USE_SERIAL.printf_P("  _| |_ _ ____   ___  ____  _____| |\n\r");
    USE_SERIAL.printf_P(" (_   _) |    \\ / _ \\|  _ \\| ___ | |\n\r");
    USE_SERIAL.printf_P("   | |_| | | | | |_| | | | | ____| |\n\r");
    USE_SERIAL.printf_P("    \\__)_|_|_|_|\\___/|_| |_|_____)\\_)\n\r");
#else   // -----
    USE_SERIAL.print("        _                         _\n\r");
    USE_SERIAL.print("    _  (_)                       | |\n\r");
    USE_SERIAL.print("  _| |_ _ ____   ___  ____  _____| |\n\r");
    USE_SERIAL.print(" (_   _) |    \\ / _ \\|  _ \\| ___ | |\n\r");
    USE_SERIAL.print("   | |_| | | | | |_| | | | | ____| |\n\r");
    USE_SERIAL.print("    \\__)_|_|_|_|\\___/|_| |_|_____)\\_)\n\r");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
}

// Function print Timonel instance status
void PrintStatus(Timonel timonel) {
    Timonel::Status tml_status = timonel.GetStatus(); /* Get the instance id parameters received from the ATTiny85 */
    uint8_t twi_address = timonel.GetTwiAddress();
    uint8_t version_major = tml_status.version_major;
    uint8_t version_minor = tml_status.version_minor;
    if ((tml_status.signature == T_SIGNATURE) && ((version_major != 0) || (version_minor != 0))) {
        String version_mj_nick = "";
        switch (version_major) {
            case 0: {
                version_mj_nick = "\"Pre-release\"";
                break;
            }
            case 1: {
                version_mj_nick = "\"Sandra\"";
                break;
            }
            default: {
                version_mj_nick = "\"Unknown\"";
                break;
            }
        }
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
        USE_SERIAL.printf_P("\n\r Timonel v%d.%d %s ", version_major, version_minor, version_mj_nick.c_str());
        USE_SERIAL.printf_P("(TWI: %02d)\n\r", twi_address);
        USE_SERIAL.printf_P(" ====================================\n\r");
        USE_SERIAL.printf_P(" Bootloader address: 0x%X\n\r", tml_status.bootloader_start);
        uint16_t app_start = tml_status.application_start;
        if (app_start != 0xFFFF) {
            USE_SERIAL.printf_P("  Application start: 0x%X (0x%X)\n\r", app_start, tml_status.trampoline_addr);
        } else {
            USE_SERIAL.printf_P("  Application start: 0x%X (Not Set)\n\r", app_start);
        }
        USE_SERIAL.printf_P("      Features code: %d | %d ", tml_status.features_code, tml_status.ext_features_code);
        if ((tml_status.ext_features_code >> F_AUTO_CLK_TWEAK) & true) {
            USE_SERIAL.printf_P("(Auto)");
        } else {
            USE_SERIAL.printf_P("(Fixed)");
        }
        USE_SERIAL.printf_P("\n\r");
        USE_SERIAL.printf_P("           Low fuse: 0x%02X\n\r", tml_status.low_fuse_setting);
        USE_SERIAL.printf_P("             RC osc: 0x%02X\n\n\r", tml_status.oscillator_cal);
#else   // -----
        USE_SERIAL.print("\n\r Timonel v");
        USE_SERIAL.print(version_major);
        USE_SERIAL.print(".");
        USE_SERIAL.print(version_minor);
        USE_SERIAL.print(" ");
        USE_SERIAL.print(version_mj_nick.c_str());
        USE_SERIAL.print(" TWI: ");
        USE_SERIAL.println(twi_address);
        USE_SERIAL.println(" ====================================");
        USE_SERIAL.print(" Bootloader address: 0x");
        USE_SERIAL.println(tml_status.bootloader_start, HEX);
        uint16_t app_start = tml_status.application_start;
        if (app_start != 0xFFFF) {
            USE_SERIAL.print("  Application start: 0x");
            USE_SERIAL.print(app_start, HEX);
            USE_SERIAL.print(" - 0x");
            USE_SERIAL.println(tml_status.trampoline_addr, HEX);
        } else {
            USE_SERIAL.print("  Application start: Not set: 0x");
            USE_SERIAL.println(app_start, HEX);
        }
        USE_SERIAL.print("      Features code: ");
        USE_SERIAL.print(tml_status.features_code);
        USE_SERIAL.print(" | ");
        USE_SERIAL.print(tml_status.ext_features_code);
        if ((tml_status.ext_features_code >> F_AUTO_CLK_TWEAK) & true) {
            USE_SERIAL.print(" (Auto)");
        } else {
            USE_SERIAL.print(" (Fixed)");
        }
        USE_SERIAL.println("");
        USE_SERIAL.print("           Low fuse: 0x");
        USE_SERIAL.println(tml_status.low_fuse_setting, HEX);
        USE_SERIAL.print("             RC osc: 0x");
        USE_SERIAL.println(tml_status.oscillator_cal, HEX);
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
    } else {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
        USE_SERIAL.printf_P("\n\r *******************************************************************\n\r");
        USE_SERIAL.printf_P(" * Unknown bootloader, application or device at TWI address %02d ... *\n\r", twi_address);
        USE_SERIAL.printf_P(" *******************************************************************\n\n\r");
#else   // -----
        USE_SERIAL.println("\n\r *******************************************************************");
        USE_SERIAL.print(" * Unknown bootloader, application or device at TWI address ");
        USE_SERIAL.println(twi_address);
        USE_SERIAL.println(" *******************************************************************\n");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
    }
}

// Function ThreeStarDelay
void ThreeStarDelay(void) {
    delay(2000);
    for (uint8_t i = 0; i < 3; i++) {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
        USE_SERIAL.printf_P("*");
#else   // -----
        USE_SERIAL.print("*");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
        delay(1000);
    }
}

// Function ShowHeader
void ShowHeader(void) {
    //ClrScr();
    delay(250);
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
    USE_SERIAL.printf_P("\n\r Timonel TWI Bootloader Multi Slave Test (v1.5 twim-ms)\n\r");
#else   // -----
    USE_SERIAL.println("\n\r Timonel TWI Bootloader Multi Slave Test (v1.5 twim-ms)");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
}
