/*
  Timonel-TwiM-MS.ino
  ===================
  Timonel libraries test program (Multi Slave) v1.4
  ----------------------------------------------------------------------------
  This demo shows how to control and update several Tiny85 microcontrollers
  running the Timonel bootloader from an ESP8266 master.
  It uses a serial console configured at 9600 N 8 1 for feedback.
  ----------------------------------------------------------------------------
  2019-04-29 Gustavo Casanova
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

#include <NbMicro.h>
#include <TimonelTwiM.h>
#include <TwiBus.h>

#include "payload.h"

#define USE_SERIAL Serial
#define SDA 0 /* I2C SDA pin */
#define SCL 2 /* I2C SCL pin */
#define MAX_TWI_DEVS 28
#define LOOP_COUNT 3
#define T_SIGNATURE 84

// Prototypes
bool CheckApplUpdate(void);
void PrintStatus(Timonel timonel);
void ThreeStarDelay(void);
void ShowHeader(void);
void ShowMenu(void);
void PrintLogo(void);
void ClrScr(void);

// Global Variables
byte slave_address = 0;
byte block_rx_size = 0;
bool app_mode = false;
byte timonels = 0;
byte applications = 0;

void setup() {
    // Initialize the serial port for debugging
    USE_SERIAL.begin(9600);
    Wire.begin(SDA, SCL);
    ClrScr();
    PrintLogo();
    ShowHeader();

    // Routine loop
    for (byte loop = 0; loop < LOOP_COUNT; loop++) {
        USE_SERIAL.printf_P("\n\rPASS %d OF %d ...\n\r", loop + 1, LOOP_COUNT);
        // The bus device scanning it has to be made as fast as possible since each
        // discovered Timonel has to be initialized before launching the user apps
        TwiBus twi(SDA, SCL);
        TwiBus::DeviceInfo dev_info_arr[HIG_TWI_ADDR - LOW_TWI_ADDR + 1];
        // Scanning the TWI bus in search of devices ...
        byte tml_count = 0;
        USE_SERIAL.printf_P("\n\r");
        while (tml_count == 0) {
            USE_SERIAL.printf_P("\r\x1b[5mScanning TWI bus ...\x1b[0m");
            twi.ScanBus(dev_info_arr, HIG_TWI_ADDR - LOW_TWI_ADDR + 1, LOW_TWI_ADDR);
            byte arr_size = (sizeof(dev_info_arr) / sizeof(dev_info_arr[0]));
            for (byte i = 0; i < arr_size; i++) {
                if (dev_info_arr[i].firmware == "Timonel") {
                    tml_count++;
                }
            }
            if (tml_count > 0) {
                USE_SERIAL.printf_P("\rTimonel devices found: %d\n\r", tml_count);
            }
            delay(1000);
        }
        Timonel *tml_pool[tml_count];
        // Create and initialize bootloader objects found
        for (byte i = 0; i <= (tml_count); i++) {
            if (dev_info_arr[i].firmware == "Timonel") {
                tml_pool[i] = new Timonel(dev_info_arr[i].addr);
                USE_SERIAL.printf_P("\n\rGetting status of Timonel device %d\n\r", dev_info_arr[i].addr);
                Timonel::Status sts = tml_pool[i]->GetStatus();
                if ((sts.features_code >> F_USE_WDT_RESET) & true) {
                    USE_SERIAL.printf_P("\n\r ***************************************************************************************\n\r");
                    USE_SERIAL.printf_P(" * WARNING! The Timonel bootloader with TWI address %02d has the \"TIMEOUT_EXIT\" feature. *\n\r", dev_info_arr[i].addr);
                    USE_SERIAL.printf_P(" * enabled. This TWI master firmware can't control it properly! Please recompile it    *\n\r");
                    USE_SERIAL.printf_P(" * using a configuration with that option disabled (e.g. \"tml-t85-small\").             *\n\r");
                    USE_SERIAL.printf_P(" ***************************************************************************************\n\r");
                }
            }
        }
        USE_SERIAL.printf_P("\n\r");
        ThreeStarDelay();
        USE_SERIAL.printf_P("\n\n\r");
        // Delete user applications from devices
        for (byte i = 0; i <= (tml_count); i++) {
            if (dev_info_arr[i].firmware == "Timonel") {
                delay(10);
                USE_SERIAL.printf_P("Deleting application on device %d ", dev_info_arr[i].addr);
                byte errors = tml_pool[i]->DeleteApplication();
                if (errors == 0) {
                    USE_SERIAL.printf_P("OK!\n\r");
                } else {
                    USE_SERIAL.printf_P("Error! (%d)\n\r", errors);
                }
                delay(1000);
                USE_SERIAL.printf_P("\n\rGetting status of device %d\n\r", dev_info_arr[i].addr);
                tml_pool[i]->GetStatus();
                PrintStatus(*tml_pool[i]);
            }
        }
        ThreeStarDelay();
        USE_SERIAL.printf_P("\n\r");
        // Upload user applications to devices and run them
        for (byte i = 0; i <= (tml_count); i++) {
            if (dev_info_arr[i].firmware == "Timonel") {
                USE_SERIAL.printf_P("\n\rUploading application to device %d, \x1b[5mPLEASE WAIT\x1b[0m ...", dev_info_arr[i].addr);
                byte errors = tml_pool[i]->UploadApplication(payload, sizeof(payload));
                if (errors == 0) {
                    USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b successful!      \n\r");
                } else {
                    USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b error! (%d)           \n\r", errors);
                }
                delay(10);
                USE_SERIAL.printf_P("\n\rGetting status of device %d\n\r", dev_info_arr[i].addr);
                PrintStatus(*tml_pool[i]);
                delay(10);
                USE_SERIAL.printf_P("Running application on device %d\n\r", dev_info_arr[i].addr);
                tml_pool[i]->RunApplication();
                delay(1500);
            }
        }
        // Reset applications and prepare for another cycle, then clean objects
        if (loop < LOOP_COUNT - 1) {
            USE_SERIAL.printf_P("\n\rLetting application run 28 seconds before resetting and starting next cycle   ");
            byte dly = 28;
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
            // Resetting devices
            // NOTE: All devices share the same application, since the application TWI address is
            // set at compile time, this is shared across all devices when the app is running.
            // Once discovered, the app TWI address is used to send the reset command to all devices.
            byte app_addr = twi.ScanBus();
            NbMicro *micro = new NbMicro;
            micro->SetTwiAddress(app_addr); /* NOTE: All devices share the same TWI application address (44) */
            USE_SERIAL.printf_P("Resetting devices running application at address %d\n\r", micro->GetTwiAddress());
            micro->TwiCmdXmit(RESETMCU, ACKRESET);
            delay(1000);
            delete micro;
            Wire.begin(SDA, SCL);
        } else {
            USE_SERIAL.printf_P("\n\rCycle completed %d of %d passes! Letting application run ...\n\n\r", LOOP_COUNT, LOOP_COUNT);
        }
        for (byte i = 0; i < tml_count; i++) {
            delete tml_pool[i];
        }
    }
}

void loop() {
  // Nothing
}

// Determine if there is a user application update available
bool CheckApplUpdate(void) {
    return false;
}

// Function clear screen
void ClrScr() {
    USE_SERIAL.write(27);        // ESC command
    USE_SERIAL.printf_P("[2J");  // clear screen command
    USE_SERIAL.write(27);        // ESC command
    USE_SERIAL.printf_P("[H");   // cursor to home command
}

// Function PrintLogo
void PrintLogo(void) {
    USE_SERIAL.printf_P("        _                         _\n\r");
    USE_SERIAL.printf_P("    _  (_)                       | |\n\r");
    USE_SERIAL.printf_P("  _| |_ _ ____   ___  ____  _____| |\n\r");
    USE_SERIAL.printf_P(" (_   _) |    \\ / _ \\|  _ \\| ___ | |\n\r");
    USE_SERIAL.printf_P("   | |_| | | | | |_| | | | | ____| |\n\r");
    USE_SERIAL.printf_P("    \\__)_|_|_|_|\\___/|_| |_|_____)\\_)\n\r");
}

// Function print Timonel instance status
void PrintStatus(Timonel timonel) {
    Timonel::Status tml_status = timonel.GetStatus(); /* Get the instance id parameters received from the ATTiny85 */
    byte twi_address = timonel.GetTwiAddress();
    byte version_major = tml_status.version_major;
    byte version_minor = tml_status.version_minor;
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
        USE_SERIAL.printf_P("\n\r Timonel v%d.%d %s ", version_major, version_minor, version_mj_nick.c_str());
        USE_SERIAL.printf_P("(TWI: %02d)\n\r", twi_address);
        USE_SERIAL.printf_P(" ====================================\n\r");
        USE_SERIAL.printf_P(" Bootloader address: 0x%X\n\r", tml_status.bootloader_start);
        word app_start = tml_status.application_start;
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
    } else {
        USE_SERIAL.printf_P("\n\r *******************************************************************\n\r");
        USE_SERIAL.printf_P(" * Unknown bootloader, application or device at TWI address %02d ... *\n\r", twi_address);
        USE_SERIAL.printf_P(" *******************************************************************\n\n\r");
    }
}

// Function ThreeStarDelay
void ThreeStarDelay(void) {
    delay(2000);
    for (byte i = 0; i < 3; i++) {
        USE_SERIAL.printf_P("*");
        delay(1000);
    }
}

// Function ShowHeader
void ShowHeader(void) {
    //ClrScr();
    delay(250);
    USE_SERIAL.printf_P("\n\rTimonel TWI Bootloader and Application Test (v1.4 twim-ms)\n\r");
}

// Function ShowMenu
void ShowMenu(void) {
    if (app_mode == true) {
        USE_SERIAL.printf_P("Application command ('a', 's', 'z' reboot, 'x' reset MCU, '?' help): ");
    } else {
        Timonel::Status sts /*= tml.GetStatus()*/;
        USE_SERIAL.printf_P("Timonel bootloader ('v' version, 'r' run app, 'e' erase flash, 'w' write flash");
        if ((sts.features_code & 0x08) == 0x08) {
            USE_SERIAL.printf_P(", 'b' set addr");
        }
        if ((sts.features_code & 0x80) == 0x80) {
            USE_SERIAL.printf_P(", 'm' mem dump");
        }
        USE_SERIAL.printf_P("): ");
    }
}
