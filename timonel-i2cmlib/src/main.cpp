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

void setup() {
  USE_SERIAL.begin(9600); /* Initialize the serial port for debugging */
  Timonel tml(8, SDA, SCL); /* Create a Timonel instance to communicate with an ATTiny85's bootloader */
  struct Timonel::status tml_status = tml.GetStatus(); /* Get the instance status parameters received from the ATTiny85 */
  //Wire.begin(SDA, SCL);
  //Timonel tml(8);
  if((tml_status.signature == T_SIGNATURE) && ((tml_status.version_major != 0) || (tml_status.version_minor != 0))) {  
    byte version_major = tml_status.version_major;
    USE_SERIAL.printf_P("\n\n\rTimonel v%d.%d", version_major, tml_status.version_minor);
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
    USE_SERIAL.printf_P("........................\n\r");
    USE_SERIAL.printf_P("Features code: %d\n\r", tml_status.features_code);
    USE_SERIAL.printf_P("Bootloader start: 0x%X\n\r", tml_status.bootloader_start);
    USE_SERIAL.printf_P("Trampoline addr: 0x%X\n\r", tml_status.trampoline_addr);
    word app_start = tml_status.application_start;
    if (app_start != 0xFFFF) {
      USE_SERIAL.printf_P("Application start: 0x%X\n\n\r", app_start);
    }
    else {
      USE_SERIAL.printf_P("Application start: (Not Set)\n\n\r");
    }
    delay(5000);
    for (byte i = 1; i < 4; i++) {
      USE_SERIAL.printf_P("*");
      delay(2000);
    }
    if(tml.CheckNewApp() == true) {
      // Upload new firmware version to ATTiny85 ...
      tml.UploadFirmware(payload, sizeof(payload), 0xff);
    }
  }
  else {
    USE_SERIAL.print("\n\n\r[Main] Error: Timonel not contacted ...\n\r");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}
