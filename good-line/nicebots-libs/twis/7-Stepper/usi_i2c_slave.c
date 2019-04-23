/*-----------------------------------------------------*\
|  USI I2C Slave Driver                                 |
|                                                       |
| This library provides a robust, interrupt-driven I2C  |
| slave implementation built on the ATTiny Universal    |
| Serial Interface (USI) hardware.  Slave operation is  |
| implemented as a register bank, where each 'register' |
| is a pointer to an 8-bit variable in the main code.   |
| This was chosen to make I2C integration transparent   |
| to the mainline code and making I2C reads simple.     |
| This library also works well with the Linux I2C-Tools |
| utilities i2cdetect, i2cget, i2cset, and i2cdump.     |
|                                                       |
| Adam Honse (GitHub: CalcProgrammer1) - 7/29/2012      |
|            -calcprogrammer1@gmail.com                 |
\*-----------------------------------------------------*/

#include "usi_i2c_slave.h"

char usi_i2c_slave_internal_address;
char usi_i2c_slave_address;
char usi_i2c_mode;

///////////////////////////////////////////////////////////////////////////////////////////////////
////USI Slave States///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#define USI_SLAVE_REGISTER_COUNT 8

// The I2C register file is stored as an array of pointers, point these to whatever your I2C registers
// need to read/write in your code.  This abstracts the buffer and makes it easier to write directly
// to values in your code.
char* USI_Slave_register_buffer[USI_SLAVE_REGISTER_COUNT];
char  USI_Slave_internal_address = 0;
char  USI_Slave_internal_address_set = 0;

enum

{

  USI_SLAVE_CHECK_ADDRESS,
  USI_SLAVE_SEND_DATA,
  USI_SLAVE_SEND_DATA_ACK_WAIT,
  USI_SLAVE_SEND_DATA_ACK_CHECK,

  USI_SLAVE_RECV_DATA_WAIT,

  USI_SLAVE_RECV_DATA_ACK_SEND

} USI_I2C_Slave_State;

/////////////////////////////////////////////////
////USI Register Setup Values////////////////////
/////////////////////////////////////////////////

#define USI_SLAVE_COUNT_ACK_USISR			0b01110000 | (0x0E << USICNT0)	//Counts one clock (ACK)
#define USI_SLAVE_COUNT_BYTE_USISR			0b01110000 | (0x00 << USICNT0)	//Counts 8 clocks (BYTE)
#define USI_SLAVE_CLEAR_START_USISR			0b11110000 | (0x00 << USICNT0)  //Clears START flag
#define USI_SLAVE_SET_START_COND_USISR		0b01110000 | (0x00 << USICNT0)
#define USI_SLAVE_SET_START_COND_USICR		0b10101000
#define USI_SLAVE_STOP_DID_OCCUR_USICR		0b10111000
#define USI_SLAVE_STOP_NOT_OCCUR_USICR		0b11101000

/////////////////////////////////////////////////
////USI Direction Macros/////////////////////////
/////////////////////////////////////////////////

#define USI_SET_SDA_OUTPUT()	{ DDR_USI |=  (1 << PORT_USI_SDA); }
#define USI_SET_SDA_INPUT() 	{ DDR_USI &= ~(1 << PORT_USI_SDA); }

#define USI_SET_SCL_OUTPUT()	{ DDR_USI |=  (1 << PORT_USI_SCL); }
#define USI_SET_SCL_INPUT() 	{ DDR_USI &= ~(1 << PORT_USI_SCL); }

#define USI_SET_BOTH_OUTPUT()	{ DDR_USI |= (1 << PORT_USI_SDA) | (1 << PORT_USI_SCL); }
#define USI_SET_BOTH_INPUT() 	{ DDR_USI &= ~((1 << PORT_USI_SDA) | (1 << PORT_USI_SCL)); }

////////////////////////////////////////////////////////////////////////////////////////////////////

void USI_I2C_Init(char address)
{
	PORT_USI &= ~(1 << PORT_USI_SCL);
	PORT_USI &= ~(1 << PORT_USI_SDA);

	usi_i2c_slave_address = address;

	USI_SET_BOTH_INPUT();
	
	USICR = (1 << USISIE) | (0 << USIOIE) | (1 << USIWM1) | (0 << USIWM0) | (1 << USICS1) | (0 << USICS0) | (0 << USICLK) | (0 << USITC);
	USISR = (1 << USISIF) | (1 << USIOIF) | (1 << USIPF) | (1 << USIDC);
}

/////////////////////////////////////////////////////////////////////////////////
// ISR USI_START_vect - USI Start Condition Detector Interrupt                 //
//                                                                             //
//  This interrupt occurs when the USI Start Condition Detector detects a      //
//  start condition.  A start condition marks the beginning of an I2C          //
//  transmission and occurs when SDA has a high->low transition followed by an //
//  SCL high->low transition.  When a start condition occurs, the I2C slave    //
//  state is set to check address mode and the counter is set to wait 8 clocks //
//  (enough for the address/rw byte to be transmitted) before overflowing and  //
//  triggering the first state table interrupt.  If a stop condition occurs,   //
//  reset the start condition detector to detect the next start condition.     //
/////////////////////////////////////////////////////////////////////////////////

ISR(USI_START_vect)

{

	USI_I2C_Slave_State = USI_SLAVE_CHECK_ADDRESS;


	USI_SET_SDA_INPUT();



	// wait for SCL to go low to ensure the Start Condition has completed (the

	// start detector will hold SCL low ) - if a Stop Condition arises then leave

	// the interrupt to prevent waiting forever - don't use USISR to test for Stop

	// Condition as in Application Note AVR312 because the Stop Condition Flag is

	// going to be set from the last TWI sequence

	while((PIN_USI & (1 << PIN_USI_SCL)) && !((PIN_USI & (1 << PIN_USI_SDA))));



	if(!(PIN_USI & (1 << PIN_USI_SDA)))

	{

		// a Stop Condition did not occur

		USICR = USI_SLAVE_STOP_NOT_OCCUR_USICR;

	}

	else

	{
		// a Stop Condition did occur

    	USICR = USI_SLAVE_STOP_DID_OCCUR_USICR;

	}



	USISR = USI_SLAVE_CLEAR_START_USISR;

}

/////////////////////////////////////////////////////////////////////////////////
// ISR USI_OVERFLOW_vect - USI Overflow Interrupt                              //
//                                                                             //
//  This interrupt occurs when the USI counter overflows.  By setting this     //
//  counter to 8, the USI can be commanded to wait one byte length before      //
//  causing another interrupt (and thus state change).  To wait for an ACK,    //
//  set the counter to 1 (actually -1, or 0x0E) it will wait one clock.        //
//  This is used to set up a state table of I2C transmission states that fits  //
//  the I2C protocol for proper transmission.                                  //
/////////////////////////////////////////////////////////////////////////////////

ISR(USI_OVERFLOW_vect)

{
	switch (USI_I2C_Slave_State)

	{
		/////////////////////////////////////////////////////////////////////////
		// Case USI_SLAVE_CHECK_ADDRESS                                        //
		//                                                                     //
		//  The first state after the start condition, this state checks the   //
		//  received byte against the stored slave address as well as the      //
		//  global transmission address of 0x00.  If there is a match, the R/W //
		//  bit is checked to branch either to sending or receiving modes.     //
		//  If the address was not for this device, the USI system is          //
		//  re-initialized for start condition.                                //
		/////////////////////////////////////////////////////////////////////////

		case USI_SLAVE_CHECK_ADDRESS:


			if((USIDR == 0) || ((USIDR >> 1) == usi_i2c_slave_address))

			{				
				if (USIDR & 0x01)

				{
					USI_I2C_Slave_State = USI_SLAVE_SEND_DATA;

				}

				else

				{
					USI_Slave_internal_address_set = 0;
					USI_I2C_Slave_State = USI_SLAVE_RECV_DATA_WAIT;

				}

				//Set USI to send ACK

				USIDR = 0;

				USI_SET_SDA_OUTPUT();

				USISR = USI_SLAVE_COUNT_ACK_USISR;
			}

			else

			{
				//Set USI to Start Condition Mode

				USICR = USI_SLAVE_SET_START_COND_USICR;

				USISR = USI_SLAVE_SET_START_COND_USISR;
			}

			break;

		/////////////////////////////////////////////////////////////////////////
		// Case USI_SLAVE_SEND_DATA_ACK_WAIT                                   //
		//                                                                     //
		//  Wait 1 clock period for the master to ACK or NACK the sent data	   //
		//  If master NACK's, it means that master doesn't want any more data. //
		/////////////////////////////////////////////////////////////////////////
		case USI_SLAVE_SEND_DATA_ACK_WAIT:

			//After sending, immediately shut off PORT = 1 to prevent driving
			//the line high (I2C should *NEVER* drive high, and could damage
			//connected devices if operating at different voltage levels)
			PORT_USI &= ~(1 << PORT_USI_SDA);


			USI_I2C_Slave_State = USI_SLAVE_SEND_DATA_ACK_CHECK;
			USI_SET_SDA_INPUT();
			USISR = USI_SLAVE_COUNT_ACK_USISR;
			break;

		/////////////////////////////////////////////////////////////////////////
		// Case USI_SLAVE_SEND_DATA_ACK_CHECK                                  //
		//                                                                     //
		//  Check USIDR to see if master sent ACK or NACK.  If NACK, set up    //
		//  a reset to START conditions, if ACK, fall through into SEND_DATA   //
		//  to continue sending data.                                          //
		/////////////////////////////////////////////////////////////////////////
		case USI_SLAVE_SEND_DATA_ACK_CHECK:
			
			if(USIDR)
			{
				//The master sent a NACK, indicating that it will not accept
				//more data.  Reset into START condition state
				USICR = USI_SLAVE_SET_START_COND_USICR;
				USISR = USI_SLAVE_SET_START_COND_USISR;
				return;
			}
			//else: fall through into SEND_DATA

		/////////////////////////////////////////////////////////////////////////
		// Case USI_SLAVE_SEND_DATA                                            //
		//                                                                     //
		//  Set USIDR to the data to be sent, then set up SDA registers to     //
		//  enable data transmission in the next 8 clocks.  Set to wait 8      //
		//  clocks and proceed to wait for ACK.                                //
		/////////////////////////////////////////////////////////////////////////
		case USI_SLAVE_SEND_DATA:

			if(USI_Slave_internal_address <= USI_SLAVE_REGISTER_COUNT)
			{
				USIDR = *(USI_Slave_register_buffer[USI_Slave_internal_address]);
			}
			else
			{
				USIDR = 0x00;
			}
			USI_Slave_internal_address++;


			USI_I2C_Slave_State = USI_SLAVE_SEND_DATA_ACK_WAIT;


			//To send data, DDR for SDA must be 1 (Output) and PORT for SDA
			//must also be 1 (line drives low on USIDR MSB = 0 or PORT = 0)

			USI_SET_SDA_OUTPUT();
			PORT_USI |= (1 << PORT_USI_SDA);

			USISR = USI_SLAVE_COUNT_BYTE_USISR;

			break;


		/////////////////////////////////////////////////////////////////////////
		// Case USI_SLAVE_RECV_DATA_WAIT                                       //
		//                                                                     //
		//  Prepares to wait 8 clocks to receive a data byte from the master.  //
		/////////////////////////////////////////////////////////////////////////

		case USI_SLAVE_RECV_DATA_WAIT:

			USI_I2C_Slave_State = USI_SLAVE_RECV_DATA_ACK_SEND;

			USI_SET_SDA_INPUT();

			USISR = USI_SLAVE_COUNT_BYTE_USISR;

			break;


		/////////////////////////////////////////////////////////////////////////
		// Case USI_SLAVE_RECV_DATA_ACK_SEND                                   //
		//                                                                     //
		//  After waiting for the master to finish transmission, this reads    //
		//  USIDR into either the i2c buffer or internal address, then sends   //
		//  an acknowledgement to the master.                                  //
		/////////////////////////////////////////////////////////////////////////

		case USI_SLAVE_RECV_DATA_ACK_SEND:



			USI_I2C_Slave_State = USI_SLAVE_RECV_DATA_WAIT;

			
			if(USI_Slave_internal_address_set == 0)
			{
				USI_Slave_internal_address = USIDR;
				USI_Slave_internal_address_set = 1;
			}
			else if(USI_Slave_internal_address <= USI_SLAVE_REGISTER_COUNT)
			{
				*(USI_Slave_register_buffer[USI_Slave_internal_address]) = USIDR;
			}
			
			USIDR = 0;

			USI_SET_SDA_OUTPUT();

			USISR = USI_SLAVE_COUNT_ACK_USISR;

			break;

	}

}
