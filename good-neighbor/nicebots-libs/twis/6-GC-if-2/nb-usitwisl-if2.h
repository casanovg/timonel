/*
 *  Timonel USI TWI Interrupt-Free Driver
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: nb-usitwisl.c (Driver headers)
 *  ........................................... 
 *  Version: 1.0 / 2019-04-19
 *  gustavo.casanova@nicebots.com
 *  ........................................... 
 *  Based on work by Atmel (AVR312), Don Blake,
 *  Rambo, bHogan, A.Vogel et others
 *  ...........................................
 */

#ifndef _NB_USITWISL_IF_H_
#define _NB_USITWISL_IF_H_

// Includes
#include <avr/interrupt.h>
#include <stdbool.h>

// Driver buffer definitions
// Allowed RX buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128 or 256
#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE (16)
#endif

#define TWI_RX_BUFFER_MASK (TWI_RX_BUFFER_SIZE - 1)

#if (TWI_RX_BUFFER_SIZE & TWI_RX_BUFFER_MASK)
#error TWI RX buffer size is not a power of 2
#endif

// Allowed TX buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128 or 256
#ifndef TWI_TX_BUFFER_SIZE
#define TWI_TX_BUFFER_SIZE (16)
#endif

#define TWI_TX_BUFFER_MASK (TWI_TX_BUFFER_SIZE - 1)

#if (TWI_TX_BUFFER_SIZE & TWI_TX_BUFFER_MASK)
#error TWI TX buffer size is not a power of 2
#endif

// Device modes
typedef enum {
	STATE_CHECK_ADDRESS,
    STATE_SEND_DATA,
    STATE_WAIT_ACK_AFTER_SEND_DATA,
    STATE_CHECK_ACK_AFTER_SEND_DATA,
    STATE_WAIT_DATA_RECEPTION,
    STATE_RECEIVE_DATA_AND_SEND_ACK   
} OperationalState;

// Global library variables
OperationalState device_state;
uint8_t rx_buffer[TWI_RX_BUFFER_SIZE];
uint8_t rx_head;
uint8_t rx_tail;
uint8_t rx_count;
uint8_t tx_buffer[TWI_TX_BUFFER_SIZE];
uint8_t tx_head;
uint8_t tx_tail;
uint8_t tx_count;

// Prototypes
//void UsiTwiSlaveInit(uint8_t);
void UsiTwiSlaveInit(void);
void UsiTwiTransmitByte(uint8_t);
uint8_t UsiTwiReceiveByte(void);
bool UsiTwiDataInTransmitBuffer(void);
uint8_t UsiTwiAmountDataInReceiveBuffer(void);
void (*Usi_onRequestPtr)(void);
void (*Usi_onReceivePtr)(uint8_t);

// I2C handlers prototypes (GC: These functions replace the USI hardware interrupts)
void UsiStartHandler(void);
void UsiOverflowHandler(uint8_t);

// -------- Internals ---------

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
#define TWI_START_COND_FLAG	USISIF	/* This status register flag indicates that an I2C START condition occurred on the bus (can trigger an interrupt) */
#define USI_OVERFLOW_FLAG USIOIF	/* This status register flag indicates that the bits reception or transmission is complete (can trigger an interrupt) */
#define TWI_STOP_COND_FLAG USIPF	/* This status register flag indicates that an I2C STOP condition occurred on the bus */
#define TWI_COLLISION_FLAG USIDC	/* This status register flag indicates that a data output collision occurred on the bus */
#define TWI_START_COND_INT USISIE	/* This control register bit defines whether an I2C START condition will trigger an interrupt */
#define USI_OVERFLOW_INT USIOIE		/* This control register bit defines whether an USI 4-bit counter overflow will trigger an interrupt */
#endif

#endif /* _NB_USITWISL_IF_H_ */
