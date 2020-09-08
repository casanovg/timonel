/*
 *  NB TWI interrupt-free USI-hardware driver
 *  Author: Gustavo Casanova
 *  .............................................
 *  File: nb-usitwisl-if.c (Slave driver library)
 *  .............................................
 *  Version: 1.0.1 / 2020-09-07
 *  gustavo.casanova@nicebots.com
 *  .............................................
 *  Based on work by Atmel (AVR312) et others
 *  .............................................
 */

// Includes
#include "nb-usitwisl-if.h"

// USI TWI driver globals
static uint8_t rx_buffer[TWI_RX_BUFFER_SIZE];
static uint8_t tx_buffer[TWI_TX_BUFFER_SIZE];
static uint8_t rx_head = 0, rx_tail = 0;
static uint8_t tx_head = 0, tx_tail = 0;
static uint8_t rx_byte_count = 0;  // Bytes received in RX buffer
static uint8_t tx_byte_count = 0;  // Bytes to transmit in TX buffer
static OverflowState device_state;

// USI TWI driver basic operations prototypes
void SET_USI_TO_WAIT_FOR_TWI_ADDRESS(void);
inline static void SET_USI_TO_SEND_BYTE(void) __attribute__((always_inline));
inline static void SET_USI_TO_RECEIVE_BYTE(void) __attribute__((always_inline));
inline static void SET_USI_TO_SEND_ACK(void) __attribute__((always_inline));
inline static void SET_USI_TO_RECEIVE_ACK(void) __attribute__((always_inline));
inline static void SET_USI_TO_DETECT_TWI_START(void) __attribute__((always_inline));
inline static void SET_USI_TO_DETECT_TWI_RESTART(void) __attribute__((always_inline));
inline static void SET_USI_TO_SHIFT_8_ADDRESS_BITS(void) __attribute__((always_inline));
inline static void SET_USI_TO_SHIFT_8_DATA_BITS(void) __attribute__((always_inline));
inline static void SET_USI_TO_SHIFT_1_ACK_BIT(void) __attribute__((always_inline));

// USI TWI driver direction setting prototypes
inline static void SET_USI_SDA_AS_OUTPUT(void) __attribute__((always_inline));
inline static void SET_USI_SDA_AS_INPUT(void) __attribute__((always_inline));
inline static void SET_USI_SCL_AS_OUTPUT(void) __attribute__((always_inline));
inline static void SET_USI_SCL_AS_INPUT(void) __attribute__((always_inline));
inline static void SET_USI_SDA_AND_SCL_AS_OUTPUT(void) __attribute__((always_inline));
inline static void SET_USI_SDA_AND_SCL_AS_INPUT(void) __attribute__((always_inline));

/* ___________________________
  |                           |
  | USI TWI byte transmission |
  |___________________________|
*/
void UsiTwiTransmitByte(const uint8_t data_byte) {
    while (tx_byte_count == TWI_RX_BUFFER_SIZE) {
    };                                               // Wait for a free spot in the buffer
    tx_head = ((tx_head + 1) & TWI_TX_BUFFER_MASK);  // Update the TX buffer head pointer
    tx_buffer[tx_head] = data_byte;                  // Write the data byte into the TX buffer
    tx_byte_count++;                                 // Update TX buffer used positions counter
}

/*  ___________________________
   |                           |
   | USI TWI byte reception    |
   |___________________________|
*/
uint8_t UsiTwiReceiveByte(void) {
    while (!rx_byte_count) {
    };                                               // Wait until data is present in the RX buffer
    rx_tail = ((rx_tail + 1) & TWI_RX_BUFFER_MASK);  // Update the RX buffer tail pointer
    rx_byte_count--;                                 // Update RX buffer used positions counter
    return rx_buffer[rx_tail];                       // Return data from the RX buffer
}

/*  _______________________________
   |                               |
   | USI TWI driver initialization |
   |_______________________________|
*/
void UsiTwiDriverInit(void) {
    // Initialize USI for TWI Slave mode.
    tx_tail = tx_head = 0;                  // Flush TWI TX buffers */
    rx_tail = rx_head = rx_byte_count = 0;  // Flush TWI RX buffers */
    SET_USI_SDA_AND_SCL_AS_OUTPUT();        // Set SCL and SDA as output */
    PORT_USI |= (1 << PORT_USI_SDA);        // Set SDA high */
    PORT_USI |= (1 << PORT_USI_SCL);        // Set SCL high */
    SET_USI_SDA_AS_INPUT();                 // Set SDA as input */
    SET_USI_TO_WAIT_FOR_TWI_ADDRESS();      // Wait for TWI start condition and address from master */
}

/*  _______________________________________________________
   |                                                       |
   | TWI start condition handler (Interrupt-like function) |
   |_______________________________________________________|
*/
inline void TwiStartHandler(void) {
    SET_USI_SDA_AS_INPUT();  // Float the SDA line
    // Following a start condition, the device shifts the address present on the TWI bus in and
    // a 4-bit counter overflow is triggered. Afterward, within the overflow handler, the device
    // should check whether it has to reply. Prepare the next overflow handler state for it.
    // Next state -> STATE_CHECK_RECEIVED_ADDRESS
    device_state = STATE_CHECK_RECEIVED_ADDRESS;
    while ((PIN_USI & (1 << PORT_USI_SCL)) && (!(PIN_USI & (1 << PORT_USI_SDA)))) {
        // Wait for SCL to go low to ensure the start condition has completed.
        // The start detector will hold SCL low.
    }
    // If a stop condition arises then leave this function to prevent waiting forever.
    // Don't use USISR to test for stop condition as in application note AVR312
    // because the stop condition flag is going to be set from the last TWI sequence.
    if (!(PIN_USI & (1 << PIN_USI_SDA))) {
        // ==> Stop condition NOT DETECTED
        SET_USI_TO_DETECT_TWI_RESTART();
    } else {
        // ==> Stop condition DETECTED
        SET_USI_TO_DETECT_TWI_START();
    }
    // Read the address present on the TWI bus
    SET_USI_TO_SHIFT_8_ADDRESS_BITS();
}

/*  ______________________________________________________
   |                                                      |
   | USI 4-bit overflow handler (Interrupt-like function) |
   |______________________________________________________|
*/
inline bool UsiOverflowHandler(void) {
    switch (device_state) {
        // If the address received after the start condition matches this device or is
        // a general call, reply ACK and check whether it should send or receive data.
        // Otherwise, set USI to wait for the next start condition and address.
        case STATE_CHECK_RECEIVED_ADDRESS: {
            if ((USIDR == 0) || ((USIDR >> 1) == TWI_ADDR)) {
                if (USIDR & 0x01) { /* If data register low-order bit = 1, start the send data mode */
                    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                    if (p_receive_event) {               //                             >>
                        p_receive_event(rx_byte_count);  // Process data in main ...     >>
                    }                                    //                             >>
                    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                    // Next state -> STATE_SEND_DATA_BYTE
                    device_state = STATE_SEND_DATA_BYTE;
                } else { /* If data register low-order bit = 0, start the receive data mode */
                    // Next state -> STATE_RECEIVE_DATA_BYTE
                    device_state = STATE_RECEIVE_DATA_BYTE;
                }
                SET_USI_TO_SEND_ACK();
            } else {
                SET_USI_TO_WAIT_FOR_TWI_ADDRESS();
            }
            return false;
        }
        // Send data mode:
        //================
        // 3) Check whether the acknowledge bit received from the master is ACK or
        // NACK. If ACK (low), just continue to STATE_SEND_DATA_BYTE without break. If NACK (high)
        // the transmission is complete. Wait for a new start condition and TWI address.
        case STATE_CHECK_RECEIVED_ACK: {
            if (USIDR) {  // NACK - handshake complete ...
                SET_USI_TO_WAIT_FOR_TWI_ADDRESS();
                // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                //                                                                  >>
                return true;  // Enable slow operations in main!                     >>
                //                                                                  >>
                // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
            }
            // Just drop straight into STATE_SEND_DATA_BYTE (no break) ...
        }
        // 1) Copy data from TX buffer to USIDR and set USI to shift 8 bits out. When the 4-bit
        // counter overflows, it means that a byte has been transmitted, so this device is ready
        // to transmit again or wait for a new start condition and address on the bus.
        case STATE_SEND_DATA_BYTE: {
            if (tx_head != tx_tail) {
                // If the TX buffer has data, copy the next byte to USI data register for sending
                tx_tail = ((tx_tail + 1) & TWI_TX_BUFFER_MASK);
                USIDR = tx_buffer[tx_tail];
                tx_byte_count--;
            } else {
                // If the buffer is empty ...
                SET_USI_TO_RECEIVE_ACK();  // This might be necessary (http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&p=805227#805227)
                SET_USI_TO_WAIT_FOR_TWI_ADDRESS();
                return false;
            }
            // Next state -> STATE_RECEIVE_ACK_AFTER_SENDING_DATA
            device_state = STATE_RECEIVE_ACK_AFTER_SENDING_DATA;
            SET_USI_TO_SEND_BYTE();
            return false;
        }
        // 2) Set USI to receive an acknowledge bit reply from master
        case STATE_RECEIVE_ACK_AFTER_SENDING_DATA: {
            // Next state -> STATE_CHECK_RECEIVED_ACK
            device_state = STATE_CHECK_RECEIVED_ACK;
            SET_USI_TO_RECEIVE_ACK();
            return false;
        }
        // Receive data mode:
        // ==================
        // 1) Set the USI to shift 8 bits in. When the 4-bit counter overflows,
        // it means that a byte has been received and this device should process it
        // on the next overflow state (STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK).
        case STATE_RECEIVE_DATA_BYTE: {
            // Next state -> STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK
            device_state = STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK;
            SET_USI_TO_RECEIVE_BYTE();
            return false;
        }
        // 2) Copy the received byte from USIDR to RX buffer and send ACK. After the
        // counter overflows, return to the previous state (STATE_RECEIVE_DATA_BYTE).
        // This mode's cycle should end when a stop condition is detected on the bus.
        case STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK: {
            // Put data into buffer
            rx_head = ((rx_head + 1) & TWI_RX_BUFFER_MASK);
            rx_buffer[rx_head] = USIDR;
            rx_byte_count++;
            // Next state -> STATE_RECEIVE_DATA_BYTE
            device_state = STATE_RECEIVE_DATA_BYTE;
            SET_USI_TO_SEND_ACK();
            return false;
        }
    }
    // Clear the 4-bit counter overflow flag in USI status register after processing each
    // overflow state to allow detecting new interrupts that take this device to next states.
    USISR |= (1 << USI_OVERFLOW_FLAG);
    return false;
}

// ----------------------------------------------------------------------------
// USI TWI basic operations functions
// ----------------------------------------------------------------------------
// Set USI to detect start and shift 7 address bits + 1 direction bit in.
void SET_USI_TO_WAIT_FOR_TWI_ADDRESS(void) {
    SET_USI_TO_DETECT_TWI_START();   // Detect start condition
    SET_USI_TO_SHIFT_8_DATA_BITS();  // Shift 8 bits
}
// ............................................................................
// Set USI to send a byte.
inline void SET_USI_TO_SEND_BYTE(void) {
    SET_USI_SDA_AS_OUTPUT();         // Drive the SDA line
    SET_USI_TO_SHIFT_8_DATA_BITS();  // Shift 8 bits
}
// ............................................................................
// Set USI to receive a byte.
inline void SET_USI_TO_RECEIVE_BYTE(void) {
    SET_USI_SDA_AS_INPUT();          // Float the SDA line
    SET_USI_TO_SHIFT_8_DATA_BITS();  // Shift 8 bits
}
// ............................................................................
// Set USI to send an ACK bit.
inline void SET_USI_TO_SEND_ACK(void) {
    USIDR = 0;                     // Clear the USI data register
    SET_USI_SDA_AS_OUTPUT();       // Drive the SDA line
    SET_USI_TO_SHIFT_1_ACK_BIT();  // Shift 1 bit
}
// ............................................................................
// Set USI to receive an ACK bit.
inline void SET_USI_TO_RECEIVE_ACK(void) {
    USIDR = 0;                     // Clear the USI data register
    SET_USI_SDA_AS_INPUT();        // Float the SDA line
    SET_USI_TO_SHIFT_1_ACK_BIT();  // Shift 1 bit
}

// ----------------------------------------------------------------------------
// USI register configurations
// ----------------------------------------------------------------------------
// Configure USI control register to detect start condition.
inline void SET_USI_TO_DETECT_TWI_START(void) {
    USICR = (1 << TWI_START_COND_INT) | (0 << USI_OVERFLOW_INT) |  // Enable start condition interrupt, disable overflow interrupt
            (1 << USIWM1) | (0 << USIWM0) |                        // Set USI in Two-wire mode, don't hold SCL low when the 4-bit counter overflows
            (1 << USICS1) | (0 << USICS0) | (0 << USICLK) |        // Clock Source = External (positive edge) for data register, External (both edges) for 4-Bit counter
            (0 << USITC);                                          // No toggle clock-port pin (SCL)
}
// ............................................................................
// Configure USI control register to detect RESTART.
inline void SET_USI_TO_DETECT_TWI_RESTART(void) {
    USICR = (1 << TWI_START_COND_INT) | (1 << USI_OVERFLOW_INT) |  // Enable start condition interrupt, disable overflow interrupt
            (1 << USIWM1) | (1 << USIWM0) |                        // Set USI in Two-wire mode, hold SCL low when the 4-bit counter overflows
            (1 << USICS1) | (0 << USICS0) | (0 << USICLK) |        // Clock Source = External (positive edge) for data register, External (both edges) for 4-Bit counter
            (0 << USITC);                                          // No toggle clock-port pin (SCL)
}
// ............................................................................
// Clear all USI status register interrupt flags to prepare for new start conditions.
inline void SET_USI_TO_SHIFT_8_ADDRESS_BITS(void) {
    USISR = (1 << TWI_START_COND_FLAG) |
            (1 << USI_OVERFLOW_FLAG) |
            (1 << TWI_STOP_COND_FLAG) |
            (1 << TWI_COLLISION_FLAG) |
            (0x0 << USICNT0);  // Reset status register 4-bit counter to shift 8 bits (data byte to be received)
}
// ............................................................................
// Clear all USI status register interrupt flags, except start condition.
void SET_USI_TO_SHIFT_8_DATA_BITS(void) {
    USISR = (0 << TWI_START_COND_FLAG) |
            (1 << USI_OVERFLOW_FLAG) |
            (1 << TWI_STOP_COND_FLAG) |
            (1 << TWI_COLLISION_FLAG) |
            (0x0 << USICNT0);  // Set status register 4-bit counter to shift 8 bits
}
// ............................................................................
// Clear all USI status register interrupt flags, except start condition.
inline void SET_USI_TO_SHIFT_1_ACK_BIT(void) {
    USISR = (0 << TWI_START_COND_FLAG) |
            (1 << USI_OVERFLOW_FLAG) |
            (1 << TWI_STOP_COND_FLAG) |
            (1 << TWI_COLLISION_FLAG) |
            (0x0E << USICNT0);  // Set status register 4-bit counter to shift 1 bit
}

// ----------------------------------------------------------------------------
// GPIO TWI direction settings
// ----------------------------------------------------------------------------
// Drive the data line
inline void SET_USI_SDA_AS_OUTPUT(void) {
    DDR_USI |= (1 << PORT_USI_SDA);
}
// ............................................................................
// Float the data line
inline void SET_USI_SDA_AS_INPUT(void) {
    DDR_USI &= ~(1 << PORT_USI_SDA);
}
// ............................................................................
// Drive the clock line
inline void SET_USI_SCL_AS_OUTPUT(void) {
    DDR_USI |= (1 << PORT_USI_SCL);
}
// ............................................................................
// Float the clock line
inline void SET_USI_SCL_AS_INPUT(void) {
    DDR_USI &= ~(1 << PORT_USI_SCL);
}
// ............................................................................
// Drive the data and clock lines
inline void SET_USI_SDA_AND_SCL_AS_OUTPUT(void) {
    DDR_USI |= (1 << PORT_USI_SDA) | (1 << PORT_USI_SCL);
}
// ............................................................................
// Float the data and clock lines
inline void SET_USI_SDA_AND_SCL_AS_INPUT(void) {
    DDR_USI &= ~((1 << PORT_USI_SDA) | (1 << PORT_USI_SCL));
}
