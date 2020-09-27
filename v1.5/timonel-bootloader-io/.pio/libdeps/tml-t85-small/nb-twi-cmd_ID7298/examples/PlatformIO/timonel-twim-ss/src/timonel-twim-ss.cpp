/*
  timonel-twim-ss.cpp
  ===================
  Timonel library test program (Single Slave) v1.5
  ----------------------------------------------------------------------------
  This demo implements an I2C serial commander to control interactively a
  Tiny85 microcontroller running the Timonel bootloader from an ESP8266
  master. It uses a serial console configured at 9600 N 8 1 for feedback.
  ----------------------------------------------------------------------------
  2020-04-29 Gustavo Casanova
  ----------------------------------------------------------------------------
*/

#include "timonel-twim-ss.h"

#include "payload.h"

#define USE_SERIAL Serial
#define SDA 2  // I2C SDA pin - ESP8266 2 - ESP32 21
#define SCL 0  // I2C SCL pin - ESP8266 0 - ESP32 22

// Global variables
bool new_key = false;
bool new_word = false;
bool app_mode = false;  // This holds the slave device running mode info: bootloader or application
char key = '\0';
uint16_t flash_page_addr = 0x0;
uint16_t timonel_start = 0xFFFF;  // Timonel start address, 0xFFFF means 'not set'
Timonel *p_timonel = nullptr;     // Pointer to a bootloader objetct
//NbMicro *p_micro = nullptr;       // Pointer to an application objetct
void (*resetFunc)(void) = 0;

// Setup block
void setup() {
    bool *p_app_mode = &app_mode;  // This is to take different actions depending on whether the bootloader or the application is active
    USE_SERIAL.begin(9600);        // Initialize the serial port for debugging
    ClrScr();
    PrintLogo();
    TwiBus twi_bus(SDA, SCL);
    uint8_t slave_address = 0;
    // Keep waiting until a slave device is detected    
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
    USE_SERIAL.printf_P("\n\rWaiting until a TWI slave device is detected on the bus   ");
#else   // -----
    USE_SERIAL.print("\n\rWaiting until a TWI slave device is detected on the bus   ");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
    while (slave_address == 0) {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
        slave_address = twi_bus.ScanBus(p_app_mode);
        RotaryDelay();
    }
    USE_SERIAL.printf_P("\b\b* ");
    USE_SERIAL.printf_P("\n\r");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
    p_timonel = new Timonel(slave_address, SDA, SCL);
    //p_micro = new NbMicro(44, SDA, SCL);
    ShowHeader();
    p_timonel->GetStatus();
    PrintStatus(*p_timonel);
    ShowMenu();
}

// Main loop
void loop() {
    if (new_key == true) {
        new_key = false;
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
        USE_SERIAL.printf_P("\n\r");
#else   // -----
            USE_SERIAL.println("");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
        switch (key) {
            // *********************************
            // * Test app ||| STDPB1_1 Command *
            // *********************************
            case 'a':
            case 'A': {
                //SetPB1On();
                //byte ret = p_micro->TwiCmdXmit(SETIO1_1, ACKIO1_1);
                byte ret = p_timonel->TwiCmdXmit(SETIO1_1, ACKIO1_1);
                if (ret) {
                    USE_SERIAL.print(" > Error: ");
                    USE_SERIAL.println(ret);

                } else {
                    USE_SERIAL.println(" > OK: ACKIO1_1");
                }
                break;
            }
            // *********************************
            // * Test app ||| STDPB1_0 Command *
            // *********************************
            case 's':
            case 'S': {
                //SetPB1Off();
                byte ret = p_timonel->TwiCmdXmit(SETIO1_0, ACKIO1_0);
                if (ret) {
                    USE_SERIAL.print(" > Error: ");
                    USE_SERIAL.println(ret);

                } else {
                    USE_SERIAL.println(" > OK: ACKIO1_0");
                }
                break;
            }
            // *********************************
            // * Test app ||| RESETINY Command *
            // *********************************
            case 'x':
            case 'X': {
                //ResetTiny();
                byte ret = p_timonel->TwiCmdXmit(RESETMCU, ACKRESET);
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\n  .\n\r . .\n\r. . .\n\n\r");
                //Wire.begin(SDA, SCL);
                if (ret) {
                    USE_SERIAL.print(" > Error: ");
                    USE_SERIAL.println(ret);

                } else {
                    USE_SERIAL.println(" > OK: ACKRESET");
                }
#else   // -----
                    USE_SERIAL.print("\n  .\n\r . .\n\r. . .\n\n\r");
                    //Wire.begin();
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                delay(500);
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                // Wire.begin(SDA, SCL);
                ESP.restart();
#else   // ------
                    resetFunc();
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                break;
            }
            // ******************
            // * Restart master *
            // ******************
            case 'z':
            case 'Z': {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\nResetting TWI Master ...\n\r\n.\n.\n.\n");
                ESP.restart();
#else   // -----
                    USE_SERIAL.print("\nResetting TWI Master ...\n\r\n.\n.\n.\n");
                    resetFunc();
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                break;
            }
            // ********************************
            // * Timonel ::: GETTMNLV command *
            // ********************************
            case 'v':
            case 'V': {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\nBootloader Cmd >>> Get bootloader version ...\r\n");
#else   // -----
                    USE_SERIAL.println("\nBootloader Cmd >>> Get bootloader version ...");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                p_timonel->GetStatus();
                PrintStatus(*p_timonel);
                break;
            }
            // ********************************
            // * Timonel ::: EXITTMNL command *
            // ********************************
            case 'r':
            case 'R': {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\nBootloader Cmd >>> Run application ...\r\n");
                USE_SERIAL.printf_P("\n. . .\n\r . .\n\r  .\n\n\r");
                USE_SERIAL.printf_P("Please wait ...\n\n\r");
#else   // -----
                    USE_SERIAL.println("\nBootloader Cmd >>> Run application ...");
                    USE_SERIAL.println("\n. . .\n\r . .\n\r  .\n");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                uint8_t cmd_errors = p_timonel->RunApplication();
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                if (cmd_errors == 0) {
                    USE_SERIAL.printf_P("Bootloader exit successful, running the user application ...");
                } else {
                    USE_SERIAL.printf_P(" [ command error! %d ]", cmd_errors);
                }
                USE_SERIAL.printf_P("\n\n\r");
#else   // -----
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                app_mode = true;
                delay(500);
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                ESP.restart();
#else   // -----
                    resetFunc();
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                break;
            }
            // ********************************
            // * Timonel ::: DELFLASH command *
            // ********************************
            case 'e':
            case 'E': {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\n\rBootloader Cmd >>> Delete app firmware from flash memory, \x1b[5mPLEASE WAIT\x1b[0m ...");
#else   // -----
                    USE_SERIAL.print("\n\rBootloader Cmd >>> Delete app firmware from flash memory, \x1b[5mPLEASE WAIT\x1b[0m ...");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                uint8_t cmd_errors = p_timonel->DeleteApplication();
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                if (cmd_errors == 0) {
                    USE_SERIAL.printf_P(" successful        ");
                } else {
                    USE_SERIAL.printf_P(" [ command error! %d ]", cmd_errors);
                }
                USE_SERIAL.printf_P("\n\n\r");
#else  // -----
                    USE_SERIAL.print("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                    if (cmd_errors == 0) {
                        USE_SERIAL.print(" successful        ");
                    } else {
                        USE_SERIAL.print(" [ command error! ");
                        USE_SERIAL.print(cmd_errors);
                        USE_SERIAL.print(" ]");
                    }
                    USE_SERIAL.println("\n");

#endif  // #if ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                break;
            }
            // ********************************
            // * Timonel ::: STPGADDR command *
            // ********************************
            case 'b':
            case 'B': {
                Timonel::Status sts = p_timonel->GetStatus();
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
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
#else   // -----
                    if ((sts.features_code & 0x08) == false) {
                        USE_SERIAL.println("\n\rSet address command not supported by current Timonel features ...");
                        break;
                    }
                    USE_SERIAL.print("\n\rPlease enter the flash memory page base address: ");
                    while (new_word == false) {
                        flash_page_addr = ReadWord();
                    }
                    if (sts.bootloader_start > MCU_TOTAL_MEM) {
                        USE_SERIAL.println("\n\n\rWarning: Timonel bootloader start address unknown, please run 'version' command to find it !");
                        break;
                    }
                    if ((flash_page_addr > (timonel_start - 64)) | (flash_page_addr == 0xFFFF)) {
                        USE_SERIAL.print("\n\n\rWarning: The highest flash page address available is ");
                        USE_SERIAL.print(timonel_start - 64);
                        USE_SERIAL.print("(0x");
                        USE_SERIAL.print(timonel_start - 64, HEX);
                        USE_SERIAL.println(" ), please correct it !!!");

                        new_word = false;
                        break;
                    }
                    if (new_word == true) {
                        USE_SERIAL.print("\n\rFlash memory page base address: ");
                        USE_SERIAL.println(flash_page_addr);
                        USE_SERIAL.print("Address high byte: ");
                        USE_SERIAL.print((flash_page_addr & 0xFF00) >> 8);
                        USE_SERIAL.print(" (<< 8) + Address low byte: ");
                        USE_SERIAL.println(flash_page_addr & 0xFF);
                        new_word = false;
                    }
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                break;
            }
            // ********************************
            // * Timonel ::: WRITPAGE command *
            // ********************************
            case 'w':
            case 'W': {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\n\rBootloader Cmd >>> Firmware upload to flash memory, \x1b[5mPLEASE WAIT\x1b[0m ...");
                uint8_t cmd_errors = p_timonel->UploadApplication(payload, sizeof(payload), flash_page_addr);
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                if (cmd_errors == 0) {
                    USE_SERIAL.printf_P(" successful, press 'r' to run the user app");
                } else {
                    USE_SERIAL.printf_P(" [ command error! %d ]", cmd_errors);
                }
                USE_SERIAL.printf_P("\n\n\r");
#else   // -----
                    USE_SERIAL.print("\n\rBootloader Cmd >>> Upload app firmware to flash memory, \x1b[5mPLEASE WAIT\x1b[0m ...");
                    uint8_t cmd_errors = p_timonel->UploadApplication(payload, sizeof(payload), flash_page_addr);
                    USE_SERIAL.print("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                    if (cmd_errors == 0) {
                        USE_SERIAL.print(" successful, press 'r' to run the user app");
                    } else {
                        USE_SERIAL.print(" [ command error! ");
                        USE_SERIAL.print(cmd_errors);
                        USE_SERIAL.print(" ]");
                    }
                    USE_SERIAL.println("\n");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                break;
            }
#if ((defined FEATURES_CODE) && ((FEATURES_CODE >> F_CMD_READFLASH) & true))
            // ********************************
            // * Timonel ::: READFLSH command *
            // ********************************
            case 'm':
            case 'M': {
                // (uint16_t) flash_size: MCU flash memory size
                // (uint8_t) slave_data_size: slave-to-master Xmit packet size
                // (uint8_t) values_per_line: MCU memory values shown per line
                p_timonel->DumpMemory(MCU_TOTAL_MEM, SLV_PACKET_SIZE, 32);
                break;
            }
#endif /* CMD_READFLASH) */
            // ******************
            // * ? Help command *
            // ******************
            case '?': {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\n\rHelp ...\n\r========\n\r");
#else   // -----
                    USE_SERIAL.println("\b\rHelp ...\n\r========");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
            //ShowHelp();
                break;
            }
            // *******************
            // * Unknown command *
            // *******************
            default: {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("Command '%d' unknown ...\n\r", key);
#else   // -----
                    USE_SERIAL.print("Unknown command: ");
                    USE_SERIAL.println(key);
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
                break;
            }
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
                USE_SERIAL.printf_P("\n\r");
#else   // -----
                    USE_SERIAL.println("");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
        }
        ShowMenu();
    }
    ReadChar();
}

// Function ReadChar
void ReadChar() {
    if (USE_SERIAL.available() > 0) {
        key = USE_SERIAL.read();
        new_key = true;
    }
}

// Function ReadWord
uint16_t ReadWord(void) {
    Timonel::Status sts = p_timonel->GetStatus();
    uint16_t last_page = (sts.bootloader_start - SPM_PAGESIZE);
    const uint8_t data_length = 16;
    char serial_data[data_length];  // an array to store the received data
    static uint8_t ix = 0;
    char rc, endMarker = 0xD;  //standard is: char endMarker = '\n'
    while (USE_SERIAL.available() > 0 && new_word == false) {
        rc = USE_SERIAL.read();
        if (rc != endMarker) {
            serial_data[ix] = rc;
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
            USE_SERIAL.printf_P("%c", serial_data[ix]);
#else   // -----
                USE_SERIAL.print(serial_data[ix]);
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
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
    if ((atoi(serial_data) < 0 || atoi(serial_data) > (int)last_page) && new_word == true) {
        for (int i = 0; i < data_length; i++) {
            serial_data[i] = 0;
        }
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
        USE_SERIAL.printf_P("\n\r");
        USE_SERIAL.printf_P("WARNING! uint16_t memory positions must be between 0 and %d -> Changing to %d", last_page, (uint16_t)atoi(serial_data));
#else   // -----
            USE_SERIAL.println("");
            USE_SERIAL.print("WARNING! uint16_t memory positions must be between 0 and ");
            USE_SERIAL.print(last_page);
            USE_SERIAL.print(" -> Changing to ");
            USE_SERIAL.println((uint16_t)atoi(serial_data));
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
    }
    return ((uint16_t)atoi(serial_data));
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
        USE_SERIAL.printf_P("\n\r *************************************************\n\r");
        USE_SERIAL.printf_P(" * User application running on TWI device %02d ... *\n\r", twi_address);
        USE_SERIAL.printf_P(" *************************************************\n\n\r");
#else   // -----
            USE_SERIAL.println("\n\r *******************************************************************");
            USE_SERIAL.print(" * Unknown bootloader, application or device at TWI address ");
            USE_SERIAL.println(twi_address);
            USE_SERIAL.println(" *******************************************************************\n");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
    }
}

// Function ShowHeader
void ShowHeader(void) {
    //ClrScr();
    delay(250);
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
    USE_SERIAL.printf_P("\n\r Timonel I2C Bootloader and Application Test (v1.5 twim-ss)\n\r");
#else   // -----
        USE_SERIAL.println("\n\r Timonel I2C Bootloader and Application Test (v1.5 twim-ss)");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
}

// Function RotaryDelay
void RotaryDelay(void) {
    USE_SERIAL.printf_P("\b\b| ");
    delay(125);
    USE_SERIAL.printf_P("\b\b/ ");
    delay(125);
    USE_SERIAL.printf_P("\b\b- ");
    delay(125);
    USE_SERIAL.printf_P("\b\b\\ ");
    delay(125);
}

// Function ShowMenu
void ShowMenu(void) {
    if (app_mode == true) {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
        USE_SERIAL.printf_P("Application command ('a', 's', 'z' reboot, 'x' reset MCU, '?' help): ");
#else   // -----
            USE_SERIAL.print("Application command ('a', 's', 'z' reboot, 'x' reset MCU, '?' help): ");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
    } else {
        Timonel::Status sts = p_timonel->GetStatus();
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
        USE_SERIAL.printf_P("Timonel bootloader ('v' version, 'r' run app, 'e' erase flash, 'w' write flash");
        if ((sts.features_code & 0x08) == 0x08) {
            USE_SERIAL.printf_P(", 'b' set addr");
        }
        if ((sts.features_code & 0x80) == 0x80) {
            USE_SERIAL.printf_P(", 'm' mem dump");
        }
        USE_SERIAL.printf_P("): ");
#else   // -----
            USE_SERIAL.print("Timonel bootloader ('v' version, 'r' run app, 'e' erase flash, 'w' write flash");
            if ((sts.features_code & 0x08) == 0x08) {
                USE_SERIAL.print(", 'b' set addr");
            }
            if ((sts.features_code & 0x80) == 0x80) {
                USE_SERIAL.print(", 'm' mem dump");
            }
            USE_SERIAL.print("): ");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
    }
}

// Function ListTwidevices
void ListTwiDevices(uint8_t sda, uint8_t scl) {
    TwiBus twi(sda, scl);
    TwiBus::DeviceInfo dev_info_arr[(((HIG_TWI_ADDR + 1) - LOW_TWI_ADDR) / 2)];
    twi.ScanBus(dev_info_arr, (((HIG_TWI_ADDR + 1) - LOW_TWI_ADDR) / 2));
    for (uint8_t i = 0; i < (((HIG_TWI_ADDR + 1) - LOW_TWI_ADDR) / 2); i++) {
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
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
#else   // -----
            USE_SERIAL.println("...........................................................");
            USE_SERIAL.print("Pos: ");
            USE_SERIAL.print(i + 1);
            USE_SERIAL.print(" | ");
            if (dev_info_arr[i].firmware != "") {
                USE_SERIAL.print("TWI Addr: ");
                USE_SERIAL.print(dev_info_arr[i].addr);
                USE_SERIAL.print(" | ");
                USE_SERIAL.print("Firmware: ");
                USE_SERIAL.print(dev_info_arr[i].firmware.c_str());
                USE_SERIAL.print(" | ");
                USE_SERIAL.print("Version ");
                USE_SERIAL.print(dev_info_arr[i].version_major);
                USE_SERIAL.print(".");
                USE_SERIAL.print(dev_info_arr[i].version_minor);
            } else {
                USE_SERIAL.println("No device found");
            }
            delay(10);
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
    }
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
    USE_SERIAL.printf_P("...........................................................\n\n\r");
#else   // -----
        USE_SERIAL.println("...........................................................\n");
#endif  // ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM
}
