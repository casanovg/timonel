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
#include "TimonelTWIM.h"
#include "payload.h"

#define USE_SERIAL Serial
#define SDA 0 /* I2C SDA pin */
#define SCL 2 /* SDA SCL pin */

void setup()
{
  // put your setup code here, to run once:
  USE_SERIAL.begin(9600); /* Init the serial port */
  Timonel tml(8, SDA, SCL);
  //Timonel le_timonier(9);
  byte tml_ver_maj = tml.GetVersionMaj();
  byte tml_ver_min = tml.GetVersionMin();
  byte tml_features = tml.GetFeatures();
  USE_SERIAL.print("\n\n\rTimonel v");
  USE_SERIAL.print(tml_ver_maj);
  USE_SERIAL.print(".");
  USE_SERIAL.println(tml_ver_min);
  USE_SERIAL.print("Features code: ");
  USE_SERIAL.println(tml_features);
}

void loop()
{
  // put your main code here, to run repeatedly:
}
