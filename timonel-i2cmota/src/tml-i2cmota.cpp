#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#define USE_SERIAL Serial
#define BLINK_DELAY 0x7FFF;

ESP8266WiFiMulti WiFiMulti;

// Blink
const int ledPin =  2;                // the number of the LED pin (2 = GPIO2 / ESP-01)
int ledState = LOW;                   // ledState used to set the LED
//unsigned long previousMillis = 0;     // will store last time LED was updated
//const long interval = 1000;            // interval at which to blink (milliseconds)
int blinkDly = BLINK_DELAY;

void setup() {
  // put your setup code here, to run once:
  USE_SERIAL.begin(9600);
  USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf_P("[SETUP] WAIT %d...\n\r", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("Nicebots.com", "R2-D2 C-3P0");

    // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    t_httpUpdate_return ret = ESPhttpUpdate.update( F("http://fw.nicebots.com/update.php") );
    //t_httpUpdate_return  ret = ESPhttpUpdate.update("https://server/file.bin", "", "fingerprint");

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        USE_SERIAL.printf_P("HTTP_UPDATE_FAILED Error (%d): %s\n\r", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        USE_SERIAL.println(F("HTTP_UPDATE_NO_UPDATES\n\r"));
        break;

      case HTTP_UPDATE_OK:
        USE_SERIAL.println(F("HTTP_UPDATE_OK\n\r"));
        break;
    }
  }  

  USE_SERIAL.println(F("\n\rNB setup and update finished, starting loop code ...\n\r"));
  USE_SERIAL.printf_P("Led blink delay: 0x%04X\n\n\r", blinkDly);

  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:

  if (blinkDly-- <= 0) {
    blinkDly = BLINK_DELAY;
    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);    
  }
  
}