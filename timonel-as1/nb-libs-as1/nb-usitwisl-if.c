/*
* Atmel Corporation
*
* File              : USI_TWI_Slave.c
* Compiler          : IAR EWAAVR 4.11A
* Revision          : $Revision: 1.14 $
* Date              : $Date: Friday, December 09, 2005 17:25:38 UTC $
* Updated by        : $Author: jtyssoe $
*
* Support mail      : avr@atmel.com
*
* Supported devices : All device with USI module can be used.
*
* AppNote           : AVR312 - Using the USI module as a I2C slave
*
* Description       : Functions for USI_TWI_receiver and USI_TWI_transmitter.
*
* Modified by Jim Gallt for compatibility with avr-gcc
* Added USI_TWI_On_Request
* February, 2012
*
****************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "nb-usitwisl-if.h"

// Static Variables
static uint8_t TWI_slaveAddress;
static volatile uint8_t USI_TWI_Overflow_State;

// Local variables
static uint8_t TWI_RxBuf[TWI_RX_BUFFER_SIZE];
static volatile uint8_t TWI_RxHead;
static volatile uint8_t TWI_RxTail;

static uint8_t TWI_TxBuf[TWI_TX_BUFFER_SIZE];
static volatile uint8_t TWI_TxHead;
static volatile uint8_t TWI_TxTail;

static void (*USI_TWI_On_Request)( void );

// ------------------------------------------------------
// Functions implemented as macros
#define SET_USI_TO_SEND_ACK() {  \
    USIDR = 0; /* Prepare ACK */ \
    DDR_USI |= (1<<PORT_USI_SDA); /* Set SDA as output*/ \
    USISR = (0<<USI_START_COND_INT)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)| /* Clear all flags, except Start Cond  */ \
      (0x0E<<USICNT0); /* set USI counter to shift 1 bit. */ \
    }

#define SET_USI_TO_READ_ACK() { \
    DDR_USI &= ~(1<<PORT_USI_SDA); /* Set SDA as input */  \
    USIDR = 0; /* Prepare ACK */ \
    USISR = (0<<USI_START_COND_INT)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)| /* Clear all flags, except Start Cond */ \
      (0x0E<<USICNT0); /* set USI counter to shift 1 bit. */ \
    }

#define SET_USI_TO_TWI_START_CONDITION_MODE() { \
    USICR = (1<<USISIE)|(0<<USIOIE)|  /* Enable Start Condition Interrupt. Disable Overflow Interrupt.*/  \
      (1<<USIWM1)|(0<<USIWM0)| /* Set USI in Two-wire mode. No USI Counter overflow hold. */  \
      (1<<USICS1)|(0<<USICS0)|(0<<USICLK)| /* Shift Register Clock Source = External, positive edge */   \
      (0<<USITC); \
    USISR = (0<<USI_START_COND_INT)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)| /* Clear all flags, except Start Cond */  \
      (0x0<<USICNT0); \
}

#define SET_USI_TO_SEND_DATA() { \
    DDR_USI |=  (1<<PORT_USI_SDA); /* Set SDA as output */ \
    USISR    =  (0<<USI_START_COND_INT)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)| /* Clear all flags, except Start Cond */ \
      (0x0<<USICNT0); /* set USI to shift out 8 bits */ \
    }

#define SET_USI_TO_READ_DATA() { \
    DDR_USI &= ~(1<<PORT_USI_SDA); /* Set SDA as input*/ \
    USISR    =  (0<<USI_START_COND_INT)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)| /* Clear all flags, except Start Cond */ \
      (0x0<<USICNT0); /* set USI to shift out 8 bits */ \
    }

// Attaches a user function that is called when master requests data from slave
void USI_TWI_Attach_On_Request( void (*usrfunction)( void )) {
  USI_TWI_On_Request = usrfunction;
}

// Flushes the TWI buffers
void Flush_TWI_Buffers(void) // change to static?
{
  TWI_RxTail = 0;
  TWI_RxHead = 0;
  TWI_TxTail = 0;
  TWI_TxHead = 0;
}

//********** USI_TWI functions **********//

// Initialise USI for TWI Slave mode.
void USI_TWI_Slave_Initialise( uint8_t TWI_ownAddress ) {
  Flush_TWI_Buffers();
  USI_TWI_On_Request = 0;  // set to null initially
  TWI_slaveAddress = TWI_ownAddress;

  DDR_USI |= ( 1 << PORT_USI_SCL ) | ( 1 << PORT_USI_SDA ); // set SDA and SCL as output
  PORT_USI |= (1<<PORT_USI_SCL); // Set SCL high
  PORT_USI |= (1<<PORT_USI_SDA); // Set SDA high
  //  DDR_USI |= (1<<PORT_USI_SCL); // Set SCL as output
  DDR_USI &= ~(1<<PORT_USI_SDA); // Set SDA as input
  USICR = (1<<USISIE)|(0<<USIOIE)| // Enable Start Condition Interrupt. Disable Overflow Interrupt.
      (1<<USIWM1)|(0<<USIWM0)| // Set USI in Two-wire mode. No USI Counter overflow prior
      // to first Start Condition (potentail failure)
      (1<<USICS1)|(0<<USICS0)|(0<<USICLK)| // Shift Register Clock Source = External, positive edge
      (0<<USITC);
  // clear all flags and reset overflow counter
  USISR = (1<<USI_START_COND_INT) | (1<<USIOIF) | (1<<USIPF) | (1<<USIDC);
}

// Puts data in the transmission buffer, Waits if buffer is full.
void USI_TWI_Transmit_Byte( uint8_t data ) {
  uint8_t tmphead;
  tmphead = ( TWI_TxHead + 1 ) & TWI_TX_BUFFER_MASK; // Calculate buffer index.
  while ( tmphead == TWI_TxTail ); // Wait for free space in buffer.
  TWI_TxBuf[tmphead] = data; // Store data in buffer.
  TWI_TxHead = tmphead; // Store new index.
}

// Returns a byte from the receive buffer. Waits if buffer is empty.
uint8_t USI_TWI_Receive_Byte( void ) {
  uint8_t tmptail;
  while ( TWI_RxHead == TWI_RxTail );
  tmptail = ( TWI_RxTail + 1 ) & TWI_RX_BUFFER_MASK; // Calculate buffer index
  TWI_RxTail = tmptail; // Store new index
  return TWI_RxBuf[tmptail]; // Return data from the buffer.
}

// Check if there is data in the receive buffer.
bool USI_TWI_Data_In_Receive_Buffer( void ) {
  return ( TWI_RxHead != TWI_RxTail ); // Return 0 (FALSE) if the receive buffer is empty.
}

// ---------------------------------------------------------------
/* Function UsiStartHandler (Interrupt-like function)
 * Detects the USI_TWI Start Condition and intialises the USI
 * for reception of the "TWI Address" packet.
 */
void UsiStartHandler(void) {
  // Set default starting conditions for new TWI package
  USI_TWI_Overflow_State = USI_SLAVE_CHECK_ADDRESS;
  DDR_USI  &= ~(1<<PORT_USI_SDA); // Set SDA as input

  // Wait for SCL to go low to ensure the "Start Condition" has completed.
  // Don't use USISR (per AVR312) because stop condition flag is going to be set
  // from the last TWI sequence.  (Donald Blake figured this out -- thanks).
  while ( (PIN_USI & (1<<PORT_USI_SCL)) && (!(PIN_USI & (1<<PORT_USI_SDA))) );

  // check to see if a stop condition has arisen
  if( PIN_USI & (1<<PIN_USI_SDA)) { // SDA pin high means stop condition has arisen
    USICR   =   (1<<USISIE)|(0<<USIOIE)| // disable Overflow, enable Start Condition Interrupt
        (1<<USIWM1)|(0<<USIWM0)| // Set USI in Two-wire mode, no USI counter overflow hold
        (1<<USICS1)|(0<<USICS0)|(0<<USICLK)| // Shift Register Clock Source = External, positive edge
        (0<<USITC);
  }
  else { // no stop condition has arisen
    USICR   =   (1<<USISIE)|(1<<USIOIE)| // Enable Overflow and Start Condition Interrupt. (Keep StartCondInt to detect RESTART)
        (1<<USIWM1)|(1<<USIWM0)| // Set USI in Two-wire mode.
        (1<<USICS1)|(0<<USICS0)|(0<<USICLK)| // Shift Register Clock Source = External, positive edge
        (0<<USITC);
  }

  USISR  =    (1<<USI_START_COND_INT)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)| // Clear flags
      (0x0<<USICNT0);  // Set USI to sample 8 bits i.e. count 16 external pin toggles.
} // end ISR

// ------------------------------------------------------------
/* Function UsiOverflowHandler (Interrupt-like function)
 * Handles all the communication. Is disabled only when waiting
 * for new Start Condition.
 */
void UsiOverflowHandler(void) {
  switch (USI_TWI_Overflow_State)
  {
  // ---------- Address mode ----------
  // Check address and send ACK (and next USI_SLAVE_SEND_DATA) if OK, else reset USI.
  case USI_SLAVE_CHECK_ADDRESS:
    if ((USIDR == 0) || (( USIDR>>1 ) == TWI_slaveAddress)) { // match address or all call
      if ( USIDR & 0x01 ) // direction bit = READ (high)
        USI_TWI_Overflow_State = USI_SLAVE_SEND_DATA;
      else // direction bit = WRITE (low)
        USI_TWI_Overflow_State = USI_SLAVE_REQUEST_DATA;
      SET_USI_TO_SEND_ACK();
    }
    else { // no address match so reset USI
      SET_USI_TO_TWI_START_CONDITION_MODE();
    }
    break;

    // ----- Master write data mode ------
    // Check reply and goto USI_SLAVE_SEND_DATA if OK, else reset USI.
  case USI_SLAVE_CHECK_REPLY_FROM_SEND_DATA:
    if ( USIDR ) // If NACK, the master does not want more data.
    {
      SET_USI_TO_TWI_START_CONDITION_MODE();
      return;
    }
    // From here we just drop straight into USI_SLAVE_SEND_DATA if the master sent an ACK

    // ------ Slave respond to request from master
    // Copy data from buffer to USIDR and set USI to shift byte. Next USI_SLAVE_REQUEST_REPLY_FROM_SEND_DATA
  case USI_SLAVE_SEND_DATA: // direction bit = READ (send data from slave to master)
    if( USI_TWI_On_Request ) { // call user function
      TWI_TxHead = TWI_TxTail = 0; // flush transmit buffer
      USI_TWI_On_Request(); // put data to be sent in TX buffer
    }
    // Put data from buffer in USI data register for send
    if ( TWI_TxHead != TWI_TxTail ) {
      TWI_TxTail = ( TWI_TxTail + 1 ) & TWI_TX_BUFFER_MASK;
      USIDR = TWI_TxBuf[TWI_TxTail];
    }
    else { // If the buffer is empty then:
      SET_USI_TO_TWI_START_CONDITION_MODE();
      return;
    }
    USI_TWI_Overflow_State = USI_SLAVE_REQUEST_REPLY_FROM_SEND_DATA;
    SET_USI_TO_SEND_DATA();
    break;

    // --------------------
    // Set USI to sample reply from master. Next USI_SLAVE_CHECK_REPLY_FROM_SEND_DATA
  case USI_SLAVE_REQUEST_REPLY_FROM_SEND_DATA:
    USI_TWI_Overflow_State = USI_SLAVE_CHECK_REPLY_FROM_SEND_DATA;
    SET_USI_TO_READ_ACK();
    break;

    // ----- Master read data mode ------
    // Set USI to sample data from master. Next USI_SLAVE_GET_DATA_AND_SEND_ACK.
  case USI_SLAVE_REQUEST_DATA: // direction bit = WRITE (slave read data from master)
    USI_TWI_Overflow_State = USI_SLAVE_GET_DATA_AND_SEND_ACK;
    SET_USI_TO_READ_DATA();
    break;

    // --------------------
    // Copy data from USIDR and send ACK. Next USI_SLAVE_REQUEST_DATA
  case USI_SLAVE_GET_DATA_AND_SEND_ACK:
    // Put data into Buffer
    TWI_RxHead = ( TWI_RxHead + 1 ) & TWI_RX_BUFFER_MASK;
    TWI_RxBuf[TWI_RxHead] = USIDR;

    USI_TWI_Overflow_State = USI_SLAVE_REQUEST_DATA;
    SET_USI_TO_SEND_ACK();
    break;
  }
}
