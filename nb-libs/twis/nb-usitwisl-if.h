/*
 *  Timonel USI TWI Driver (Interrupt-Free)
 *  ..................................................
 *  Based on USITWISlave by Donald Blake
 *  donblake at worldnet.att.net
 *  .................................................. 
 *  Adapted to work interrupt-free
 *  2018-07-15 gustavo.casanova@nicebots.com
 */

#ifndef _NB_USITWISL_IF_H_
#define _NB_USITWISL_IF_H_

// Includes
#include <stdbool.h>
#include <avr/interrupt.h>

// Prototypes
void UsiTwiSlaveInit(uint8_t);
void UsiTwiTransmitByte(uint8_t);
uint8_t UsiTwiReceiveByte(void);
bool UsiTwiDataInTransmitBuffer(void);
uint8_t UsiTwiAmountDataInReceiveBuffer(void);
void (*Usi_onRequestPtr)(void);
void (*Usi_onReceivePtr)(uint8_t);

// I2C handlers prototypes
// GC: These functions replace the USI hardware interrupts
void UsiStartHandler(void);
void UsiOverflowHandler(void);

// Driver buffer definitions
// Allowed RX buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128 or 256
#ifndef TWI_RX_BUFFER_SIZE
    #define TWI_RX_BUFFER_SIZE (16)
#endif

#define TWI_RX_BUFFER_MASK (TWI_RX_BUFFER_SIZE - 1)

#if (TWI_RX_BUFFER_SIZE & TWI_RX_BUFFER_MASK)
    #  error TWI RX buffer size is not a power of 2
#endif

// Allowed TX buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128 or 256
#ifndef TWI_TX_BUFFER_SIZE
    #define TWI_TX_BUFFER_SIZE (16)
#endif

#define TWI_TX_BUFFER_MASK (TWI_TX_BUFFER_SIZE - 1)

#if (TWI_TX_BUFFER_SIZE & TWI_TX_BUFFER_MASK)
    #  error TWI TX buffer size is not a power of 2
#endif

#endif /* _NB_USITWISL_IF_H_ */
