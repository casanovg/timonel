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
#define SDA 0 /* I2C SDA pin */
#define SCL 2 /* SDA SCL pin */

// Prototypes
bool CheckApplUpdate(void);
void PrintStatus(Timonel tml);
void ThreeStarDelay(void);

// Setup block
void setup() {
    USE_SERIAL.begin(9600);   /* Initialize the serial port for debugging */
    Timonel tml(8, SDA, SCL); /* Create a Timonel instance to communicate with an ATTiny85's bootloader */
    if(CheckApplUpdate() == true) {
        delay(2000);
        for (byte i = 1; i < 4; i++) {
            USE_SERIAL.printf_P("\n\n\r  %d %d %d %d %d %d %d\r\n ***************\n\r", i, i, i, i, i, i, i);
            PrintStatus(tml);
            delay(250);
            USE_SERIAL.printf_P("\n\n\r[Main] Upload firmware to ATTiny85 ...\n\r");
            tml.UploadApplication(payload, sizeof(payload));
            delay(250);
            PrintStatus(tml);
            ThreeStarDelay();    
            USE_SERIAL.printf_P("\n\n\r[Main] Deleting ATTiny85 firmware ...\n\r");
            tml.DeleteApplication();
            delay(850);
            tml.RunApplication();
            delay(10);
            PrintStatus(tml);
            ThreeStarDelay();
        }
        USE_SERIAL.printf_P("\n\n\r[Main] Setup routine finished!\n\r");
    }
    else {
        USE_SERIAL.print("\n\n\r[Main] Error: Timonel not contacted ...\n\r");
    }
}

// Main loop
void loop() {
    // put your main code here, to run repeatedly:
}

// Determine if there is a user application update available
bool CheckApplUpdate(void) {
	  return true;
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
