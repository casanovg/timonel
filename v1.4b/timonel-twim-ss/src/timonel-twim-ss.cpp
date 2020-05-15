/*
  timonel-twim-ss.cpp
  ===================
  Timonel library test program (Single Slave) v1.4
  ----------------------------------------------------------------------------
  This demo implements an I2C serial commander to control interactively a
  Tiny85 microcontroller running the Timonel bootloader from an ESP8266
  master. It uses a serial console configured at 9600 N 8 1 for feedback.
  ----------------------------------------------------------------------------
  2020-04-29 Gustavo Casanova
  ----------------------------------------------------------------------------
*/

#include <NbMicro.h>
#include <TimonelTwiM.h>
#include <TwiBus.h>

#include "payload.h"

#define USE_SERIAL Serial
#define SDA 0 /* I2C SDA pin */
#define SCL 2 /* I2C SCL pin */

// Prototypes
void setup(void);
void loop(void);
bool CheckApplUpdate(void);
void ListTwiDevices(byte sda = 0, byte scl = 0);
void PrintStatus(Timonel tml);
void ThreeStarDelay(void);
void ReadChar(void);
word ReadWord(void);
void ShowHeader(void);
void ShowMenu(void);
void ClrScr(void);
void PrintLogo(void);

// Global variables
byte slave_address = 0;
byte block_rx_size = 0;
bool new_key = false;
bool new_byte = false;
bool new_word = false;
bool app_mode = false; /* This holds the slave device running mode info: bootloader or application */
char key = '\0';
word flash_page_addr = 0x0;
word timonel_start = 0xFFFF; /* Timonel start address, 0xFFFF means 'not set' */

Timonel tml; /* From now on, we'll keep a Timonel instance active */

// Setup block
void setup() {
    bool *p_app_mode = &app_mode; /* This is to take different actions depending on whether the bootloader or the application is active */
    USE_SERIAL.begin(9600);       /* Initialize the serial port for debugging */
    //Wire.begin(SDA, SCL);
    ClrScr();
    delay(150);
    PrintLogo();
    TwiBus i2c(SDA, SCL);
    byte slave_address = i2c.ScanBus(p_app_mode);
    tml.SetTwiAddress(slave_address);
    ShowHeader();
    tml.GetStatus();
    PrintStatus(tml);
    ShowMenu();
}

// Main loop
void loop() {
    if (new_key == true) {
        new_key = false;
        USE_SERIAL.printf_P("\n\r");
        switch (key) {
            // *********************************
            // * Test app ||| STDPB1_1 Command *
            // *********************************
            case 'a':
            case 'A': {
                //SetPB1On();
                break;
            }
            // *********************************
            // * Test app ||| STDPB1_0 Command *
            // *********************************
            case 's':
            case 'S': {
                //SetPB1Off();
                break;
            }
            // *********************************
            // * Test app ||| RESETINY Command *
            // *********************************
            case 'x':
            case 'X': {
                //ResetTiny();
                USE_SERIAL.printf_P("\n  .\n\r . .\n\r. . .\n\n\r");
                Wire.begin(SDA, SCL);
                delay(500);
                // #if ESP8266
                //                 ESP.restart();
                // #else
                //                 resetFunc();
                // #endif /* ESP8266 */
                break;
            }
            // ******************
            // * Restart master *
            // ******************
            case 'z':
            case 'Z': {
                USE_SERIAL.printf_P("\nResetting TWI Master ...\n\r\n.\n.\n.\n");
#if ESP8266
                ESP.restart();
#else
                resetFunc();
#endif /* ESP8266 */
                break;
            }
            // ********************************
            // * Timonel ::: GETTMNLV command *
            // ********************************
            case 'v':
            case 'V': {
                USE_SERIAL.printf_P("\nBootloader Cmd >>> Get bootloader version ...\r\n");
                tml.GetStatus();
                PrintStatus(tml);
                break;
            }
            // ********************************
            // * Timonel ::: EXITTMNL command *
            // ********************************
            case 'r':
            case 'R': {
                USE_SERIAL.printf_P("\nBootloader Cmd >>> Run application ...\r\n");
                tml.RunApplication();
                USE_SERIAL.printf_P("\n. . .\n\r . .\n\r  .\n\n\r");
                app_mode = true;
                delay(2000);
#if ESP8266
                ESP.restart();
#else
                resetFunc();
#endif /* ESP8266 */
                break;
            }
            // ********************************
            // * Timonel ::: DELFLASH command *
            // ********************************
            case 'e':
            case 'E': {
                USE_SERIAL.printf_P("\n\rBootloader Cmd >>> Delete app firmware from flash memory, \x1b[5mPLEASE WAIT\x1b[0m ...");
                byte cmd_errors = tml.DeleteApplication();
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                if (cmd_errors == 0) {
                    USE_SERIAL.printf_P(" successful        ");
                } else {
                    USE_SERIAL.printf_P(" [ command error! %d ]", cmd_errors);
                }
                USE_SERIAL.printf_P("\n\n\r");
                break;
            }
            // ********************************
            // * Timonel ::: STPGADDR command *
            // ********************************
            case 'b':
            case 'B': {
                Timonel::Status sts = tml.GetStatus();
                if ((sts.features_code & 0x08) == false) {
                    USE_SERIAL.printf_P("\n\rSet address command not supported by current Timonel features ...\n\r");
                    break;
                }
                USE_SERIAL.printf_P("\n\rPlease enter the flash memory page base address: ");
                while (new_word == false) {
                    flash_page_addr = ReadWord();
                }
                if (sts.bootloader_start > MCU_TOTAL_MEM) {
                    USE_SERIAL.printf_P("\n\n\rWarning: Timonel bootloader start address unknown, please run 'version' command to find it !\n\r");
                    //new_word = false;
                    break;
                }
                if ((flash_page_addr > (timonel_start - 64)) | (flash_page_addr == 0xFFFF)) {
                    USE_SERIAL.printf_P("\n\n\rWarning: The highest flash page address available is %d (0x%X), please correct it !!!", timonel_start - 64, timonel_start - 64);
                    new_word = false;
                    break;
                }
                if (new_word == true) {
                    USE_SERIAL.printf_P("\n\rFlash memory page base address: %d\r\n", flash_page_addr);
                    USE_SERIAL.printf_P("Address high byte: %d (<< 8) + Address low byte: %d\n\r", (flash_page_addr & 0xFF00) >> 8,
                                        flash_page_addr & 0xFF);
                    new_word = false;
                }
                break;
            }
            // ********************************
            // * Timonel ::: WRITPAGE command *
            // ********************************
            case 'w':
            case 'W': {
                USE_SERIAL.printf_P("\n\rBootloader Cmd >>> Upload app firmware to flash memory, \x1b[5mPLEASE WAIT\x1b[0m ...");
                byte cmd_errors = tml.UploadApplication(payload, sizeof(payload), flash_page_addr);
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                if (cmd_errors == 0) {
                    USE_SERIAL.printf_P(" successful        ");
                } else {
                    USE_SERIAL.printf_P(" [ command error! %d ]", cmd_errors);
                }
                USE_SERIAL.printf_P("\n\n\r");
                break;
            }
#if ((defined FEATURES_CODE) && ((FEATURES_CODE >> F_CMD_READFLASH) & true))
            // ********************************
            // * Timonel ::: READFLSH command *
            // ********************************
            case 'm':
            case 'M': {
                // (word) flash_size: MCU flash memory size
                // (byte) slave_data_size: slave-to-master Xmit packet size
                // (byte) values_per_line: MCU memory values shown per line
                tml.DumpMemory(MCU_TOTAL_MEM, SLV_PACKET_SIZE, 32);
                new_byte = false;
                break;
            }
#endif /* CMD_READFLASH) */
            // ******************
            // * ? Help command *
            // ******************
            case '?': {
                USE_SERIAL.printf_P("\n\rHelp ...\n\r========\n\r");
                //ShowHelp();
                break;
            }
            // *******************
            // * Unknown command *
            // *******************
            default: {
                USE_SERIAL.printf_P("Command '%d' unknown ...\n\r", key);
                break;
            }
                USE_SERIAL.printf_P("\n\r");
        }
        ShowMenu();
    }
    ReadChar();
}

// Determine if there is a user application update available
// TODO: Implement an update checking mechanism
bool CheckApplUpdate(void) {
    return true;
}

// Function ReadChar
void ReadChar() {
    if (USE_SERIAL.available() > 0) {
        key = USE_SERIAL.read();
        new_key = true;
    }
}

// Function ReadWord
word ReadWord(void) {
    Timonel::Status sts = tml.GetStatus();
    word last_page = (sts.bootloader_start - SPM_PAGESIZE);
    const byte data_length = 16;
    char serial_data[data_length];  // an array to store the received data
    static byte ix = 0;
    char rc, endMarker = 0xD;  //standard is: char endMarker = '\n'
    while (USE_SERIAL.available() > 0 && new_word == false) {
        rc = USE_SERIAL.read();
        if (rc != endMarker) {
            serial_data[ix] = rc;
            USE_SERIAL.printf_P("%c", serial_data[ix]);
            ix++;
            if (ix >= data_length) {
                ix = data_length - 1;
            }
        } else {
            serial_data[ix] = '\0';  // terminate the string
            ix = 0;
            new_word = true;
        }
    }
    if ((atoi(serial_data) < 0 || atoi(serial_data) > last_page) && new_word == true) {
        for (int i = 0; i < data_length; i++) {
            serial_data[i] = 0;
        }
        USE_SERIAL.printf_P("\n\r");
        USE_SERIAL.printf_P("WARNING! Word memory positions must be between 0 and %d -> Changing to %d", last_page, (word)atoi(serial_data));
    }
    return ((word)atoi(serial_data));
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
    USE_SERIAL.printf_P("\n\r Timonel I2C Bootloader and Application Test (v1.4 twim-ss)\n\r");
}

// Function ShowMenu
void ShowMenu(void) {
    if (app_mode == true) {
        USE_SERIAL.printf_P("Application command ('a', 's', 'z' reboot, 'x' reset MCU, '?' help): ");
    } else {
        Timonel::Status sts = tml.GetStatus();
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

// Function ListTwidevices
void ListTwiDevices(byte sda, byte scl) {
    TwiBus twi(sda, scl);
    TwiBus::DeviceInfo dev_info_arr[(((HIG_TWI_ADDR + 1) - LOW_TWI_ADDR) / 2)];
    twi.ScanBus(dev_info_arr, (((HIG_TWI_ADDR + 1) - LOW_TWI_ADDR) / 2));
    for (byte i = 0; i < (((HIG_TWI_ADDR + 1) - LOW_TWI_ADDR) / 2); i++) {
        USE_SERIAL.printf_P("...........................................................\n\r");
        USE_SERIAL.printf_P("Pos: %02d | ", i + 1);
        if (dev_info_arr[i].firmware != "") {
            USE_SERIAL.printf_P("TWI Addr: %02d | ", dev_info_arr[i].addr);
            USE_SERIAL.printf_P("Firmware: %s | ", dev_info_arr[i].firmware.c_str());
            USE_SERIAL.printf_P("Version %d.%d\n\r", dev_info_arr[i].version_major, dev_info_arr[i].version_minor);
        } else {
            USE_SERIAL.printf_P("No device found\n\r");
        }
        delay(10);
    }
    USE_SERIAL.printf_P("...........................................................\n\n\r");
}
