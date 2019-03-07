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

// Class constructor
Timonel::Timonel(byte twi_address, byte sda, byte scl) : addr_(twi_address) {
	if (!((sda == 0) && (scl == 0))) {
		USE_SERIAL.printf_P("\n\r[%s] Creating a new I2C connection\n\r", __func__);		
		Wire.begin(sda, scl); /* Init I2C sda:GPIO0, scl:GPIO2 (ESP-01) / sda:D3, scl:D4 (NodeMCU) */
		reusing_twi_connection_ = false;
	}
	else {
		USE_SERIAL.printf_P("\n\r[%s] Reusing I2C connection\n\r", __func__);
		reusing_twi_connection_ = true;
	}
  	//addr_ = twi_address;
  	TwoStepInit(0);
}

// Function to check the status parameters of the bootloader running on the ATTiny85
byte Timonel::QueryStatus(void) {
  	byte twi_reply_arr[V_CMD_LENGTH] = { 0 };  					/* Status received from I2C slave */
	byte twi_cmd_err = TwiCmdXmit(GETTMNLV, ACKTMNLV, twi_reply_arr, V_CMD_LENGTH);
	if ((twi_cmd_err == 0) && (twi_reply_arr[CMD_ACK_POS] == ACKTMNLV) && (twi_reply_arr[V_SIGNATURE] == T_SIGNATURE)) {
		status_.signature = twi_reply_arr[V_SIGNATURE];
		status_.version_major = twi_reply_arr[V_MAJOR];
		status_.version_minor = twi_reply_arr[V_MINOR];
		status_.features_code = twi_reply_arr[V_FEATURES];
		status_.bootloader_start = (twi_reply_arr[V_BOOT_ADDR_MSB] << 8) + twi_reply_arr[V_BOOT_ADDR_LSB];
		status_.application_start = (twi_reply_arr[V_APPL_ADDR_LSB] << 8) + twi_reply_arr[V_APPL_ADDR_MSB];
		status_.trampoline_addr = (~(((twi_reply_arr[V_APPL_ADDR_MSB] << 8) | twi_reply_arr[V_APPL_ADDR_LSB]) & 0xFFF));
		status_.trampoline_addr++;
		status_.trampoline_addr = ((((status_.bootloader_start >> 1) - status_.trampoline_addr) & 0xFFF) << 1);
	}
  	else {
	    //USE_SERIAL.printf_P("\n\r[%s] Error: parsing %d command! <<< %d\n\r", __func__, GETTMNLV, twi_reply_arr[0]);
	    return(ERR_01);
  	}
  	return(OK);
}

// Get the Timonel running status
Timonel::status Timonel::GetStatus(void) {
	QueryStatus();
	return(status_);
}

// Function InitTiny
byte Timonel::InitTiny(void) {
	Wire.beginTransmission(addr_);
	Wire.write(INITTINY);
	Wire.endTransmission();
	Wire.requestFrom(addr_, (byte)1);
	//byte block_rx_size = 0;
	return(0);
}

// Function TwoStepInit
byte Timonel::TwoStepInit(word time) {
	delay(time);
	InitTiny();												/* Two-step Tiny85 initialization: STEP 1 */
	return(QueryStatus()); 											/* Two-step Tiny85 initialization: STEP 2 */
}

// Function WritePageBuff
byte Timonel::WritePageBuff(byte data_array[]) {
	const byte cmd_size = TXDATASIZE + 2;
	const byte reply_size = 2;
	byte twi_cmd[cmd_size] = { 0 };
	byte twi_reply_arr[reply_size] = { 0 };
	byte checksum = 0;
	twi_cmd[0] = WRITPAGE;
	for (int i = 1; i < cmd_size - 1; i++) {
		twi_cmd[i] = data_array[i - 1];
		checksum += (byte)data_array[i - 1];				/* Data checksum accumulator (mod 256) */
	}
	twi_cmd[cmd_size - 1] = checksum;
	byte wrt_errors = TwiCmdXmit(twi_cmd, cmd_size, ACKWTPAG, twi_reply_arr, reply_size);
	if (twi_reply_arr[0] == ACKWTPAG) {
		if (twi_reply_arr[1] != checksum) {
			USE_SERIAL.printf_P("[%s] Data parsed with {{{ERROR}}} <<< Checksum = 0x%X\r\n", __func__, twi_reply_arr[1]);
			if (wrt_errors++ > 0) {						/* Checksum error detected ... */
				USE_SERIAL.printf_P("\n\r[%s] Checksum Errors, Aborting ...\r\n", __func__);
				exit(wrt_errors);
			}
		}
	}
	else {
		USE_SERIAL.printf_P("[%s] Error parsing %d command! <<< %d\r\n", __func__, twi_cmd[0], twi_reply_arr[0]);
		if (wrt_errors++ > 0) {							/* Opcode error detected ... */
			USE_SERIAL.printf_P("\n\r[%s] Opcode Reply Errors, Aborting ...\n\r", __func__);
			exit(wrt_errors);
		}
	}
	return(wrt_errors);
}

// Upload a user application to an ATTiny85 running Timonel
byte Timonel::UploadApplication(const byte payload[], int payload_size, int start_address) {
	
	byte packet = 0;										/* Byte amount to be sent in a single I2C data packet */
	byte padding = 0;										/* Amount of padding bytes to match the page size */
	byte page_end = 0;										/* Byte counter to detect the end of flash mem page */
	byte page_count = 1;									/* Current page counter */
	byte upl_errors = 0;									/* Upload error counter */
	byte data_packet[TXDATASIZE] = { 0xFF };				/* TWI data packet array */
	bool app_use_tpl = false;								/* Application use trampoline page flag */
	
	if ((status_.features_code & 0x08) != false) {			/* If CMD_STPGADDR is enabled */
		
		if (start_address >= PAGE_SIZE) {					/* If application start address is not 0 */
			USE_SERIAL.printf_P("\n\n\r[%s] Application doesn't start at 0, fixing reset vector to jump to Timonel ...\n\n\r", __func__);
			FillSpecialPage(1);
			SetPageAddress(start_address);					/* Calculate and fill reset page */
			delay(100);
		}

		if ((status_.features_code & 0x02) == false) {		/* if AUTO_TPL_CALC is disabled */
			if (payload_size <= status_.bootloader_start - PAGE_SIZE) {	/* if the application doesn't use the trampoline page */
				USE_SERIAL.printf_P("\n\n\r[%s] Application doesn't use trampoline page ...\n\n\r", __func__);
				FillSpecialPage(2, payload[1], payload[0]);	/* Calculate and fill trampoline page */
				SetPageAddress(start_address);
			}
			else {
				if (payload_size <= status_.bootloader_start) { /* if the application DO use the trampoline page, set a flag */
					USE_SERIAL.printf_P("\n\n\r[%s] Application uses trampoline page ...\n\n\r", __func__);
					// SET BONGO SET BONGO SET BONGO SET BONGO SET BONGO SET BONGO SET BONGO SET BONGO 
					// SET BONGO SET BONGO SET BONGO SET BONGO SET BONGO SET BONGO SET BONGO SET BONGO 
					app_use_tpl = true;
				}
				else { /* if the application overlaps the bootloader */
					USE_SERIAL.printf_P("\n\n\r[%s] Application doesn't fit in flash memory ...\n\r", __func__);
					USE_SERIAL.printf_P("[%s] Bootloafer start: %d\n\r", __func__, status_.bootloader_start);
					USE_SERIAL.printf_P("[%s] Application size: %d\n\r", __func__, payload_size);
					USE_SERIAL.printf_P("[%s] ----------------------\n\r", __func__);
					USE_SERIAL.printf_P("[%s]         Overflow: %d\n\r", __func__, payload_size - status_.bootloader_start);
					return(2);
				}
			}
			delay(100);
		}
	}

	if ((payload_size % PAGE_SIZE) != 0) { /* If the payload doesn't use an exact number of pages, resize it by adding padding data */
		padding = ((((int)(payload_size / PAGE_SIZE) + 1) * PAGE_SIZE) - payload_size);
		payload_size += padding;
	}

	USE_SERIAL.printf_P("\n\r[%s] Writing payload to flash, starting at 0x%04X ...\n\n\r", __func__, start_address);

	for (int i = 0; i < payload_size; i++) {
		if (i < (payload_size - padding)) {
			data_packet[packet] = payload[i];				/* If there are data to fill the page, use it ... */
		}
		else {
			data_packet[packet] = 0xFF;						/* If there are no more data, complete the page with padding (0xff) */
		}

		// BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO 
		// BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO 
		// BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO 
		// BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO 
		// BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO BONGO
		if (app_use_tpl == true) { /* If the flag for the app's use of the trampoline page is set, modify the last two bytes */
			word tpl = CalculateTrampoline(status_.bootloader_start, payload[1] | payload[0]);
			if (i == (payload_size - 2)) {
				data_packet[packet] = (byte)(tpl & 0xFF);
				USE_SERIAL.printf_P("\n\r[%s] KATSULO %d ...\n\n\r", __func__, i);
			}
			if (i == (payload_size - 1)) {
				data_packet[packet] = (byte)((tpl >> 8) & 0xFF);
				USE_SERIAL.printf_P("\n\r[%s] KOKOA %d ...\n\n\r", __func__, i);
			}
		}

		if (packet++ == (TXDATASIZE - 1)) {					/* When a data packet is completed to be sent ... */
			for (int j = 0; j < TXDATASIZE; j++) {
				USE_SERIAL.printf_P(".");
			}
			upl_errors += WritePageBuff(data_packet);		/* Send data to T85 through I2C */
			packet = 0;
			delay(10);										/* ###### DELAY BETWEEN PACKETS SENT TO PAGE ###### */
		}
		if (upl_errors > 0) {
			//DeleteFlash();
			TwoStepInit(2000);
#if ESP8266
			ESP.restart();
#else
			resetFunc();
#endif /* ESP8266 */
			return(upl_errors);
		}
		if (page_end++ == (PAGE_SIZE - 1)) {					/* When a page end is detected ... */

			USE_SERIAL.printf_P(" P%d ", page_count);

			if ((status_.features_code & 0x08) != false) {		/* If CMD_STPGADDR is enabled in Timonel, add a   */
				delay(100);										/* 100 ms delay to allow memory flashing and set  */
				USE_SERIAL.printf_P("\n\r");					/* the next page address before sending new data. */
				SetPageAddress(start_address + (page_count * PAGE_SIZE));
			}

			delay(100);											/* ###### DELAY BETWEEN PAGE WRITINGS ... ###### */
			page_count++;

			if (i < (payload_size - 1)) {
				page_end = 0;
			}
		}
	}

	if (upl_errors == 0) {
		USE_SERIAL.printf_P("\n\r[%s] Application was successfully transferred to T85, please select 'run app' command to start it ...\n\r", __func__);
	}
	else {
		USE_SERIAL.printf_P("\n\r[%s] Communication errors detected during firmware transfer, please retry !!! ErrCnt: %d\n\r", __func__, upl_errors);
		//DeleteFlash();
		TwoStepInit(2000);
#if ESP8266
		ESP.restart();
#else
		resetFunc();
#endif /* ESP8266 */
	}
	return(upl_errors);
}

// Function SetPageAddress
byte Timonel::SetPageAddress(word page_addr) {
	const byte cmd_size = 4;
	const byte reply_size = 2;
	byte twi_cmd_arr[cmd_size] = { STPGADDR, 0, 0, 0 };
	byte twi_reply_arr[reply_size];
	twi_cmd_arr[1] = ((page_addr & 0xFF00) >> 8);					/* Flash page address MSB */
	twi_cmd_arr[2] = (page_addr & 0xFF);							/* Flash page address LSB */
	twi_cmd_arr[3] = (byte)(twi_cmd_arr[1] + twi_cmd_arr[2]);		/* Checksum */
	//USE_SERIAL.printf_P("\n\n\r[%s] Setting flash page address on ATTiny85 >>> %d (STPGADDR)\n\r", __func__, twi_cmd_arr[0]);
	byte twi_cmd_err = TwiCmdXmit(twi_cmd_arr, cmd_size, AKPGADDR, twi_reply_arr, reply_size);
	if (twi_cmd_err == 0) {
		//USE_SERIAL.printf_P("[%s] Command %d parsed OK <<< %d\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
		if (twi_reply_arr[1] == twi_cmd_arr[3]) {
			//USE_SERIAL.printf_P("[%s] Operands %d and %d parsed OK by slave <<< ATtiny85 Flash Page Address Check = %d\n\r", __func__, twi_cmd_arr[1], twi_cmd_arr[2], twi_reply_arr[1]);
			USE_SERIAL.printf_P("[%s] Address %04X (%02X) (%02X) parsed OK by ATTiny85 <<< Check = %d\n\r", __func__, page_addr, twi_cmd_arr[1], twi_cmd_arr[2], twi_reply_arr[1]);
		}
		else {
			USE_SERIAL.printf_P("[%s] Operand %d parsed with {{{ERROR}}} <<< ATTiny85 Check = %d\r\n", __func__, twi_cmd_arr[1], twi_reply_arr[1]);
		}
	}
	else {
		USE_SERIAL.printf_P("[%s] Error parsing %d command! <<< %d\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
	}
	return(twi_cmd_err);
}

// Function FillSpecialPage
byte Timonel::FillSpecialPage(byte page_type, byte app_reset_msb, byte app_reset_lsb) {
	word address = 0x0000;
	//byte special_page[64] = { 0xFF };
	byte special_page[64] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};
	byte packet = 0;										/* Byte amount to be sent in a single I2C data packet */
	byte data_packet[TXDATASIZE] = { 0xFF };
	// Function mode: 1=reset vector page, 2=trampoline page
	switch (page_type) {
		case 1: {	/* Reset Vector Page (0) */
			special_page[0] = (0xC0 + ((((status_.bootloader_start / 2) - 1) >> 8) & 0xFF));
			special_page[1] = (((status_.bootloader_start / 2) - 1) & 0xFF);
		break;
		}
		case 2: {	/* Trampoline Page (Timonel start - 64)*/
			//address = 0xE00; //status_.bootloader_start - PAGE_SIZE;
			address = status_.bootloader_start - (PAGE_SIZE);
			//word tpl = (((~((status_.bootloader_start >> 1) - ((((app_reset_msb << 8) | app_reset_lsb) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
			word tpl = CalculateTrampoline(status_.bootloader_start, ((app_reset_msb << 8) | app_reset_lsb));
			special_page[PAGE_SIZE - 1] = (byte)((tpl >> 8) & 0xFF);
			special_page[PAGE_SIZE - 2] = (byte)(tpl & 0xFF);
			break;
		}
		default: {
			// ---
			break;	
		}
	}
	byte twi_errors = SetPageAddress(address);
	delay(100);
	for (byte i = 0; i < PAGE_SIZE; i++) {
		data_packet[packet] = special_page[i];
		if (packet++ == (TXDATASIZE - 1)) {
			for (int j = 0; j < TXDATASIZE; j++) {
				USE_SERIAL.printf_P("|");
			}
			twi_errors += WritePageBuff(data_packet);		/* Send data to T85 through I2C */
			packet = 0;
			delay(10);	
		}
	}
	USE_SERIAL.printf_P(" [Special page type %d]\n\n\r", page_type);
	delay(100);
}

// Ask Timonel to stop executing and run the user application
byte Timonel::RunApplication(void) {
	USE_SERIAL.printf_P("\n\r[%s] Exit bootloader & run application >>> %d\r\n", __func__, EXITTMNL);
	return(TwiCmdXmit(EXITTMNL, ACKEXITT));
}

// Instruct Timonel to delete the user application
byte Timonel::DeleteApplication(void) {
	USE_SERIAL.printf_P("\n\r[%s] Delete Flash Memory >>> %d\r\n", __func__, DELFLASH);
	return(TwiCmdXmit(DELFLASH, ACKDELFL));
}

// Function CalculateTrampoline
word Timonel::CalculateTrampoline(word bootloader_start, word application_start) {
	return(((~((bootloader_start >> 1) - ((application_start + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
}

// Function DumpFlashMem
byte Timonel::DumpMemory(word flash_size, byte rx_data_size, byte values_per_line) {
	if ((status_.features_code & 0x80) == false) {
		USE_SERIAL.printf_P("\n\r[%s] Function not implemented in current Timonel setup ...\r\n", __func__, DELFLASH);
		return(3);
	}
	const byte cmd_size = 5;
	byte twi_cmd_arr[cmd_size] = { READFLSH, 0, 0, 0, 0 };
	byte twi_reply_arr[rx_data_size + 2];
	byte checksum_errors = 0;
	int v = 1;
	twi_cmd_arr[3] = rx_data_size;
	USE_SERIAL.printf_P("\n\r[%s] Dumping Flash Memory ...\n\n\r", __func__);
	USE_SERIAL.printf_P("Addr %04X: ", 0);
	for (word address = 0; address < flash_size; address += rx_data_size) {
		twi_cmd_arr[1] = ((address & 0xFF00) >> 8);			/* Flash page address high byte */
		twi_cmd_arr[2] = (address & 0xFF);					/* Flash page address low byte */
		twi_cmd_arr[4] = (byte)(twi_cmd_arr[0] + twi_cmd_arr[1] + twi_cmd_arr[2] + twi_cmd_arr[3]); /* READFLSH Checksum */
		byte twi_cmd_err = TwiCmdXmit(twi_cmd_arr, cmd_size, ACKRDFSH, twi_reply_arr, rx_data_size + 2);
		if (twi_cmd_err == 0) {
			byte checksum = 0;
			for (byte i = 1; i < (rx_data_size + 1); i++) {
				USE_SERIAL.printf_P("%02X", twi_reply_arr[i]);							/* Memory values */
				if (v == values_per_line) {
					USE_SERIAL.printf_P("\n\r");
					if ((address + rx_data_size) < flash_size) {
						USE_SERIAL.printf_P("Addr %04X: ", address + rx_data_size);		/* Page address */
					}
					v = 0;
				}
				else {
					USE_SERIAL.printf_P(" ");											/* Space between values */
				}
				v++;
				checksum += (byte)twi_reply_arr[i];
			}
			if (checksum != twi_reply_arr[rx_data_size + 1]) {
				USE_SERIAL.printf_P("\n\r   ### Checksum ERROR! ###   %d\n\r", checksum);
				//USE_SERIAL.printf_P("%d\n\r", checksum + 1);
				//USE_SERIAL.printf_P(" <-- calculated, received --> %d\n\r", twi_reply_arr[rx_data_size + 1]);
				if (checksum_errors++ == MAXCKSUMERRORS) {
					USE_SERIAL.printf_P("[%s] Too many Checksum ERRORS, aborting! \n\r", __func__);
					delay(1000);
					return(2);
				}
			}
		}
		else {
			USE_SERIAL.printf_P("[%s] DumpFlashMem Error parsing %d command <<< %d\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
			return(1);
		}
		delay(100);
	}
	return(0);
}

// Function TWI command transmit (Overload A: transmit single byte command)
byte Timonel::TwiCmdXmit(byte twi_cmd, byte twi_reply, byte twi_reply_arr[], byte reply_size) {
	const byte cmd_size = 1;
	byte twi_cmd_arr[cmd_size] = { twi_cmd };
	return(TwiCmdXmit(twi_cmd_arr, cmd_size, twi_reply, twi_reply_arr, reply_size));
}

// Function TWI command transmit (Overload B: transmit multibyte command)
byte Timonel::TwiCmdXmit(byte twi_cmd_arr[], byte cmd_size, byte twi_reply, byte twi_reply_arr[], byte reply_size) {
	for (int i = 0; i < cmd_size; i++) {
		Wire.beginTransmission(addr_);
		Wire.write(twi_cmd_arr[i]);
		Wire.endTransmission();
	}
	// Receive reply
	if (reply_size == 0) {
		Wire.requestFrom(addr_, ++reply_size);
		byte reply = Wire.read();
		if (reply == twi_reply) {						/* I2C reply from slave */
			USE_SERIAL.printf_P("[%s] Command %d parsed OK <<< %d\n\n\r", __func__, twi_cmd_arr[0], reply);
			return(0);
		}
		else {
			USE_SERIAL.printf_P("[%s] Error parsing %d command <<< %d\n\n\r", __func__, twi_cmd_arr[0], reply);
			return(1);
		}
	}
	else {
		byte reply_length = Wire.requestFrom(addr_, reply_size);
  		for (int i = 0; i < reply_size; i++) {
	    	twi_reply_arr[i] = Wire.read();
  		}
	 	if ((twi_reply_arr[0] == twi_reply) && (reply_length == reply_size)) {
			//USE_SERIAL.printf_P("[%s] Multibyte command %d parsed OK <<< %d\n\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
			return(0);
		}
		else {
			USE_SERIAL.printf_P("[%s] Error parsing %d multibyte command <<< %d\n\n\r", __func__, twi_cmd_arr[0], twi_reply_arr[0]);
			return(2);
		}		  
	}
}
