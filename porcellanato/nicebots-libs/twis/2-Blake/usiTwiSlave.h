/*
* Atmel Corporation
*
* File              : USI_TWI_Slave.h
* Compiler          : IAR EWAAVR 4.11A
* Revision          : $Revision: 1.14 $
* Date              : $Date: Friday, December 09, 2005 17:25:38 UTC $
* Updated by        : $Author: jtyssoe $
*
* Support mail      : avr@atmel.com
*
* Supported devices : All device with USI module can be used.
*                     The example is written for the ATmega169, ATtiny26 & ATtiny2313
*
* AppNote           : AVR312 - Using the USI module as a TWI slave
*
* Description       : Header file for USI_TWI driver
*
*
* Modified by Jim Gallt for compatibility with avr-gcc
* February, 2012
*
****************************************************************************/

#include <stdbool.h>
#include <avr/interrupt.h>
#include <avr/io.h>

// Prototypes
/* GC Function names changed to match timonel.c
void	USI_TWI_Slave_Initialise( uint8_t );
void	USI_TWI_Transmit_Byte( uint8_t );
uint8_t	USI_TWI_Receive_Byte( void );
bool	USI_TWI_Data_In_Receive_Buffer( void );
void	USI_TWI_Attach_On_Request( void (*usrfunction)( void )); // user supplied function
*/
void	UsiTwiSlaveInit( uint8_t );
void	UsiTwiTransmitByte( uint8_t );
uint8_t	UsiTwiReceiveByte( void );
bool	USI_TWI_Data_In_Receive_Buffer( void );
void	USI_TWI_Attach_On_Request( void (*usrfunction)( void )); // user supplied function

void (*Usi_onRequestPtr)(void);
void (*Usi_onReceivePtr)(uint8_t);

// I2C handlers prototypes
// GC: These functions replace the USI hardware interrupts
void UsiStartHandler(void);
void UsiOverflowHandler(void);

//////////////////////////////////////////////////////////////////
///////////////// Driver Buffer Definitions //////////////////////
//////////////////////////////////////////////////////////////////
// 1,2,4,8,16,32,64,128 or 256 bytes are allowed buffer sizes

#define TWI_RX_BUFFER_SIZE  (16)
#define TWI_RX_BUFFER_MASK ( TWI_RX_BUFFER_SIZE - 1 )

#if ( TWI_RX_BUFFER_SIZE & TWI_RX_BUFFER_MASK )
        #error TWI RX buffer size is not a power of 2
#endif

// 1,2,4,8,16,32,64,128 or 256 bytes are allowed buffer sizes

#define TWI_TX_BUFFER_SIZE  (16)
#define TWI_TX_BUFFER_MASK ( TWI_TX_BUFFER_SIZE - 1 )

#if ( TWI_TX_BUFFER_SIZE & TWI_TX_BUFFER_MASK )
        #error TWI TX buffer size is not a power of 2
#endif

// TWI states
#define USI_SLAVE_CHECK_ADDRESS                (0x00)
#define USI_SLAVE_SEND_DATA                    (0x01)
#define USI_SLAVE_REQUEST_REPLY_FROM_SEND_DATA (0x02)
#define USI_SLAVE_CHECK_REPLY_FROM_SEND_DATA   (0x03)
#define USI_SLAVE_REQUEST_DATA                 (0x04)
#define USI_SLAVE_GET_DATA_AND_SEND_ACK        (0x05)


// Device dependent defines
#if defined(__AVR_AT90tiny26__) | defined(__AVR_ATtiny26__) // JGG
    #define DDR_USI             DDRB
    #define PORT_USI            PORTB
    #define PIN_USI             PINB
    #define PORT_USI_SDA        PB0
    #define PORT_USI_SCL        PB2
    #define PIN_USI_SDA         PINB0
    #define PIN_USI_SCL         PINB2
    #define USI_START_COND_INT  USISIF
    #define USI_START_VECTOR    USI_STRT_vect
    #define USI_OVERFLOW_VECTOR USI_OVF_vect
#endif

#if defined(__AVR_AT90Tiny2313__) | defined(__AVR_ATtiny2313__) | defined (__AVR_ATtiny4313__) // JGG
    #define DDR_USI             DDRB
    #define PORT_USI            PORTB
    #define PIN_USI             PINB
    #define PORT_USI_SDA        PB5
    #define PORT_USI_SCL        PB7
    #define PIN_USI_SDA         PINB5
    #define PIN_USI_SCL         PINB7
    #define USI_START_COND_INT  USISIF
    #define USI_START_VECTOR    USI_START_vect // JGG
    #define USI_OVERFLOW_VECTOR USI_OVERFLOW_vect // JGG
#endif

#if defined(__AVR_ATtiny25__) | defined(__AVR_ATtiny45__) | defined(__AVR_ATtiny85__)
    #define DDR_USI             DDRB
    #define PORT_USI            PORTB
    #define PIN_USI             PINB
    #define PORT_USI_SDA        PB0
    #define PORT_USI_SCL        PB2
    #define PIN_USI_SDA         PINB0
    #define PIN_USI_SCL         PINB2
    #define USI_START_COND_INT  USISIF /* GC: ORIGINALLY USICIF, changed to USISIF */
    #define USI_START_VECTOR    USI_START_vect
    #define USI_OVERFLOW_VECTOR USI_OVF_vect
#endif

#if defined(__AVR_AT90Mega165__) | defined(__AVR_ATmega165__) | \
    defined(__AVR_ATmega325__) | defined(__AVR_ATmega3250__) | \
    defined(__AVR_ATmega645__) | defined(__AVR_Tmega6450__) | \
    defined(__AVR_ATmega329__) | defined(__AVR_ATmega3290__) | \
    defined(__AVR_ATmega649__) | defined(__AVR_ATmega6490__)
    #define DDR_USI             DDRE
    #define PORT_USI            PORTE
    #define PIN_USI             PINE
    #define PORT_USI_SDA        PE5
    #define PORT_USI_SCL        PE4
    #define PIN_USI_SDA         PINE5
    #define PIN_USI_SCL         PINE4
    #define USI_START_COND_INT  USISIF
    #define USI_START_VECTOR    USI_START_vect
    #define USI_OVERFLOW_VECTOR USI_OVERFLOW_vect
#endif

#if defined(__AVR_AT90Mega169__) | defined(__AVR_ATmega169__)
    #define DDR_USI             DDRE
    #define PORT_USI            PORTE
    #define PIN_USI             PINE
    #define PORT_USI_SDA        PE5
    #define PORT_USI_SCL        PE4
    #define PIN_USI_SDA         PINE5
    #define PIN_USI_SCL         PINE4
    #define USI_START_COND_INT  USISIF
    #define USI_START_VECTOR    USI_START_vect
    #define USI_OVERFLOW_VECTOR USI_OVERFLOW_vect
#endif

