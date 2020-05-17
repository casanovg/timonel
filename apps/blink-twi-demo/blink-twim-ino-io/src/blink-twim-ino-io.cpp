//
// *****
//

#include <blink-twim-ino-io.h>

// Global variables
byte block_rx_size = 0;
bool new_key = false;
char key = '\0';

byte slave_address = FindSlave();
NbMicro *p_micro = new NbMicro(slave_address, SDA, SCL);

void setup() {
    // put your setup code here, to run once:
    USE_SERIAL.begin(9600);       /* Initialize the serial port for debugging */    
    ClrScr();
    ShowHeader();
    ShowMenu();

}

void loop() {
    if (new_key == true) {
        new_key = false;
        USE_SERIAL.printf_P("\n\r");
        switch (key) {
            // *********************************
            // * Test app ||| STDPB1_1 Command *
            // *********************************
            case 'a':
            case 'A': {
                byte ret = p_micro->TwiCmdXmit(SETIO1_1, ACKIO1_1);
                if (ret) {
                    USE_SERIAL.printf_P(" > Error: %d\n\r", ret);
                } else {
                    USE_SERIAL.printf_P(" > OK: ACKIO1_1\n\r", ret);
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
                    USE_SERIAL.printf_P(" > Error: %d\n\r", ret);
                } else {
                    USE_SERIAL.printf_P(" > OK: ACKIO1_0\n\r", ret);
                }
                break;
            }                
            // *********************************
            // * Test app ||| RESETINY Command *
            // *********************************
            case 'x':
            case 'X': {
                USE_SERIAL.printf_P("\n  .\n\r . .\n\r. . .\n\n\r");
                byte ret = p_micro->TwiCmdXmit(RESETMCU, ACKRESET);
                if (ret) {
                    USE_SERIAL.printf_P("Error: %d\n\r", ret);
                } else {
                    USE_SERIAL.printf_P(" > OK: ACKRESET\n\r", ret);
                }
                                
                delay(500);
#ifdef ARDUINO_ARCH_ESP8266
                ESP.restart();
#else
                resetFunc();
#endif // ARDUINO_ARCH_ESP8266
                break;
            }
            // ******************
            // * Restart master *
            // ******************
            case 'z':
            case 'Z': {
                USE_SERIAL.printf_P("\nResetting TWI Master ...\n\r\n.\n.\n.\n");
#if ESP8266
                ESP.restart();
#else
                resetFunc();
#endif /* ESP8266 */
                break;
            }
            // *******************
            // * Unknown command *
            // *******************
            default: {
                USE_SERIAL.printf_P("Command '%d' unknown ...\n\r", key);
                break;
            }
                USE_SERIAL.printf_P("\n\r");
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
    USE_SERIAL.write(27);        // ESC command
    USE_SERIAL.printf_P("[2J");  // clear screen command
    USE_SERIAL.write(27);        // ESC command
    USE_SERIAL.printf_P("[H");   // cursor to home command
}

// Function ShowHeader
void ShowHeader(void) {
    //ClrScr();
    delay(250);
    USE_SERIAL.printf_P("\n\rBlink Twi Master Test\n\r");
    uint8_t dev_addr = p_micro->GetTwiAddress();
    if (dev_addr != 0) {
        USE_SERIAL.printf_P("Detected device at TWI address: %d\n\r", dev_addr);
    } else {
        USE_SERIAL.printf_P("No TWI device detected ...\n\r");
    }    
}

// Function ShowMenu
void ShowMenu(void) {
        USE_SERIAL.printf_P("Application command ('a' blink, 's' stop, 'z' reboot, 'x' reset MCU, '?' help): ");
}

// Function ListTwidevices
void ListTwiDevices(uint8_t sda, uint8_t scl) {
    TwiBus twi(sda, scl);
    TwiBus::DeviceInfo dev_info_arr[(((HIG_TWI_ADDR + 1) - LOW_TWI_ADDR) / 2)];
    twi.ScanBus(dev_info_arr, (((HIG_TWI_ADDR + 1) - LOW_TWI_ADDR) / 2));
    for (uint8_t i = 0; i < (((HIG_TWI_ADDR + 1) - LOW_TWI_ADDR) / 2); i++) {
        USE_SERIAL.printf_P("...........................................................\n\r");
        USE_SERIAL.printf_P("Pos: %02d | ", i + 1);
        if (dev_info_arr[i].firmware != "") {
            USE_SERIAL.printf_P("TWI Addr: %02d | ", dev_info_arr[i].addr);
            USE_SERIAL.printf_P("Firmware: %s | ", dev_info_arr[i].firmware.c_str());
            USE_SERIAL.printf_P("Version %d.%d\n\r", dev_info_arr[i].version_major, dev_info_arr[i].version_minor);
        } else {
            USE_SERIAL.printf_P("No device found\n\r");
        }
        delay(10);
    }
    USE_SERIAL.printf_P("...........................................................\n\n\r");
}

// Function FindSlave
uint8_t FindSlave(void) {
    bool *p_app_mode = false;
    TwiBus i2c(SDA, SCL);
    return (i2c.ScanBus(p_app_mode));
}