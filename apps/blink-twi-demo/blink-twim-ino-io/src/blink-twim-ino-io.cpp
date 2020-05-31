/*
 *  Blink TWI Master for Arduino (PlatformIO)
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: blink-twim-ino-io.cpp (Application)
 *  ........................................... 
 *  Version: 1.0 / 2020-05-29
 *  gustavo.casanova@gmail.com
 *  ...........................................
 *  This library allows scanning the TWI (I2C) bus in search
 *  of connected devices addresses and data. If a device found
 *  is running Timonel, it returns its version number.
 */

#include <blink-twim-ino-io.h>

// Global variables
NbMicro *p_micro = nullptr;
bool new_key = false;
char key = '\0';

// put your setup code here, to run once:
void setup() {
    USE_SERIAL.begin(9600);  // Initialize the serial port for debugging
    ClrScr();
    // Detect slave
    byte slave_address = FindSlave();
    if (slave_address) {
        p_micro = new NbMicro(slave_address, SDA, SCL);
    } else {
        USE_SERIAL.println("No slave detected over the TWI bus");
        delay(1000);
        abort();
    }
    ShowHeader();
    ShowMenu();
}

// Main loop
void loop() {
    if (new_key == true) {
        new_key = false;
        USE_SERIAL.println("");
        switch (key) {
            // *********************************
            // * Test app ||| STDPB1_1 Command *
            // *********************************
            case 'a':
            case 'A': {
                byte ret = p_micro->TwiCmdXmit(SETIO1_1, ACKIO1_1);
                if (ret) {
                    USE_SERIAL.print(" > Error: ");
                    USE_SERIAL.println(ret);

                } else {
                    USE_SERIAL.println(" > OK: ACKIO1_1");
                }
                break;
            }
            // *********************************
            // * Test app ||| STDPB1_0 Command *
            // *********************************
            case 's':
            case 'S': {
                byte ret = p_micro->TwiCmdXmit(SETIO1_0, ACKIO1_0);
                if (ret) {
                    USE_SERIAL.print(" > Error: ");
                    USE_SERIAL.println(ret);

                } else {
                    USE_SERIAL.println(" > OK: ACKIO1_0");
                }
                break;
            }
            // *********************************
            // * Test app ||| INFORMAT Command *
            // *********************************
            case 'i':
            case 'I': {
                byte ret_len = 1 + 6;  // 1 ack byte + 6 data bytes
                char info_ret[ret_len];
                byte ret = p_micro->TwiCmdXmit(INFORMAT, ACKINFOR, (byte *)info_ret, ret_len);
                if (ret) {
                    USE_SERIAL.print(" > Error: ");
                    USE_SERIAL.println(ret);

                } else {
                    USE_SERIAL.println(" > OK: ACKINFOR");
                    USE_SERIAL.println("");
                    for (byte i = 1; i < ret_len; i++) {
                        USE_SERIAL.print(info_ret[i]);
                    }
                    USE_SERIAL.println("\n\r");
                }
                break;
            }
            // *********************************
            // * Test app ||| RESETINY Command *
            // *********************************
            case 'x':
            case 'X': {
                USE_SERIAL.print("\n  .\n\r . .\n\r. . .\n\n\r");
                byte ret = p_micro->TwiCmdXmit(RESETMCU, ACKRESET);
                if (ret) {
                    USE_SERIAL.print(" > Error: ");
                    USE_SERIAL.println(ret);

                } else {
                    USE_SERIAL.println(" > OK: ACKRESET");
                }
                delay(500);
#ifdef ARDUINO_ARCH_ESP8266
                ESP.restart();
#else
                resetFunc();
#endif  // ARDUINO_ARCH_ESP8266
                break;
            }
            // ******************
            // * Restart master *
            // ******************
            case 'z':
            case 'Z': {
                USE_SERIAL.print("\n\rResetting TWI Master ...\n\n\r  .\n\r . .\n\r. . .\n\n\r");
#ifdef ARDUINO_ARCH_ESP8266
                ESP.restart();
#else
                resetFunc();
#endif  // ARDUINO_ARCH_ESP8266
                break;
            }
            // *******************
            // * Unknown command *
            // *******************
            default: {
                USE_SERIAL.print("Command '");
                USE_SERIAL.print(key);
                USE_SERIAL.println(" ' unknown ...");

                break;
            }
        }
        ShowMenu();
    }
    ReadChar();
}

// Function ReadChar
void ReadChar(void) {
    if (USE_SERIAL.available() > 0) {
        key = USE_SERIAL.read();
        new_key = true;
    }
}

// Function clear screen
void ClrScr(void) {
    USE_SERIAL.write(27);     // ESC command
    USE_SERIAL.print("[2J");  // clear screen command
    USE_SERIAL.write(27);     // ESC command
    USE_SERIAL.print("[H");   // cursor to home command
}

// Function ShowHeader
void ShowHeader(void) {
    //ClrScr();
    delay(250);
    USE_SERIAL.print("\n\rBlink TWI Master Test ");
#if (ARDUINO_ARCH_ESP8266 || ARDUINO_ESP32_DEV || ESP_PLATFORM)
    USE_SERIAL.print("(ESP ");
#else   // -----
    USE_SERIAL.print("(AVR ");
#endif  // ARDUINO_ARCH_ESP8266
    USE_SERIAL.println("microprocessor)");
    USE_SERIAL.println("...............................................................");
    uint8_t dev_addr = p_micro->GetTwiAddress();
    if (dev_addr != 0) {
        USE_SERIAL.print("Detected device at TWI address: ");
        USE_SERIAL.println(dev_addr);
    } else {
        USE_SERIAL.println("No TWI device detected ...");
    }
}

// Function ShowMenu
void ShowMenu(void) {
    USE_SERIAL.print("Application command ('a' blink, 's' stop, 'i' info, 'z' reboot, 'x' reset slave): ");
}

// Function FindSlave
uint8_t FindSlave(void) {
    bool app_mode = false;
    bool *p_app_mode = &app_mode;
    TwiBus twi_bus(SDA, SCL);
    return (twi_bus.ScanBus(p_app_mode));
}
