/*
  NbTinyX5.cpp
  ============
  Library code for I2C communications with an Atmel
  ATTiny85 microcontroller using the NB protocol.
  ---------------------------
  2019-03-08 Gustavo Casanova
  ---------------------------
*/

#include "NBTinyX5.h"
#include "../TimonelTwiM/TimonelTwiM.h"

// Class constructor
NbTinyX5::NbTinyX5(byte twi_address, byte sda, byte scl) : addr_(twi_address), sda_(sda), scl_(scl) {
	if (!((sda == 0) && (scl == 0))) 	{
		USE_SERIAL.printf_P("\n\r[%s] Creating a new I2C connection\n\r", __func__);
		Wire.begin(sda, scl); /* Init I2C sda:GPIO0, scl:GPIO2 (ESP-01) / sda:D3, scl:D4 (NodeMCU) */
		reusing_twi_connection_ = false;
	}
	else 	{
		USE_SERIAL.printf_P("\n\r[%s] Reusing I2C connection\n\r", __func__);
    reusing_twi_connection_ = true;
	}
}

// Function InitTiny
byte NbTinyX5::InitTiny(void) {
	Wire.beginTransmission(addr_);
	Wire.write(INITTINY);
	Wire.endTransmission();
	Wire.requestFrom(addr_, (byte)1);
	//byte block_rx_size = 0;
	return (0);
}

// Function TWI command transmit (Overload A: transmit single byte command)
byte NbTinyX5::TwiCmdXmit(byte twi_cmd, byte twi_reply, byte twi_reply_arr[], byte reply_size) {
	const byte cmd_size = 1;
	byte twi_cmd_arr[cmd_size] = {twi_cmd};
	return (TwiCmdXmit(twi_cmd_arr, cmd_size, twi_reply, twi_reply_arr, reply_size));
}

// Function TWI command transmit (Overload B: transmit multibyte command)
byte NbTinyX5::TwiCmdXmit(byte twi_cmd_arr[], byte cmd_size, byte twi_reply, byte twi_reply_arr[], byte reply_size) {
	for (int i = 0; i < cmd_size; i++) {
		Wire.beginTransmission(addr_);
		Wire.write(twi_cmd_arr[i]);
		Wire.endTransmission();
	}
	// Receive reply
	if (reply_size == 0) {
		Wire.requestFrom(addr_, ++reply_size);
		byte reply = Wire.read();
		if (reply == twi_reply) { /* I2C reply from slave */
			USE_SERIAL.printf_P("[%s] Command %d parsed OK <<< %d\n\n\r", __func__, twi_cmd_arr[0], reply);
			return (0);
		}
		else {
			USE_SERIAL.printf_P("[%s] Error parsing %d command <<< %d\n\n\r", __func__, twi_cmd_arr[0], reply);
			return (1);
		}
	}
	else {
		byte reply_length = Wire.requestFrom(addr_, reply_size);
		for (int i = 0; i < reply_size; i++) {
			twi_reply_arr[i] = Wire.read();
		}
		if ((twi_reply_arr[0] == twi_reply) && (reply_length == reply_size)) {
			//USE_SERIAL.printf_P("[%s] Multibyte command %d parsed OK <<< %d\n\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
			return (0);
		}
		else {
			USE_SERIAL.printf_P("[%s] Error parsing %d multibyte command <<< %d\n\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
			return (2);
		}
	}
}

// Class constructor
TwiBus::TwiBus(byte sda, byte scl) : sda_(sda), scl_(scl) {
	if (!((sda == 0) && (scl == 0))) {
		USE_SERIAL.printf_P("\n\r[%s] Creating a new I2C connection\n\r", __func__);
		Wire.begin(sda, scl); /* Init I2C sda:GPIO0, scl:GPIO2 (ESP-01) / sda:D3, scl:D4 (NodeMCU) */
		reusing_twi_connection_ = false;
	}
	else {
		USE_SERIAL.printf_P("\n\r[%s] Reusing I2C connection\n\r", __func__);
		reusing_twi_connection_ = true;
	}
}

// Function ScanTWI (Overload A: Return the address and mode of the first TWI device found on the bus)
byte TwiBus::ScanBus(bool *p_app_mode) {
	//
	// Address 08 to 35: Timonel bootloader
	// Address 36 to 63: Application firmware
	// Each I2C slave must have a unique bootloader address that corresponds
	// to a defined application address, as shown in this table:
	// T: |08|09|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|
	// A: |36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|
	//
	USE_SERIAL.println("Scanning I2C bus ...");
	byte scanned_addr = 8;
	while (scanned_addr < 120) {
		Wire.beginTransmission(scanned_addr);
		if (Wire.endTransmission() == 0) {
			if (scanned_addr < 36) {
				USE_SERIAL.printf_P("Timonel Bootloader found at address: %d (0x%X)", scanned_addr, scanned_addr);
				*p_app_mode = false;
			}
			else {
				USE_SERIAL.printf_P("Test App Firmware found at address: %d (0x%X)", scanned_addr, scanned_addr);
				*p_app_mode = true;
			}
			return (scanned_addr);
		}
		delay(500);
		scanned_addr++;
	}
}

// Function ScanTWI (Overload B: Fill an array with the address, firmware and version of all devices found on the bus)
byte TwiBus::ScanBus(struct device device_arr[]) {
	//
	// Address 08 to 35: Timonel bootloader
	// Address 36 to 63: Application firmware
	// Each I2C slave must have a unique bootloader address that corresponds
	// to a defined application address, as shown in this table:
	// T: |08|09|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|
	// A: |36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|
	//
	USE_SERIAL.println("Scanning I2C bus ...");
	byte scanned_addr = 8;
	while (scanned_addr < 64) {
		Wire.beginTransmission(scanned_addr);
		if (Wire.endTransmission() == 0) {
			if (scanned_addr < 36) {
				Timonel tml(scanned_addr);
				struct Timonel::status sts = tml.GetStatus();
				device_arr[scanned_addr - 8].addr = scanned_addr;
				if (sts.signature == 84) {
					device_arr[scanned_addr - 8].firmware = "Timonel";
				}
				device_arr[scanned_addr - 8].version_major = sts.version_major;
				device_arr[scanned_addr - 8].version_minor = sts.version_minor;
			}
			else {
				device_arr[scanned_addr - 8].addr = scanned_addr;
				device_arr[scanned_addr - 8].firmware = "Application";
				device_arr[scanned_addr - 8].version_major = 0;
				device_arr[scanned_addr - 8].version_minor = 1;
			}
		}
		delay(500);
		scanned_addr++;
	}
	return(0);
}