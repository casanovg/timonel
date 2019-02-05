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
#define sda 0 /* SDA I2C line */
#define scl 2 /* SCL I2C line */

void setup()
{
  // put your setup code here, to run once:
  USE_SERIAL.begin(9600); /* Init the serial port */
  Timonel tml(8, sda, scl);
  Timonel popote(9);
  byte tmlVerMaj = tml.GetVersionMaj();
  byte tmlVerMin = tml.GetVersionMin();
  USE_SERIAL.print("\n\rTimonel v");
  USE_SERIAL.print(tmlVerMaj);
  USE_SERIAL.print(".");
  USE_SERIAL.println(tmlVerMin);
}

void loop()
{
  // put your main code here, to run repeatedly:
}
