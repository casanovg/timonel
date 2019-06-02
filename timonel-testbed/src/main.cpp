/*
  main.cpp (timonel-testbed)
  ==========================
  Timonel bootloader testbed
  ----------------------------
  2019-03-22 Gustavo Casanova
  ---------------------------
*/
#include <Arduino.h>
//#include "Wire.h"
#include "TimonelTwiM.h"
#include "payload.h"

#define USE_SERIAL Serial
#define TML_ADDR 8  /* Bootloader I2C address*/
#define APP_ADDR 36 /* Application I2C address*/
#define SDA 0       /* I2C SDA pin */
#define SCL 2       /* I2C SCL pin */

// Prototypes
void setup(void);
void loop(void);
bool CheckApplUpdate(void);
void ListTwiDevices(byte sda = 0, byte scl = 0);
byte GetAllTimonels(Timonel tml_arr[], byte tml_arr_size, byte sda = 0, byte scl = 0);
void PrintStatus(Timonel tml);
void ThreeStarDelay(void);
void ReadChar(void);
byte ReadByte(void);
word ReadWord(void);
void ShowHeader(void);
void ShowMenu(void);
void ClrScr(void);

// Global variables
byte slave_address = 0;
byte block_rx_size = 0;
bool new_key = false;
bool new_byte = false;
bool new_word = false;
bool app_mode = true;
char key = '\0';
word flash_page_addr = 0x0;
word timonel_start = 0xFFFF; /* Timonel start address, 0xFFFF means 'not set' */
byte twi_address = 0;

// Setup block
void setup() {
    USE_SERIAL.begin(9600); /* Initialize the serial port for debugging */
    ClrScr();
    ShowHeader();
    ShowMenu();
}

// Main loop
void loop() {
    if (new_key == true) {
        new_key = false;
        USE_SERIAL.printf_P("\n\r");
        switch (key) {
            // **********************************
            // * Testbed ||| Wire begin Command *
            // **********************************
            case '1': {
                //SetPB1On();
                Wire.begin(SDA, SCL);
                USE_SERIAL.printf_P("\n\rTWI bus successfully initialized!\n\n\r");
                break;
            }
            // *********************************
            // * Testbed ||| Initialize Timonel*
            // *********************************
            case '2': {
                //USE_SERIAL.printf_P("\n\rPlease enter the Timonel TWI address (8 to 35): ");
                USE_SERIAL.printf_P("\n\rPlease enter the Timonel TWI address (0 to 127): ");
                while (new_byte == false) {
                    twi_address = ReadByte();                    
                }
                //if ((twi_address >= 8) && (twi_address <= 35)) {
                if ((twi_address >= 0) && (twi_address <= 127)) { /* This version allows I2C general calls */
					USE_SERIAL.printf_P("\n\n\rTWI Address set to: %d, please press 3 to initialize Timonel ...\n\n\r", twi_address);
				}
                else {
                    USE_SERIAL.printf_P("\n\n\rInvalid address!\n\n\r");
                }
                new_byte = false;
                break;
            }
            case '3': {
				USE_SERIAL.printf_P("\n\rInitializing Timonel with address: %d\n\n\r", twi_address);
                Timonel tml(twi_address);
                PrintStatus(tml);
                break;
            }
#if ((defined FEATURES_CODE) && ((FEATURES_CODE >> F_CMD_READFLASH) & true))
            case '4': {
                Timonel tml(twi_address);
                tml.DumpMemory(MCU_TOTAL_MEM, SLV_PACKET_SIZE, VALUES_PER_LINE);
                //tml.DumpMemory(MCU_TOTAL_MEM, 32, VALUES_PER_LINE);
                new_byte = false;
                break;
            }
#endif /* FEATURES_CODE >> F_CMD_READFLASH */
            // *********************************
            // * Test App ||| RESETINY Command *
            // *********************************
            case 'x':
            case 'X': {
                //ResetTiny();
                USE_SERIAL.printf_P("\n  .\n\r . .\n\r. . .\n\n\r");
                delay(2000);
#if ESP8266
                ESP.restart();
#else
                resetFunc();
#endif /* ESP8266 */
                break;
            }
            // ******************
            // * Restart Master *
            // ******************
            case 'z':
            case 'Z': {
                USE_SERIAL.printf_P("\nResetting ESP8266 ...\n\r\n.\n.\n.\n");
#if ESP8266
                ESP.restart();
#else
                resetFunc();
#endif /* ESP8266 */
                break;
            }
            // ********************************
            // * Timonel ::: GETTMNLV Command *
            // ********************************
            case 'v':
            case 'V': {
                //USE_SERIAL.printf_P("\nBootloader Cmd >>> Get bootloader version ...\r\n");
                //tml.GetStatus();
                //PrintStatus(tml);
                break;
            }
            // ********************************
            // * Timonel ::: EXITTMNL Command *
            // ********************************
            case '7': {
                //USE_SERIAL.printf_P("\nBootloader Cmd >>> Run Application ...\r\n");
                Timonel tml(twi_address);
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
            // * Timonel ::: DELFLASH Command *
            // ********************************
            case '6': {
                //USE_SERIAL.printf_P("\nBootloader Cmd >>> Delete app firmware from T85 flash memory ...\r\n");
                Timonel tml(twi_address);
                tml.DeleteApplication();
                delay(750);
                //tml.GetStatus();
                //PrintStatus(tml);
                //TwoStepInit(750);
                break;
            }
            // ********************************
            // * Timonel ::: STPGADDR Command *
            // ********************************
            // case 'b':
            // case 'B': {
            //     //byte resetFirstByte = 0;
            //     //byte resetSecondByte = 0;
            //     Timonel::Status sts = tml.GetStatus();
            //     if ((sts.features_code & 0x08) == false) {
            //         USE_SERIAL.printf_P("\n\rSet address command not supported by current Timonel features ...\n\r");
            //         break;
            //     }
            //     USE_SERIAL.printf_P("\n\rPlease enter the flash memory page base address: ");
            //     while (new_word == false) {
            //         flash_page_addr = ReadWord();
            //     }
            //     if (sts.bootloader_start > MCU_TOTAL_MEM) {
            //         USE_SERIAL.printf_P("\n\n\rWarning: Timonel bootloader start address unknown, please run 'version' command to find it !\n\r");
            //         //new_word = false;
            //         break;
            //     }
            //     if ((flash_page_addr > (timonel_start - 64)) | (flash_page_addr == 0xFFFF)) {
            //         USE_SERIAL.printf_P("\n\n\rWarning: The highest flash page addreess available is %d (0x%X), please correct it !!!", timonel_start - 64, timonel_start - 64);
            //         new_word = false;
            //         break;
            //     }
            //     if (new_word == true) {
            //         USE_SERIAL.printf_P("\r\nFlash memory page base address: %d\r\n", flash_page_addr);
            //         USE_SERIAL.printf_P("Address high byte: %d (<< 8) + Address low byte: %d\n\r", (flash_page_addr & 0xFF00) >> 8,
            //                             flash_page_addr & 0xFF);
            //         new_word = false;
            //     }
            //     break;
            // }
            // ********************************
            // * Timonel ::: WRITPAGE Command *
            // ********************************
            case '5': {
                Timonel tml(twi_address);
                tml.UploadApplication(payload, sizeof(payload), flash_page_addr);
                break;
            }
            // // ********************************
            // // * Timonel ::: READFLSH Command *
            // // ********************************
            // case 'm':
            // case 'M': {
            //     //byte dataSize = 0;    // flash data size requested to ATtiny85
            //     //byte dataIX = 0;  // Requested flash data start position
            //     tml.DumpMemory(MCU_TOTAL_MEM, RX_DATA_SIZE, VALUES_PER_LINE);
            //     //DumpFlashMem(MCUTOTALMEM, 8, 32);
            //     new_byte = false;
            //     break;
            // }
            // ******************
            // * ? Help Command *
            // ******************
            case '?': {
                USE_SERIAL.printf_P("\n\rHelp ...\n\r========\n\r");
                //ShowHelp();
                break;
            }
            // *******************
            // * Unknown Command *
            // *******************
            default: {
                USE_SERIAL.printf_P("[Main] Command '%d' unknown ...\n\r", key);
                break;
            }
                USE_SERIAL.printf_P("\n\r");
                USE_SERIAL.printf_P("Menusero\n\r");
        }
        ShowMenu();
    }
    ReadChar();
}

// Determine if there is a user application update available
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

// Function ReadByte
byte ReadByte(void) {
	const byte dataLength = 16;
	char serialData[dataLength];	// an array to store the received data  
	static byte ix = 0;
	char rc, endMarker = 0xD;		//standard is: char endMarker = '\n'
	while (Serial.available() > 0 && new_byte == false) {
		rc = Serial.read();
		if (rc != endMarker) {
			serialData[ix] = rc;
			Serial.print(serialData[ix]);
			ix++;
			if (ix >= dataLength) {
				ix = dataLength - 1;
			}
		}
		else {
			serialData[ix] = '\0';	// terminate the string
			ix = 0;
			new_byte = true;
		}
	}
	if ((atoi(serialData) < 0 || atoi(serialData) > 255) && new_byte == true) {
		Serial.println("");
		Serial.print("WARNING! Byte values must be 0 to 255 -> Truncating to ");
		Serial.println((byte)atoi(serialData));
	}
	return((byte)atoi(serialData));
}

// Function ReadWord
// word ReadWord(void) {
//     //Timonel::Status sts = tml.GetStatus();
//     word last_page = (sts.bootloader_start - PAGE_SIZE);
//     const byte data_length = 16;
//     char serial_data[data_length];  // an array to store the received data
//     static byte ix = 0;
//     char rc, endMarker = 0xD;  //standard is: char endMarker = '\n'
//     while (USE_SERIAL.available() > 0 && new_word == false) {
//         rc = USE_SERIAL.read();
//         if (rc != endMarker) {
//             serial_data[ix] = rc;
//             USE_SERIAL.printf_P("%c", serial_data[ix]);
//             ix++;
//             if (ix >= data_length) {
//                 ix = data_length - 1;
//             }
//         } else {
//             serial_data[ix] = '\0';  // terminate the string
//             ix = 0;
//             new_word = true;
//         }
//     }
//     if ((atoi(serial_data) < 0 || atoi(serial_data) > last_page) && new_word == true) {
//         for (int i = 0; i < data_length; i++) {
//             serial_data[i] = 0;
//         }
//         USE_SERIAL.printf_P("\n\r");
//         USE_SERIAL.printf_P("WARNING! Word memory positions must be between 0 and %d -> Changing to %d", last_page, (word)atoi(serial_data));
//     }
//     return ((word)atoi(serial_data));
// }

// Function Clear Screen
void ClrScr() {
    USE_SERIAL.write(27);        // ESC command
    USE_SERIAL.printf_P("[2J");  // clear screen command
    USE_SERIAL.write(27);        // ESC command
    USE_SERIAL.printf_P("[H");   // cursor to home command
}

// Function Print Timonel instance status
void PrintStatus(Timonel timonel) {
    Timonel::Status tml_status = timonel.GetStatus(); /* Get the instance id parameters received from the ATTiny85 */
    byte twi_address = timonel.GetTwiAddress();
    byte version_major = tml_status.version_major;
    byte version_minor = tml_status.version_minor;
    if (((tml_status.signature == T_SIGNATURE_CTM) || (tml_status.signature == T_SIGNATURE_AUT)) && \
       ((version_major != 0) || (version_minor != 0))) {
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
        USE_SERIAL.printf_P("      Features code: %d ", tml_status.features_code);
        if (tml_status.auto_clock_tweak == true) {
            USE_SERIAL.printf_P("(Auto-Twk)");
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
    USE_SERIAL.printf_P("\n\r[ Timonel Bootloader Testbed (v1.1 \"MA'82\") ]");
    USE_SERIAL.printf_P("\n\r[ 2019-04-02 Gustavo Casanova ............. ]\n\n\r");
}

// Function ShowMenu
void ShowMenu(void) {
    if (app_mode == true) {
        USE_SERIAL.printf_P("Test command ('1' wire begin, '2' set addr, '3' init Timonel, '4' dump mem, '5' upload, 6 'delete', 7 'run'): ");
    } 
    // else {
    //     Timonel::Status sts = tml.GetStatus();
    //     USE_SERIAL.printf_P("Timonel booloader ('v' version, 'r' run app, 'e' erase flash, 'w' write flash");
    //     if ((sts.features_code & 0x08) == 0x08) {
    //         USE_SERIAL.printf_P(", 'b' set addr");
    //     }
    //     if ((sts.features_code & 0x80) == 0x80) {
    //         USE_SERIAL.printf_P(", 'm' mem dump");
    //     }
    //     USE_SERIAL.printf_P("): ");
    // }
}

// Function ListTwidevices
void ListTwiDevices(byte sda, byte scl) {
    TwiBus twi(sda, scl);
    TwiBus::DeviceInfo dev_info_arr[HIG_TWI_ADDR];
    twi.ScanBus(dev_info_arr, HIG_TWI_ADDR);
    for (byte i = 0; i < HIG_TWI_ADDR; i++) {
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

//Function GetALlTimonels
byte GetAllTimonels(Timonel tml_arr[], byte tml_arr_size, byte sda, byte scl) {
    byte timonels = 0;
    TwiBus twi(sda, scl);
    TwiBus::DeviceInfo dev_info_arr[HIG_TWI_ADDR];
    twi.ScanBus(dev_info_arr, HIG_TWI_ADDR);
    for (byte i = 0; i < HIG_TWI_ADDR; i++) {
        if ((dev_info_arr[i].firmware != "Timonel") && (timonels < tml_arr_size)) {
            USE_SERIAL.printf_P("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\r");
            USE_SERIAL.printf_P("Pos: %02d | ", timonels);
            byte tml_addr = dev_info_arr[i].addr;
            USE_SERIAL.printf_P("Timonel found at address: %02d, creating object ...\n\r", tml_addr);
            tml_arr[timonels].SetTwiAddress(tml_addr);
            timonels++;
        }
        delay(10);
    }
    USE_SERIAL.printf_P("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\r");
    //Timonel tml_array[timonels];
    return 0;
}

