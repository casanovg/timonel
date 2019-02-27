/*
  main.cpp
  ========
  Timonel library test program
  ----------------------------
  Style: https://google.github.io/styleguide/cppguide.html#Naming
  ---------------------------
  2018-12-13 Gustavo Casanova
  ---------------------------
*/
#include <Arduino.h>
#include <Memory>
#include "TimonelTWIM.h"
#include "payload.h"

#define USE_SERIAL Serial
#define TML_ADDR 8          /* Bootloader I2C address*/
#define APP_ADDR 36         /* Application I2C address*/
#define SDA 0               /* I2C SDA pin */
#define SCL 2               /* SDA SCL pin */

// Prototypes
void setup(void);
void loop(void);
bool CheckApplUpdate(void);
void PrintStatus(Timonel tml);
void ThreeStarDelay(void);
void ReadChar(void);
word ReadWord(void);
void ShowHeader(void);
void ShowMenu(void);
void ClrScr(void);

// Global Variables
byte slave_address = 0;
byte block_rx_size = 0;
bool new_key = false;
bool new_byte = false;
bool new_word = false;
bool app_mode = false;
char key = '\0';
word flash_page_addr = 0x0;
word timonel_start = 0xFFFF;		/* Timonel start address, 0xFFFF means 'not set' */

// Setup block
void setup() {
    USE_SERIAL.begin(9600);         /* Initialize the serial port for debugging */
    Wire.begin(SDA, SCL);
    Timonel tml(TML_ADDR);
    tml.GetStatus();
    ClrScr();
    ShowHeader();
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
            // * Test App ||| STDPB1_1 Command *
            // *********************************
            case 'a': case 'A': {
                //SetPB1On();
                break;
            }
            // *********************************
            // * Test App ||| STDPB1_0 Command *
            // *********************************
            case 's': case 'S': {
                //SetPB1Off();
                break;
            }
            // *********************************
            // * Test App ||| RESETINY Command *
            // *********************************
            case 'x': case 'X': {
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
            case 'z': case 'Z': {
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
            case 'v': case 'V': {
                //USE_SERIAL.printf_P("\nBootloader Cmd >>> Get bootloader version ...\r\n");
                Timonel tml(TML_ADDR);
                tml.GetStatus();
                PrintStatus(tml);
                break;
            }
            // ********************************
            // * Timonel ::: EXITTMNL Command *
            // ********************************
            case 'r': case 'R': {
                //USE_SERIAL.printf_P("\nBootloader Cmd >>> Run Application ...\r\n");
                Timonel tml(TML_ADDR);
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
            case 'e': case 'E': {
                //USE_SERIAL.printf_P("\nBootloader Cmd >>> Delete app firmware from T85 flash memory ...\r\n");
                Timonel tml(TML_ADDR);
                tml.DeleteApplication();
                delay(750);
                tml.GetStatus();
                PrintStatus(tml);
                //TwoStepInit(750);
                break;
            }
            // ********************************
            // * Timonel ::: STPGADDR Command *
            // ********************************
            case 'b': case 'B': {
                //byte resetFirstByte = 0;
                //byte resetSecondByte = 0;
                USE_SERIAL.printf_P("Please enter the flash memory page base address: ");
                while (new_word == false) {
                    flash_page_addr = ReadWord();
                }
                if (timonel_start > MCU_TOTAL_MEM) {
                    USE_SERIAL.printf_P("\n\n\rWarning: Timonel bootloader start address unknown, please run 'version' command to find it !\n\r");
                    //new_word = false;
                    break;
                }
                if ((flash_page_addr > (timonel_start - 64)) | (flash_page_addr == 0xFFFF)) {
                    USE_SERIAL.printf_P("\n\n\rWarning: The highest flash page addreess available is %d (0x%X), please correct it !!!", timonel_start - 64, timonel_start - 64);
                    new_word = false;
                    break;
                }
                if (new_word == true) {
                    USE_SERIAL.printf_P("\r\nFlash memory page base address: %d\r\n", flash_page_addr);
                    USE_SERIAL.printf_P("Address high byte: %d (<< 8) + Address low byte: %d", (flash_page_addr & 0xFF00) >> 8, \
                                                                                                flash_page_addr & 0xFF);
                    new_word = false;
                }
                break;
            }
            // ********************************
            // * Timonel ::: WRITPAGE Command *
            // ********************************
            case 'w': case 'W': {
                Timonel tml(TML_ADDR);
                tml.UploadApplication(payload, sizeof(payload),0x400);
                break;
            }
            // ********************************
            // * Timonel ::: READFLSH Command *
            // ********************************
            case 'm': case 'M': {
                //byte dataSize = 0;	// flash data size requested to ATtiny85
                //byte dataIX = 0;	// Requested flash data start position
                Timonel tml(TML_ADDR);
                tml.DumpMemory(MCU_TOTAL_MEM, RX_DATA_SIZE, VALUES_PER_LINE);            
                //DumpFlashMem(MCUTOTALMEM, 8, 32);
                new_byte = false;
                break;
            }
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

// Function ReadWord
word ReadWord(void) {
	const byte data_length = 16;
	char serial_data[data_length];	// an array to store the received data  
	static byte ix = 0;
	char rc, endMarker = 0xD;		//standard is: char endMarker = '\n'
	while (USE_SERIAL.available() > 0 && new_word == false) {
		rc = USE_SERIAL.read();
		if (rc != endMarker) {
			serial_data[ix] = rc;
			USE_SERIAL.printf_P("%d", serial_data[ix]);
			ix++;
			if (ix >= data_length) {
				ix = data_length - 1;
			}
		}
		else {
			serial_data[ix] = '\0';	// terminate the string
			ix = 0;
			new_word = true;
		}
	}
	if ((atoi(serial_data) < 0 || atoi(serial_data) > MCU_TOTAL_MEM) && new_word == true) {
		for (int i = 0; i < data_length; i++) {
			serial_data[i] = 0;
		}
		USE_SERIAL.printf_P("\n\r");
		USE_SERIAL.printf_P("WARNING! Word memory positions must be between 0 and %d -> Changing to %d\n\r", MCU_TOTAL_MEM, (word)atoi(serial_data));
	}
	return((word)atoi(serial_data));
}

// Function Clear Screen
void ClrScr() {
	USE_SERIAL.write(27);       // ESC command
	USE_SERIAL.printf_P("[2J");    // clear screen command
	USE_SERIAL.write(27);       // ESC command
	USE_SERIAL.printf_P("[H");     // cursor to home command
}

// Print Timonel instance status
void PrintStatus(Timonel timonel) {
    struct Timonel::status tml_status = timonel.GetStatus(); /* Get the instance id parameters received from the ATTiny85 */
    if((tml_status.signature == T_SIGNATURE) && ((tml_status.version_major != 0) || (tml_status.version_minor != 0))) {  
        byte version_major = tml_status.version_major;
        USE_SERIAL.printf_P("\n\r Timonel v%d.%d", version_major, tml_status.version_minor);
        switch (version_major) {
            case 0: {
                USE_SERIAL.printf_P(" Pre-release \n\r");
                break;
            }
            case 1: {
                USE_SERIAL.printf_P(" \"Sandra\" \n\r");
                break;
            }
            default: {
                USE_SERIAL.printf_P(" Unknown \n\r");
                break;
            }
        }
        USE_SERIAL.printf_P(" ====================================\n\r");
        struct Timonel::status tml_status = timonel.GetStatus(); /* Get the instance status parameters received from the ATTiny85 */
        USE_SERIAL.printf_P(" Bootloader address: 0x%X\n\r", tml_status.bootloader_start);
        word app_start = tml_status.application_start;
        if (app_start != 0xFFFF) {
            USE_SERIAL.printf_P("  Application start: 0x%X (0x%X)\n\r", app_start, tml_status.trampoline_addr);
        }
        else {
            USE_SERIAL.printf_P("  Application start: 0x%X (Not Set)\n\r", app_start);
        }
        USE_SERIAL.printf_P("      Features code: %d\n\r", tml_status.features_code);
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
    ClrScr();
    delay(500);
	USE_SERIAL.printf_P("\n\r\rTimonel Bootloader and Application I2C Commander Test (v1.2 i2cmlib)\n\r");
}

// Function ShowMenu
void ShowMenu(void) {
	if (app_mode == true) {
		USE_SERIAL.printf_P("\n\rApplication command ('a', 's', 'z' reboot, 'x' reset T85, '?' help): ");
	}
	else {
		USE_SERIAL.printf_P("\n\rTimonel booloader ('v' version, 'r' run app, 'e' erase flash, 'w' write flash, 'm' mem dump): ");
	}
}


// Setup block
// void setup() {
//     USE_SERIAL.begin(9600);         /* Initialize the serial port for debugging */
//     Wire.begin(SDA, SCL);
//     Timonel tml(8);              /* Create a Timonel instance to communicate with an ATTiny85's bootloader */
//     if(CheckApplUpdate() == true) {
//         delay(2000);
//         for (byte i = 1; i < 4; i++) {
//             USE_SERIAL.printf_P("\n\n\r  %d %d %d %d %d %d %d\r\n ***************\n\r", i, i, i, i, i, i, i);
//             PrintStatus(tml);
//             delay(250);
//             USE_SERIAL.printf_P("\n\n\r[Main] Upload firmware to ATTiny85 ...\n\r");
//             tml.UploadApplication(payload, sizeof(payload));
//             delay(250);
//             PrintStatus(tml);
//             ThreeStarDelay();    
//             USE_SERIAL.printf_P("\n\n\r[Main] Deleting ATTiny85 firmware ...\n\r");
//             tml.DeleteApplication();
//             delay(850);
//             tml.RunApplication();
//             delay(10);
//             PrintStatus(tml);
//             ThreeStarDelay();
//         }
//         USE_SERIAL.printf_P("\n\n\r[Main] Setup routine finished!\n\r");
//     }
//     else {
//         USE_SERIAL.print("\n\n\r[Main] Error: Timonel not contacted ...\n\r");
//     }
// }
