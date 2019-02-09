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
  //GetTmlID();
  reusing_twi_connection_ = true;
}

// Constructor B (Use it to open the TWI channel)
Timonel::Timonel(byte twi_address, byte sda, byte scl) {
  Wire.begin(sda, scl); /* Init I2C sda:GPIO0, scl:GPIO2 (ESP-01) / sda:D3, scl:D4 (NodeMCU) */
  addr_ = twi_address;
  TwoStepInit(0);
  //GetTmlID();  
  reusing_twi_connection_ = false;
}

// Destructor
Timonel::~Timonel() {
  if(reusing_twi_connection_ == true) {
    //USE_SERIAL.printf_P("\n\r[Class Destructor] Reused I2C connection will remain active ...\n\r");
  }
  else {
    //USE_SERIAL.printf_P("\n\r[Class Destructor] The I2C connection created by this object will be closed ...\n\r");
  }
}

// Function to know if Timonel was contacted
bool Timonel::IsTimonelContacted() {
  return(timonel_contacted_);
}

// Function to get the Timonel version major number
byte Timonel::GetVersionMaj() {
  return(tml_ver_major_);
}

// Function to get the Timonel version minor number
byte Timonel::GetVersionMin() {
  return(tml_ver_minor_);
}

// Function to get the available features
byte Timonel::GetFeatures() {
  return(tml_features_code_);
}

// Function to get the Timonel bootloader start address
word Timonel::GetTmlStart() {
  return(timonel_start_);
}

// Function to get the Timonel bootloader start address
word Timonel::GetAppStart() {
  return(application_start_);
}

// Function to get the tranpoline address
word Timonel::GetTplAddr() {
  return(trampoline_addr_);
}

// Function to get the Timonel available features code
byte Timonel::GetTmlID() {
  // I2C TX
  Wire.beginTransmission(addr_);
  Wire.write(GETTMNLV);
  Wire.endTransmission(addr_);
  // I2X RX
  block_rx_size_ = Wire.requestFrom(addr_, (int)V_CMD_LENGTH, (int)true);
  byte ackRX[V_CMD_LENGTH] = { 0 };  /* Data received from I2C slave */
  for (int i = 0; i < block_rx_size_; i++) {
    ackRX[i] = Wire.read();
  }
  if (ackRX[CMD_ACK_POS] == ACKTMNLV) {
    if (ackRX[V_SIGNATURE] == T_SIGNATURE) {
      timonel_start_ = (ackRX[V_BOOT_ADDR_MSB] << 8) + ackRX[V_BOOT_ADDR_LSB];
      application_start_ = (ackRX[V_APPL_ADDR_MSB] << 8) + ackRX[V_APPL_ADDR_LSB];
      trampoline_addr_ = (~(((ackRX[V_APPL_ADDR_MSB] << 8) | ackRX[V_APPL_ADDR_LSB]) & 0xFFF));
      trampoline_addr_++;
      trampoline_addr_ = ((((timonel_start_ >> 1) - trampoline_addr_) & 0xFFF) << 1);
      tml_signature_ = ackRX[V_SIGNATURE];
      tml_ver_major_ = ackRX[V_MAJOR];
      tml_ver_minor_ = ackRX[V_MINOR];
      tml_features_code_ = ackRX[V_FEATURES];
    }
    else {
      //USE_SERIAL.printf_P("\n\r[Timonel::GetTmlID] Error: Firmware signature unknown!\n\r");
      return(ERR_02);
    }
  }
  else {
    //USE_SERIAL.printf_P("\n\r[Timonel::GetTmlID] Error: parsing %d command! <<< %d\n\r", GETTMNLV, ackRX[0]);
    return(ERR_01);
  }
  timonel_contacted_ = true;
  return(OK);
}

// Function to upload firmware to the ATTiny85
byte Timonel::UploadFirmware(const byte payload[], int payloadsize) {
	byte packet = 0;								/* Byte counter to be sent in a single I2C data packet */
	byte padding = 0;							/* Amount of padding bytes to match the page size */
	byte pageEnd = 0;							/* Byte counter to detect the end of flash mem page */
	byte pageCount = 1;
	byte wrtErrors = 0;
	uint8_t dataPacket[TXDATASIZE] = { 0xFF };
	if ((payloadsize % FLASHPGSIZE) != 0) {		/* If the payload to be sent is smaller than flash page size, resize it to match */
		padding = ((((int)(payloadsize / FLASHPGSIZE) + 1) * FLASHPGSIZE) - payloadsize);
		payloadsize += padding;
	}
	Serial.println("\nWriting payload to flash ...\n\r");
	//if (flashPageAddr == 0xFFFF) {
	//	Serial.println("Warning: Flash page start address no set, please use 'b' command to set it ...\n\r");
	//	return(1);
	//}
	//Serial.print("::::::::::::::::::::::::::::::::::::::: Page ");
	//Serial.print(pageCount);
	//Serial.print(" - Address 0x");
	//Serial.println(flashPageAddr, HEX);
	for (int i = 0; i < payloadsize; i++) {
		if (i < (payloadsize - padding)) {
			dataPacket[packet] = payload[i];		/* If there are data to fill the page, use it ... */
		}
		else {
			dataPacket[packet] = 0xff;				/* If there are no more data, complete the page with padding (0xff) */
		}
		if (packet++ == (TXDATASIZE - 1)) {			/* When a data packet is completed to be sent ... */
			for (int b = 0; b < TXDATASIZE; b++) {
				//Serial.print("0x");
				//if (dataPacket[b] < 0x10) {
				//	Serial.print("0");
				//}
				//Serial.print(dataPacket[b], HEX);
				//Serial.print(" ");
				Serial.print(".");
			}
			wrtErrors += WritePageBuff(dataPacket);	/* Send data to T85 through I2C */
			packet = 0;
			delay(10);								/* ###### DELAY BETWEEN PACKETS SENT TO PAGE ###### */
		}
		if (wrtErrors > 0) {
			//Serial.println("\n\r==== WriteFlash: There were transmission errors, aborting ...");
			//DeleteFlash();
			TwoStepInit(2000);
#if ESP8266
			ESP.restart();
#else
			resetFunc();
#endif /* ESP8266 */
			return(wrtErrors);
		}
		if (pageEnd++ == (FLASHPGSIZE - 1)) {		/* When a page end is detected ... */

			Serial.print(pageCount++);
			//DumpPageBuff(FLASHPGSIZE, TXDATASIZE, TXDATASIZE);
			delay(100);								/* ###### DELAY BETWEEN PAGE WRITINGS ... ###### */

			if (i < (payloadsize - 1)) {
				//Serial.print("::::::::::::::::::::::::::::::::::::::: Page ");
				//Serial.print(++pageCount);
				//Serial.print(" - Address 0x");
				//Serial.println(((flashPageAddr + 1 + i) & 0xFFFF), HEX);
				pageEnd = 0;
			}
		}
	}
	if (wrtErrors == 0) {
		Serial.println("\n\n\r[UploadFirmware] Firmware was successfully transferred to T85, please select 'run app' command to start it ...");
	}
	else {
		Serial.print("\n\n\r[UploadFirmware] Communication errors detected during firmware transfer, please retry !!! ErrCnt: ");
		Serial.print(wrtErrors);
		//DeleteFlash();
		TwoStepInit(2000);
#if ESP8266
		ESP.restart();
#else
		resetFunc();
#endif /* ESP8266 */
	}
	return(wrtErrors);
}

// Function InitTiny
void Timonel::InitTiny(void) {
	byte cmdTX[1] = { INITTINY };
	Wire.beginTransmission(addr_);
	Wire.write(cmdTX[0]);
	Wire.endTransmission();
	//blockRXSize = Wire.requestFrom(slaveAddress, (byte)1);
	//blockRXSize = 0;
}

// Function TwoStepInit
void Timonel::TwoStepInit(word time) {
  delay(time);
	InitTiny();			/* Two-step Tiny85 initialization: STEP 1 */
	GetTmlID();	    /* Two-step Tiny85 initialization: STEP 2 */
}

// Function WritePageBuff
byte Timonel::WritePageBuff(uint8_t dataArray[]) {
	const byte txSize = TXDATASIZE + 2;
	byte cmdTX[txSize] = { 0 };
	byte commErrors = 0;					/* I2C communication error counter */
	byte checksum = 0;
	//Serial.println("");
	cmdTX[0] = WRITPAGE;
	for (int b = 1; b < txSize - 1; b++) {
		cmdTX[b] = dataArray[b - 1];
		checksum += (byte)dataArray[b - 1];
	}
	cmdTX[txSize - 1] = checksum;
	//Serial.print("[Timonel] Writting data to Attiny85 memory page buffer >>> ");
	//Serial.print(cmdTX[0]);
	//Serial.println("(WRITBUFF)");
	// Transmit command
	byte transmitData[txSize] = { 0 };
	//Serial.print("[Timonel] - Sending data >>> ");
	for (int i = 0; i < txSize; i++) {
		//if (i > 0) {
		//	if (i < txSize - 1) {
		//		Serial.print("0x");
		//		Serial.print(cmdTX[i], HEX);
		//		Serial.print(" ");WritePageBuff
		//	}
		//	else {
		//		Serial.print("\n\r[Timonel] - Sending CRC >>> ");
		//		Serial.println(cmdTX[i]);
		//	}
		//}
		transmitData[i] = cmdTX[i];
		Wire.beginTransmission(addr_);
		Wire.write(transmitData[i]);
		Wire.endTransmission();
	}
	// Receive acknowledgement
	byte blockRXSize = Wire.requestFrom(addr_, (byte)2);
	byte ackRX[2] = { 0 };   // Data received from slave
	for (int i = 0; i < blockRXSize; i++) {
		ackRX[i] = Wire.read();
	}
	if (ackRX[0] == ACKWTPAG) {
		//Serial.print("[Timonel] - Command ");
		//Serial.print(cmdTX[0]);
		//Serial.print(" parsed OK <<< ");
		//Serial.println(ackRX[0]);
		if (ackRX[1] == checksum) {
			//Serial.print("[Timonel] - Data parsed OK by slave <<< Checksum = 0x");
			//Serial.println(ackRX[1], HEX);
			//Serial.println("");
		}
		else {
			Serial.print("[Timonel] - Data parsed with {{{ERROR}}} <<< Checksum = 0x");
			Serial.println(ackRX[1], HEX);
			//Serial.println("");
			if (commErrors++ > 0) {					/* Checksum error detected ... */
				Serial.println("\n\r[Timonel] - WritePageBuff Checksum Errors, Aborting ...");
				exit(commErrors);
			}
		}
	}
	else {
		Serial.print("[Timonel] - Error parsing ");
		Serial.print(cmdTX[0]);
		Serial.print(" command! <<< ");
		Serial.println(ackRX[0]);
		Serial.println("");
		if (commErrors++ > 0) {					/* Opcode error detected ... */
			Serial.println("\n\r[Timonel] - WritePageBuff Opcode Reply Errors, Aborting ...");
			exit(commErrors);
		}
	}
	return(commErrors);
}