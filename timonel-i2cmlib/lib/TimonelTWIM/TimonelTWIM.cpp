/*
  TimonelTWIM.cpp
  ===============
  Library code for uploading firmware to an Atmel ATTiny85
  microcontroller that runs the Timonel I2C bootloader.
  ---------------------------
  2018-12-13 Gustavo Casanova
  ---------------------------
*/

#include "TimonelTWIM.h"

// Constructor A (Use it when a TWI channel is already opened)
Timonel::Timonel(byte twi_address) {
  addr_ = twi_address;
  TwoStepInit(0);
  reusing_twi_connection_ = true;
}

// Constructor B (Use it to open the TWI channel)
Timonel::Timonel(byte twi_address, byte sda, byte scl) {
  Wire.begin(sda, scl); /* Init I2C sda:GPIO0, scl:GPIO2 (ESP-01) / sda:D3, scl:D4 (NodeMCU) */
  addr_ = twi_address;
  TwoStepInit(0);
  reusing_twi_connection_ = false;
}

// Class destructor
Timonel::~Timonel(void) {
  if(reusing_twi_connection_ == true) {
    //USE_SERIAL.printf_P("\n\r[Class Destructor] Reused I2C connection will remain active ...\n\r");
  }
  else {
    //USE_SERIAL.printf_P("\n\r[Class Destructor] The I2C connection created by this object will be closed ...\n\r");
  }
}

// Function to check the status parameters of the bootloader running on the ATTiny85
byte Timonel::QueryID(void) {
	struct status status_;
	// I2C TX

  	// Wire.beginTransmission(addr_);
  	// Wire.write(GETTMNLV);
  	// Wire.endTransmission(addr_);
  	// // I2X RX
  	// block_rx_size_ = Wire.requestFrom(addr_, V_CMD_LENGTH, true);
  	byte ack_rx[V_CMD_LENGTH] = { 0 };  /* Data received from I2C slave */
  	// for (int i = 0; i < block_rx_size_; i++) {
	//     ack_rx[i] = Wire.read();
  	// }
  	
	byte twi_error = TWICmdSingle(GETTMNLV, ACKTMNLV, V_CMD_LENGTH, ack_rx);
	  
	if((ack_rx[CMD_ACK_POS] == ACKTMNLV) && (ack_rx[V_SIGNATURE] == T_SIGNATURE)) {
		if((id_.signature == 0) && (id_.version_major == 0) && (id_.version_minor == 0) && (id_.features_code == 0)) {
			id_.signature = ack_rx[V_SIGNATURE];
			id_.version_major = ack_rx[V_MAJOR];
			id_.version_minor = ack_rx[V_MINOR];
			id_.features_code = ack_rx[V_FEATURES];
		}
		status_.bootloader_start = (ack_rx[V_BOOT_ADDR_MSB] << 8) + ack_rx[V_BOOT_ADDR_LSB];
		status_.application_start = (ack_rx[V_APPL_ADDR_LSB] << 8) + ack_rx[V_APPL_ADDR_MSB];
		status_.trampoline_addr = (~(((ack_rx[V_APPL_ADDR_MSB] << 8) | ack_rx[V_APPL_ADDR_LSB]) & 0xFFF));
		status_.trampoline_addr++;
		status_.trampoline_addr = ((((status_.bootloader_start >> 1) - status_.trampoline_addr) & 0xFFF) << 1);
		}
  	else {
	    //USE_SERIAL.printf_P("\n\r[Timonel::GetTmlID] Error: parsing %d command! <<< %d\n\r", GETTMNLV, ack_rx[0]);
	    return(ERR_01);
  	}
  	return(OK);
}

// Function to get all the id parameters of a Timonel instance
Timonel::id Timonel::GetID(void) {
	return id_;
}

Timonel::status Timonel::GetStatus(void) {
	struct status status_;
  	// I2C TX
  	// Wire.beginTransmission(addr_);
  	// Wire.write(GETTMNLV);
  	// Wire.endTransmission(addr_);
  	// // I2X RX
  	// block_rx_size_ = Wire.requestFrom(addr_, V_CMD_LENGTH, true);
  	byte ack_rx[V_CMD_LENGTH] = { 0 };  /* Data received from I2C slave */
  	// for (int i = 0; i < block_rx_size_; i++) {
	//     ack_rx[i] = Wire.read();
  	// }

	byte twi_error = TWICmdSingle(GETTMNLV, ACKTMNLV, V_CMD_LENGTH, ack_rx);

	if ((ack_rx[CMD_ACK_POS] == ACKTMNLV) && (ack_rx[V_SIGNATURE] == T_SIGNATURE)) {
		status_.bootloader_start = (ack_rx[V_BOOT_ADDR_MSB] << 8) + ack_rx[V_BOOT_ADDR_LSB];
		status_.application_start = (ack_rx[V_APPL_ADDR_LSB] << 8) + ack_rx[V_APPL_ADDR_MSB];
		status_.trampoline_addr = (~(((ack_rx[V_APPL_ADDR_MSB] << 8) | ack_rx[V_APPL_ADDR_LSB]) & 0xFFF));
		status_.trampoline_addr++;
		status_.trampoline_addr = ((((status_.bootloader_start >> 1) - status_.trampoline_addr) & 0xFFF) << 1);
	}
	else {
		//USE_SERIAL.printf_P("\n\r[Timonel::GetTmlID] Error: Firmware signature unknown!\n\r");
	}
	return(status_);
}

// Function InitTiny
void Timonel::InitTiny(void) {
	Wire.beginTransmission(addr_);
	Wire.write(INITTINY);
	Wire.endTransmission();
	Wire.requestFrom(addr_, (byte)1);
	//byte block_rx_size = 0;
}

// Function TwoStepInit
void Timonel::TwoStepInit(word time) {
	delay(time);
	InitTiny();					/* Two-step Tiny85 initialization: STEP 1 */
	//QueryTmlStatus(); /* Two-step Tiny85 initialization: STEP 2 */
	//GetStatus();
	QueryID();
}

// Function WritePageBuff
byte Timonel::WritePageBuff(uint8_t data_array[]) {
	const byte tx_size = TXDATASIZE + 2;
	byte cmd_tx[tx_size] = { 0 };
	byte comm_errors = 0;					/* I2C communication error counter */
	byte checksum = 0;
	cmd_tx[0] = WRITPAGE;
	for (int b = 1; b < tx_size - 1; b++) {
		cmd_tx[b] = data_array[b - 1];
		checksum += (byte)data_array[b - 1];
	}
	cmd_tx[tx_size - 1] = checksum;
	byte transmit_data[tx_size] = { 0 };
	for (int i = 0; i < tx_size; i++) {
		transmit_data[i] = cmd_tx[i];
		Wire.beginTransmission(addr_);
		Wire.write(transmit_data[i]);
		Wire.endTransmission();
	}
	// Receive acknowledgement
	byte block_rx_size = Wire.requestFrom(addr_, (byte)2);
	byte ack_rx[2] = { 0 };   // Data received from slave
	for (int i = 0; i < block_rx_size; i++) {
		ack_rx[i] = Wire.read();
	}
	if (ack_rx[0] == ACKWTPAG) {
		if (ack_rx[1] == checksum) {
		}
		else {
			Serial.printf_P("[WritePageBuff] Data parsed with {{{ERROR}}} <<< Checksum = 0x%X\r\n", ack_rx[1]);
			if (comm_errors++ > 0) {					/* Checksum error detected ... */
				Serial.printf_P("\n\r[WritePageBuff] Checksum Errors, Aborting ...\r\n");
				exit(comm_errors);
			}
		}
	}
	else {
		Serial.printf_P("[WritePageBuff] Error parsing %d command! <<< %d\r\n", cmd_tx[0], ack_rx[0]);
		if (comm_errors++ > 0) {					/* Opcode error detected ... */
			Serial.printf_P("\n\r[WritePageBuff] Opcode Reply Errors, Aborting ...\n\r");
			exit(comm_errors);
		}
	}
	return(comm_errors);
}

// Function to determine if there is an ATTiny85 application update
bool Timonel::CheckNewApp(void) {
	return true;
}

// Function to upload firmware to the ATTiny85
byte Timonel::UploadFirmware(const byte payload[], int payload_size, int start_address) {
	byte packet = 0;															/* Byte counter to be sent in a single I2C data packet */
	byte padding = 0;															/* Amount of padding bytes to match the page size */
	byte page_end = 0;														/* Byte counter to detect the end of flash mem page */
	byte page_count = 1;
	byte wrt_errors = 0;
	byte data_packet[TXDATASIZE] = { 0xFF };
	if ((payload_size % FLASHPGSIZE) != 0) {			/* If the payload to be sent is smaller than flash page size, resize it to match */
		padding = ((((int)(payload_size / FLASHPGSIZE) + 1) * FLASHPGSIZE) - payload_size);
		payload_size += padding;
	}
	Serial.printf_P("\n[UploadFirmware] Writing payload to flash, starting at 0x%04X ...\n\n\r", start_address);
	for (int i = 0; i < payload_size; i++) {
		if (i < (payload_size - padding)) {
			data_packet[packet] = payload[i];					/* If there are data to fill the page, use it ... */
		}
		else {
			data_packet[packet] = 0xff;								/* If there are no more data, complete the page with padding (0xff) */
		}
		if (packet++ == (TXDATASIZE - 1)) {					/* When a data packet is completed to be sent ... */
			for (int b = 0; b < TXDATASIZE; b++) {
				Serial.printf_P(".");
			}
			wrt_errors += WritePageBuff(data_packet);	/* Send data to T85 through I2C */
			packet = 0;
			delay(10);																/* ###### DELAY BETWEEN PACKETS SENT TO PAGE ###### */
		}
		if (wrt_errors > 0) {
			//DeleteFlash();
			TwoStepInit(2000);
#if ESP8266
			ESP.restart();
#else
			resetFunc();
#endif /* ESP8266 */
			return(wrt_errors);
		}
		if (page_end++ == (FLASHPGSIZE - 1)) {			/* When a page end is detected ... */

			Serial.printf_P("%d", page_count++);
			//DumpPageBuff(FLASHPGSIZE, TXDATASIZE, TXDATASIZE);
			delay(100);																/* ###### DELAY BETWEEN PAGE WRITINGS ... ###### */

			if (i < (payload_size - 1)) {
				page_end = 0;
			}
		}
	}
	if (wrt_errors == 0) {
		Serial.printf_P("\n\n\r[UploadFirmware] Application was successfully transferred to T85, please select 'run app' command to start it ...\n\r");
	}
	else {
		Serial.printf_P("\n\n\r[UploadFirmware] Communication errors detected during firmware transfer, please retry !!! ErrCnt: %d\n\r", wrt_errors);
		//DeleteFlash();
		TwoStepInit(2000);
#if ESP8266
		ESP.restart();
#else
		resetFunc();
#endif /* ESP8266 */
	}
	return(wrt_errors);
}

// Function RunApplication
byte Timonel::RunApplication(void) {
	Serial.printf_P("\n\r[RunApplication] Exit bootloader & run application >>> %d\r\n", EXITTMNL);
	return(TWICmdSingle(EXITTMNL, ACKEXITT));
}

// Function DeleteFirmware
byte Timonel::DeleteFirmware(void) {
	Serial.printf_P("\n\r[DeleteFirmware] Delete Flash Memory >>> %d\r\n", DELFLASH);
	return(TWICmdSingle(DELFLASH, ACKDELFL));
}

// Function TWICmdSingle
  byte Timonel::TWICmdSingle(byte twi_command, byte twi_acknowledge, byte reply_size, byte reply_array[]) {
	// Transmit command
	Wire.beginTransmission(addr_);
	Wire.write(twi_command);				/* Transmit I2C command to slave */
	Wire.endTransmission();
	// Receive reply
	//byte reply_length = Wire.requestFrom(addr_, reply_size, true);
	if (reply_size == 0) {
		Wire.requestFrom(addr_, ++reply_size, true);
		if (Wire.read() == twi_acknowledge) {	/* Return I2C reply from slave */
			Serial.printf_P("[TWICmdSingle] Command %d parsed OK <<< %d\n\n\r", twi_command, twi_acknowledge);
			return(0);
		}
		else {
			Serial.printf_P("[TWICmdSingle] Error parsing %d command <<< %d\n\n\r", twi_command, twi_acknowledge);
			return(1);
		}
	}
	else {
		byte reply_length = Wire.requestFrom(addr_, reply_size, true);
  		for (int i = 0; i < reply_size; i++) {
	    	reply_array[i] = Wire.read();
  		}
	 	if ((reply_array[0] == twi_acknowledge) && (reply_length == reply_size)) {
			return(0);
		}
		else {
			return(1);
		}		  
	}
}

