/*
 *  NB USI-device interrupt-free TWI driver
 *  Author: Gustavo Casanova
 *  .............................................
 *  File: nb-usitwisl-if.h (Slave driver headers)
 *  .............................................
 *  Version: 1.0.2 / 2020-09-19
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

#include "hw-mapping.h"

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

#endif  // NB_USITWISL_IF_H
