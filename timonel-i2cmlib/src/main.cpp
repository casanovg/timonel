#include <Arduino.h>
#include "TimonelTWIM.h"
#include "payload.h"

#define USE_SERIAL Serial

void setup() {
  // put your setup code here, to run once:
  USE_SERIAL.begin(9600);   // Init the serial port
  Timonel tml(3);
  byte tmlVerMaj = tml.GetVersionMaj();
  byte tmlVerMin = tml.GetVersionMin();
  USE_SERIAL.print("\n\rTimonel v");
  USE_SERIAL.print(tmlVerMaj);
  USE_SERIAL.print(".");
  USE_SERIAL.println(tmlVerMin);
}

void loop() {
  // put your main code here, to run repeatedly:
}
