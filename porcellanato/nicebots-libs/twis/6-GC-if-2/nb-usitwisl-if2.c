/*
 *  Timonel USI TWI Interrupt-Free Driver
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: nb-usitwisl.c (Driver library)
 *  ........................................... 
 *  Version: 1.0 / 2019-04-19
 *  gustavo.casanova@nicebots.com
 *  ...........................................
 */

// Includes
#include "nb-usitwisl-if2.h"

// Device Dependent Defines
#if defined(__AVR_ATtiny25__) | \
    defined(__AVR_ATtiny45__) | \
    defined(__AVR_ATtiny85__)
#define DDR_USI DDRB
#define PORT_USI PORTB
#define PIN_USI PINB
#define PORT_USI_SDA PB0
#define PORT_USI_SCL PB2
#define PIN_USI_SDA PINB0
#define PIN_USI_SCL PINB2
#define TWI_START_COND_FLAG	USISIF	/* This flag indicates that an I2C START condition occurred on the bus (can trigger an interrupt) */
#define USI_OVERFLOW_FLAG USIOIF	/* This flag indicates that the bits reception or transmission is complete (can trigger an interrupt) */
#define TWI_STOP_COND_FLAG USIPF	/* This flag indicates that an I2C STOP condition occurred on the bus */
#define TWI_COLLISION_FLAG USIDC	/* This flag indicates that a data output collision occurred on the bus */
#endif

// USI direction macros
#define USI_SET_SDA_OUTPUT()	{ DDR_USI |=  (1 << PORT_USI_SDA); }
#define USI_SET_SDA_INPUT() 	{ DDR_USI &= ~(1 << PORT_USI_SDA); }
#define USI_SET_SCL_OUTPUT()	{ DDR_USI |=  (1 << PORT_USI_SCL); }
#define USI_SET_SCL_INPUT() 	{ DDR_USI &= ~(1 << PORT_USI_SCL); }
#define USI_SET_BOTH_OUTPUT()	{ DDR_USI |= (1 << PORT_USI_SDA) | (1 << PORT_USI_SCL); }
#define USI_SET_BOTH_INPUT() 	{ DDR_USI &= ~((1 << PORT_USI_SDA) | (1 << PORT_USI_SCL)); }

// SET_USI_TO_SEND_ACK Macro Function
#define SET_USI_TO_SEND_ACK( ) \
{ \
  /* prepare ACK, ack is a zero */ \
  USIDR = 0; \
  /* set SDA as output */ \
  DDR_USI |= ( 1 << PORT_USI_SDA ); \
  /* clear all interrupt flags, except Start Cond */ \
  USISR = \
       ( 0 << TWI_START_COND_FLAG ) | \
       ( 1 << USI_OVERFLOW_FLAG ) | \
       ( 1 << TWI_STOP_COND_FLAG ) | \
       ( 1 << TWI_COLLISION_FLAG )| \
       /* set USI counter to shift 1 bit */ \
       ( 0x0E << USICNT0 ); \
}

// SET_USI_TO_READ_ACK Macro Function
#define SET_USI_TO_READ_ACK( ) \
{ \
  /* set SDA as input */ \
  DDR_USI &= ~( 1 << PORT_USI_SDA ); \
  /* prepare ACK */ \
  USIDR = 0; \
  /* clear all interrupt flags, except Start Cond */ \
  USISR = \
       ( 0 << TWI_START_COND_FLAG ) | \
       ( 1 << USI_OVERFLOW_FLAG ) | \
       ( 1 << TWI_STOP_COND_FLAG ) | \
       ( 1 << TWI_COLLISION_FLAG ) | \
       /* set USI counter to shift 1 bit */ \
       ( 0x0E << USICNT0 ); \
}

// SET_USI_TO_TWI_START_CONDITION_MODE Macro Function
#define SET_USI_TO_TWI_START_CONDITION_MODE( ) \
{ \
  USICR = \
       /* enable Start Condition Interrupt, disable Overflow Interrupt */ \
       ( 1 << USISIE ) | ( 0 << USIOIE ) | \
       /* set USI in Two-wire mode, no USI Counter overflow hold */ \
       ( 1 << USIWM1 ) | ( 0 << USIWM0 ) | \
       /* Shift Register Clock Source = External, positive edge */ \
       /* 4-Bit Counter Source = external, both edges */ \
       ( 1 << USICS1 ) | ( 0 << USICS0 ) | ( 0 << USICLK ) | \
       /* no toggle clock-port pin */ \
       ( 0 << USITC ); \
  USISR = \
        /* clear all interrupt flags, except Start Cond */ \
        ( 0 << TWI_START_COND_FLAG ) | ( 1 << USI_OVERFLOW_FLAG ) | ( 1 << TWI_STOP_COND_FLAG ) | \
        ( 1 << TWI_COLLISION_FLAG ) | ( 0x0 << USICNT0 ); \
}

// SET_USI_TO_SEND_DATA Macro Function
#define SET_USI_TO_SEND_DATA( ) \
{ \
  /* set SDA as output */ \
  DDR_USI |=  ( 1 << PORT_USI_SDA ); \
  /* clear all interrupt flags, except Start Cond */ \
  USISR    =  \
       ( 0 << TWI_START_COND_FLAG ) | ( 1 << USI_OVERFLOW_FLAG ) | ( 1 << TWI_STOP_COND_FLAG ) | \
       ( 1 << TWI_COLLISION_FLAG) | \
       /* set USI to shift out 8 bits */ \
       ( 0x0 << USICNT0 ); \
}

// SET_USI_TO_READ_DATA Macro Function
#define SET_USI_TO_READ_DATA( ) \
{ \
  /* set SDA as input */ \
  DDR_USI &= ~( 1 << PORT_USI_SDA ); \
  /* clear all interrupt flags, except Start Cond */ \
  USISR    = \
       ( 0 << TWI_START_COND_FLAG ) | ( 1 << USI_OVERFLOW_FLAG ) | \
       ( 1 << TWI_STOP_COND_FLAG ) | ( 1 << TWI_COLLISION_FLAG ) | \
       /* set USI to shift out 8 bits */ \
       ( 0x0 << USICNT0 ); \
}

// USI_RECEIVE_CALLBACK Macro Function
#define USI_RECEIVE_CALLBACK() \
{ \
    if (Usi_onReceivePtr) \
    { \
        if (UsiTwiAmountDataInReceiveBuffer()) \
        { \
            Usi_onReceivePtr(UsiTwiAmountDataInReceiveBuffer()); \
        } \
    } \
}

// ONSTOP_USI_RECEIVE_CALLBACK Macro Function
#define ONSTOP_USI_RECEIVE_CALLBACK() \
    {                                 \
        if (USISR & (1 << TWI_STOP_COND_FLAG)) {   \
            USI_RECEIVE_CALLBACK();   \
        }                             \
    }

// USI_REQUEST_CALLBACK Macro Function
#define USI_REQUEST_CALLBACK()  \
{                               \
    USI_RECEIVE_CALLBACK();     \
    if(Usi_onRequestPtr) {      \
        Usi_onRequestPtr();     \
    }                           \
}    

// Data Type definitions
typedef enum {
    USI_SLAVE_CHECK_ADDRESS = 0x00,
    USI_SLAVE_SEND_DATA = 0x01,
    USI_SLAVE_REQUEST_REPLY_FROM_SEND_DATA = 0x02,
    USI_SLAVE_CHECK_REPLY_FROM_SEND_DATA = 0x03,
    USI_SLAVE_REQUEST_DATA = 0x04,
    USI_SLAVE_GET_DATA_AND_SEND_ACK = 0x05
} overflowState_t;

// Local variables
static uint8_t slaveAddress;
static volatile overflowState_t overflowState;

static uint8_t rxBuf[TWI_RX_BUFFER_SIZE];
static volatile uint8_t rxHead;
static volatile uint8_t rxTail;
static volatile uint8_t rxCount;

static uint8_t txBuf[TWI_TX_BUFFER_SIZE];
static volatile uint8_t txHead;
static volatile uint8_t txTail;
static volatile uint8_t txCount;

// Function FlushTwiBuffers
static void FlushTwiBuffers(void) {
    rxTail = 0;
    rxHead = 0;
    rxCount = 0;
    txTail = 0;
    txHead = 0;
    txCount = 0;
}

// Initialize USI for TWI slave mode

// Function UsiTwiSlaveInit
void UsiTwiSlaveInit(uint8_t twi_address) {
    FlushTwiBuffers();
    slaveAddress = twi_address;

    // In Two Wire mode (USIWM1, USIWM0 = 1X), the slave USI will pull SCL
    // low when a start condition is detected or a counter overflow (only
    // for USIWM1, USIWM0 = 11).  This inserts a wait state.  SCL is released
    // by the ISRs (USI_START_vect and USI_OVERFLOW_vect).

    // Set SCL and SDA as output
	USI_SET_BOTH_OUTPUT();

    // set SCL high
    PORT_USI |= (1 << PORT_USI_SCL);

    // set SDA high
    PORT_USI |= (1 << PORT_USI_SDA);

    // Set SDA as input
	USI_SET_SDA_INPUT();

    USICR =
        // enable Start Condition Interrupt
        (1 << USISIE) |
        // disable Overflow Interrupt
        (0 << USIOIE) |
        // set USI in Two-wire mode, no USI Counter overflow hold
        (1 << USIWM1) | (0 << USIWM0) |
        // Shift Register Clock Source = external, positive edge
        // 4-Bit Counter Source = external, both edges
        (1 << USICS1) | (0 << USICS0) | (0 << USICLK) |
        // no toggle clock-port pin
        (0 << USITC);

    // Clear all interrupt flags and reset overflow counter
    USISR = (1 << TWI_START_COND_FLAG) | (1 << USI_OVERFLOW_FLAG) | (1 << TWI_STOP_COND_FLAG) | (1 << TWI_COLLISION_FLAG);
}

// Function UsiTwiDataInTransmitBuffer
bool UsiTwiDataInTransmitBuffer(void) {
    // return 0 (false) if the receive buffer is empty
    return txCount;
}

// Function UsiTwiTransmitByte
void UsiTwiTransmitByte(uint8_t data) {

    // Wait for free space in buffer
    while (txCount == TWI_TX_BUFFER_SIZE) {
        // Nothing
    };

    // Store data in buffer
    txBuf[txHead] = data;
    txHead = (txHead + 1) & TWI_TX_BUFFER_MASK;
    txCount++;
}

// Function UsiTwiReceiveByte
uint8_t UsiTwiReceiveByte(void) {
    uint8_t rtn_byte;
    // wait for Rx data
    while (!rxCount) {
        // Nothing
    };

    rtn_byte = rxBuf[rxTail];
    // calculate buffer index
    rxTail = (rxTail + 1) & TWI_RX_BUFFER_MASK;
    rxCount--;

    // return data from the buffer.
    return rtn_byte;
}

// Function UsiTwiAmountDataInReceiveBuffer
uint8_t UsiTwiAmountDataInReceiveBuffer(void) {
    return rxCount;
}

// Function UsiStartHandler
// GC: Interrupt-like function
void UsiStartHandler(void) {
    // This triggers on second write, but claims to the callback there is only *one* byte in buffer
    //ONSTOP_USI_RECEIVE_CALLBACK();

    // This triggers on second write, but claims to the callback there is only *one* byte in buffer
    //USI_RECEIVE_CALLBACK();

    // set default starting conditions for new TWI package
    overflowState = USI_SLAVE_CHECK_ADDRESS;

    // set SDA as input
    DDR_USI &= ~(1 << PORT_USI_SDA);

    // wait for SCL to go low to ensure the Start Condition has completed (the
    // start detector will hold SCL low ) - if a Stop Condition arises then leave
    // the interrupt to prevent waiting forever - don't use USISR to test for Stop
    // Condition as in Application Note AVR312 because the Stop Condition Flag is
    // going to be set from the last TWI sequence
    while ( (PIN_USI & (1<<PORT_USI_SCL)) && (!(PIN_USI & (1<<PORT_USI_SDA))) );
        // SCL his high
        // (PIN_USI & (1 << PIN_USI_SCL)) &&
        // // and SDA is low
        // !((PIN_USI & (1 << PIN_USI_SDA))))
        // ;
        

    if (!(PIN_USI & (1 << PIN_USI_SDA))) {
        // a Stop Condition did not occur

        USICR =
            // keep Start Condition Interrupt enabled to detect RESTART
            (1 << USISIE) |
            // enable Overflow Interrupt
            (1 << USIOIE) |
            // set USI in Two-wire mode, hold SCL low on USI Counter overflow
            (1 << USIWM1) | (1 << USIWM0) |
            // Shift Register Clock Source = External, positive edge
            // 4-Bit Counter Source = external, both edges
            (1 << USICS1) | (0 << USICS0) | (0 << USICLK) |
            // no toggle clock-port pin
            (0 << USITC);
    } else {
        // a Stop Condition did occur

        USICR =
            // enable Start Condition Interrupt
            (1 << USISIE) |
            // disable Overflow Interrupt
            (0 << USIOIE) |
            // set USI in Two-wire mode, no USI Counter overflow hold
            (1 << USIWM1) | (0 << USIWM0) |
            // Shift Register Clock Source = external, positive edge
            // 4-Bit Counter Source = external, both edges
            (1 << USICS1) | (0 << USICS0) | (0 << USICLK) |
            // no toggle clock-port pin
            (0 << USITC);
    }

    USISR =
        // clear interrupt flags - resetting the Start Condition Flag will
        // release SCL
        (1 << TWI_START_COND_FLAG) | (1 << USI_OVERFLOW_FLAG) | (1 << TWI_STOP_COND_FLAG) | (1 << TWI_COLLISION_FLAG) |
        // set USI to sample 8 bits (count 16 external SCL pin toggles)
        (0x0 << USICNT0);

    USISR |= (1 << TWI_START_COND_FLAG); /* Reset the USI start flag in USISR register to prepare for new ints */
}

// Function UsiOverflowHandler
// GC: Interrupt-like function
void UsiOverflowHandler(void) {
    switch (overflowState) {
        // Address mode: check address and send ACK (and next USI_SLAVE_SEND_DATA) if OK,
        // else reset USI
        case USI_SLAVE_CHECK_ADDRESS: {
            // #############################################################
            // # GC: Disable this device answers to "general calls"
            // #############################################################
            // ****** if ( ( USIDR == 0 ) || ( ( USIDR >> 1 ) == slaveAddress) ) {
            if (((USIDR >> 1) & 0x3F) == slaveAddress) {
                if (USIDR & 0x01) {             /* If 1: Slave sends data to master */
                    USI_REQUEST_CALLBACK();
                    overflowState = USI_SLAVE_SEND_DATA;
                } else {                        /* If 0: Slave receives data from master */
                    overflowState = USI_SLAVE_REQUEST_DATA;
                }
                SET_USI_TO_SEND_ACK();
            }
            else {
                SET_USI_TO_TWI_START_CONDITION_MODE();
            }
            break;
        }
        // Master write data mode: check reply and goto USI_SLAVE_SEND_DATA if OK,
        // else reset USI
        case USI_SLAVE_CHECK_REPLY_FROM_SEND_DATA: {
            if (USIDR) {
                // if NACK, the master does not want more data
                SET_USI_TO_TWI_START_CONDITION_MODE();
                return;
            }
        }
        // From here we just drop straight into USI_SLAVE_SEND_DATA if the
        // master sent an ACK

        // Copy data from buffer to USIDR and set USI to shift byte
        // Next USI_SLAVE_REQUEST_REPLY_FROM_SEND_DATA
        case USI_SLAVE_SEND_DATA: {
            // Get data from Buffer
            if (txCount) {
                USIDR = txBuf[txTail];
                txTail = (txTail + 1) & TWI_TX_BUFFER_MASK;
                txCount--;
            } else {
                // The buffer is empty
                SET_USI_TO_READ_ACK();  // This might be necessary sometimes see http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&p=805227#805227
                SET_USI_TO_TWI_START_CONDITION_MODE();
                return;
            }
            overflowState = USI_SLAVE_REQUEST_REPLY_FROM_SEND_DATA;
            SET_USI_TO_SEND_DATA();
            break;
        }

        // Set USI to sample reply from master
        // Next USI_SLAVE_CHECK_REPLY_FROM_SEND_DATA
        case USI_SLAVE_REQUEST_REPLY_FROM_SEND_DATA: {
            overflowState = USI_SLAVE_CHECK_REPLY_FROM_SEND_DATA;
            SET_USI_TO_READ_ACK();
            break;
        }

        // Master read data mode: set USI to sample data from master, next
        // USI_SLAVE_GET_DATA_AND_SEND_ACK
        case USI_SLAVE_REQUEST_DATA: {
            overflowState = USI_SLAVE_GET_DATA_AND_SEND_ACK;
            SET_USI_TO_READ_DATA();
            break;
        }
        // Copy data from USIDR and send ACK
        // Next USI_SLAVE_REQUEST_DATA
        case USI_SLAVE_GET_DATA_AND_SEND_ACK: {
            // put data into buffer
            // check buffer size
            if (rxCount < TWI_RX_BUFFER_SIZE) {
                rxBuf[rxHead] = USIDR;
                rxHead = (rxHead + 1) & TWI_RX_BUFFER_MASK;
                rxCount++;
            } else {
                // Overrun
                // Drop data
            }
            // Next USI_SLAVE_REQUEST_DATA
            overflowState = USI_SLAVE_REQUEST_DATA;
            SET_USI_TO_SEND_ACK();
            break;
        }
    }

    USISR |= (1 << USI_OVERFLOW_FLAG); /* Reset the USI overflow flag in USISR register to prepare for new ints */
}
