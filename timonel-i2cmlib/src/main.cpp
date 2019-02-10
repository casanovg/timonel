/*
  main.cpp
  ========
  Test program for Timonel libraries.
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
  // put your setup code here, to run once:
  USE_SERIAL.begin(9600); /* Init the serial port */
  Timonel tml(8, SDA, SCL);
  struct Timonel::status tml_status = tml.GetStatus();
  //Wire.begin(SDA, SCL);
  //Timonel tml(8);
  if (tml.IsTimonelContacted() == true) {
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
    USE_SERIAL.printf_P(".....................\n\r");
    USE_SERIAL.printf_P("Features code: %d\n\r", tml_status.features_code);
    USE_SERIAL.printf_P("Bootloader start: 0x%X\n\r", tml_status.bootloader_start);
    USE_SERIAL.printf_P("Trampoline addr: 0x%X\n\r", tml_status.trampoline_addr);
    word app_start = tml_status.application_start;
    if (app_start != 0xFFFF) {
      USE_SERIAL.printf_P("Application start: 0x%X\n\r", app_start);
    }
    else {
      USE_SERIAL.printf_P("Application start: (Not Set)\n\r");
    }
    delay(5000);
    USE_SERIAL.printf_P("\n\r*");
    delay(2000);
    USE_SERIAL.printf_P("*");
    delay(2000);
    USE_SERIAL.printf_P("*\n\r");
    delay(2000);
    //tml.UploadFirmware(payload, sizeof(payload));
  }
  else {
    USE_SERIAL.print("\n\n\r[Main] Error: Timonel not contacted ...\n\r");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}
