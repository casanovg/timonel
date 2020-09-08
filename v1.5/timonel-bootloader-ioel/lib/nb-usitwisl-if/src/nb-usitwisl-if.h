/*
 *  NB TWI interrupt-free USI-hardware driver
 *  Author: Gustavo Casanova
 *  .............................................
 *  File: nb-usitwisl-if.h (Slave driver headers)
 *  .............................................
 *  Version: 1.0.1 / 2020-09-07
 *  gustavo.casanova@nicebots.com
 *  .............................................
 *  Based on work by Atmel (AVR312) et others
 *  .............................................
 */

#ifndef NB_USITWISL_IF_H
#define NB_USITWISL_IF_H

// Includes
#include <avr/interrupt.h>
#include <stdbool.h>

// Driver buffer defines
// Allowed RX buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128 or 256
#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE 64
#endif  // TWI_RX_BUFFER_SIZE

#define TWI_RX_BUFFER_MASK (TWI_RX_BUFFER_SIZE - 1)

#if (TWI_RX_BUFFER_SIZE & TWI_RX_BUFFER_MASK)
#error TWI RX buffer size is not a power of 2
#endif  // TWI_RX_BUFFER_SIZE & TWI_RX_BUFFER_MASK

// Allowed TX buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128 or 256
#ifndef TWI_TX_BUFFER_SIZE
#define TWI_TX_BUFFER_SIZE 64
#endif  // TWI_TX_BUFFER_SIZE

#define TWI_TX_BUFFER_MASK (TWI_TX_BUFFER_SIZE - 1)

#if (TWI_TX_BUFFER_SIZE & TWI_TX_BUFFER_MASK)
#error TWI TX buffer size is not a power of 2
#endif  // TWI_TX_BUFFER_SIZE & TWI_TX_BUFFER_MASK

// TWI driver operational modes
typedef enum {
    STATE_CHECK_RECEIVED_ADDRESS = 0,
    STATE_SEND_DATA_BYTE = 1,
    STATE_RECEIVE_ACK_AFTER_SENDING_DATA = 2,
    STATE_CHECK_RECEIVED_ACK = 3,
    STATE_RECEIVE_DATA_BYTE = 4,
    STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK = 5
} OverflowState;

// Function pointers
void (*p_receive_event)(uint8_t);

// USI TWI driver prototypes
void UsiTwiTransmitByte(const uint8_t data_byte);
uint8_t UsiTwiReceiveByte(void);
void UsiTwiDriverInit(void);
void TwiStartHandler(void);
bool UsiOverflowHandler(void);

// -----------------------------------------------------------------------------------

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
#define TWI_START_COND_FLAG USISIF  // This status register flag indicates that an I2C START condition occurred on the bus (can trigger an interrupt)
#define USI_OVERFLOW_FLAG USIOIF    // This status register flag indicates that the bits reception or transmission is complete (can trigger an interrupt)
#define TWI_STOP_COND_FLAG USIPF    // This status register flag indicates that an I2C STOP condition occurred on the bus
#define TWI_COLLISION_FLAG USIDC    // This status register flag indicates that a data output collision occurred on the bus
#define TWI_START_COND_INT USISIE   // This control register bit defines whether an I2C START condition will trigger an interrupt
#define USI_OVERFLOW_INT USIOIE     // This control register bit defines whether an USI 4-bit counter overflow will trigger an interrupt
#endif                              // ATtinyX5

#endif  // NB_USITWISL_IF_H
