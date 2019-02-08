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
  //Wire.begin(SDA, SCL);
  //Timonel tml(8);
  if (tml.IsTimonelContacted() == true) {
    USE_SERIAL.printf_P("\n\n\rTimonel v%d.%d\n\r", tml.GetVersionMaj(), tml.GetVersionMin());
    USE_SERIAL.printf_P("Features code: %d\n\r", tml.GetFeatures());
    USE_SERIAL.printf_P("Timonel start: 0x%X\n\r", tml.GetTmlStart());
    USE_SERIAL.printf_P("Trampoline addr: 0x%X\n\r", tml.GetTrampoline());
  }
  else {
    USE_SERIAL.print("\n\n\r[Main] Error: Timonel not contacted ...\n\r");
  }
  USE_SERIAL.printf_P("Cotza: 0x%X\n\r", payload[44]);
  //tml.UploadFirmware(payload[]);
}

void loop() {
  // put your main code here, to run repeatedly:
}
