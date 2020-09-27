/*
    Blink TWI Slave for Arduino
    Author: Gustavo Casanova
    .....................................................
    File: Blink-WireS-Slave.ino (I2C blink application)
    .....................................................
    Version: 1.0 / 2020-05-16
    gustavo.casanova@nicebots.com
    .....................................................
    This a led blink I2C SLAVE test program compatible
    with the NB command-set through TWI (I2C).

    Available NB TWI commands:
    ------------------------------
    a - (SETIO1_1) Start blinking
    s - (SETIO1_0) Stop blinking
    x - (RESETMCU) Reset device
*/

// Includes
#include "Wire.h"
#include <nb-twi-cmd.h>

// Defines
#define TWI_ADDR 12 // This I2C slave address
#define LONG_DELAY 0x3FFFF

// Global variables
uint8_t command[32] = {0}; // I2C Command received from master
bool reset_now = false;
bool blink = true;
int ledState = LOW;
uint32_t toggle_delay = LONG_DELAY;
void (*resetFunc)(void) = 0;

/*  ___________________
   |                   |
   |    Setup Block    |
   |___________________|
*/
void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  Wire.begin(TWI_ADDR);
  Serial.begin(9600);
  ClrScr();
  Serial.println("\n\rBlink wire slave test started");
  Serial.println(".............................\n\r");
}

/*  ___________________
   |                   |
   |     Main Loop     |
   |___________________|
*/
void loop(void) {
  // Reset this micro
  if (reset_now == true) {
    resetFunc();
  }
  // Toggle on-board led
  if (toggle_delay-- == 0) {
    if (blink == true) {
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
      digitalWrite(LED_BUILTIN, ledState);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }
    toggle_delay = LONG_DELAY;
  }
}

/*  ________________________
   |                        |
   | TWI data receive event |
   |________________________|
*/
void receiveEvent(int received_bytes) {
  for (uint8_t i = 0; i < received_bytes; i++) {
    command[i] = Wire.read();
    Serial.print("0x");
    Serial.print(command[i], HEX);
    Serial.print(" ");
  }
}

/*  ________________________
   |                        |
   | TWI data request event |
   |________________________|
*/
void requestEvent(void) {
  byte opCodeAck = ~command[0]; /* Command Operation Code acknowledge => Command Bitwise "Not". */
  switch (command[0]) {
    // ******************
    // * SETIO1_1 Reply *
    // ******************
    case SETIO1_1: {
        blink = true;
        Wire.write(opCodeAck);
        break;
      }
    // ******************
    // * SETIO1_0 Reply *
    // ******************
    case SETIO1_0: {
        blink = false;
        Wire.write(opCodeAck);
        break;
      }
    case INFORMAT: {
        char info[6] = {'H', 'e', 'l', 'l', 'o', '!'};
        Wire.write(opCodeAck);
        for (int i = 0; i < sizeof(info); i++) {
          Wire.write(info[i]);
        }
        break;
      }
    // ******************
    // * RESETMCU Reply *
    // ******************
    case RESETMCU: {
        Wire.write(opCodeAck);
        reset_now = true;
        break;
      }
    // *************************
    // * Unknown Command Reply *
    // *************************
    default: {
        Wire.write(UNKNOWNC);
        break;
      }
  }
}

/*  ______________
   |              |
   | Clear Screen |
   |______________|
*/
void ClrScr(void) {
  Serial.write(27);     // ESC command
  Serial.print("[2J");  // clear screen command
  Serial.write(27);     // ESC command
  Serial.print("[H");   // cursor to home command
}
