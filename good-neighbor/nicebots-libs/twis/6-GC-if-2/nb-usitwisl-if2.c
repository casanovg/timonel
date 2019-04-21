/*
 *  Timonel USI TWI Interrupt-Free Driver
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: nb-usitwisl.c (Driver library)
 *  ........................................... 
 *  Version: 1.0 / 2019-04-19
 *  gustavo.casanova@nicebots.com
 *  ........................................... 
 *  Based on work by Atmel (AVR312), Don Blake,
 *  Rambo, bHogan, A. Vogel et others
 *  ...........................................
 */

// Includes
#include "nb-usitwisl-if2.h"

// USI direction setting prototypes
inline void SET_USI_SDA_AS_OUTPUT() __attribute__((always_inline));
inline void SET_USI_SDA_AS_INPUT() __attribute__((always_inline));
inline void SET_USI_SCL_AS_OUTPUT() __attribute__((always_inline));
inline void SET_USI_SCL_AS_INPUT() __attribute__((always_inline));
inline void SET_USI_SDA_AND_SCL_AS_OUTPUT() __attribute__((always_inline));
inline void SET_USI_SDA_AND_SCL_AS_INPUT() __attribute__((always_inline));

// USI basic TWI operations prototypes
inline void SET_USI_TO_SEND_ACK() __attribute__((always_inline));
inline void SET_USI_TO_WAIT_ACK() __attribute__((always_inline));
inline void SET_USI_TO_WAIT_FOR_START_COND_AND_ADDRESS() __attribute__((always_inline));
inline void SET_USI_TO_SEND_DATA() __attribute__((always_inline));
inline void SET_USI_TO_RECEIVE_DATA() __attribute__((always_inline));

// Data callback prototypes
inline void DATA_RECEIVED_FROM_MASTER_CALLBACK() __attribute__((always_inline));
inline void DATA_REQUESTED_BY_MASTER_CALLBACK() __attribute__((always_inline));
inline void STOP_CONDITION_RECEIVED_CALLBACK() __attribute__((always_inline));

// USI direction setting functions
inline void SET_USI_SDA_AS_OUTPUT() {
	DDR_USI |=  (1 << PORT_USI_SDA);
}

inline void SET_USI_SDA_AS_INPUT() {
	DDR_USI &= ~(1 << PORT_USI_SDA);
}

inline void SET_USI_SCL_AS_OUTPUT() {
	DDR_USI |=  (1 << PORT_USI_SCL);
}

inline void SET_USI_SCL_AS_INPUT() {
	DDR_USI &= ~(1 << PORT_USI_SCL);
}

inline void SET_USI_SDA_AND_SCL_AS_OUTPUT() {
	DDR_USI |= (1 << PORT_USI_SDA) | (1 << PORT_USI_SCL);
}

inline void SET_USI_SDA_AND_SCL_AS_INPUT() {
	DDR_USI &= ~((1 << PORT_USI_SDA) | (1 << PORT_USI_SCL));
}

// USI basic TWI operations functions
// -----------------------------------------------------
inline void SET_USI_TO_SEND_ACK() {
	USIDR = 0;	/* Set the USI data register for sending an ACK (TWI ACK = low) */
	SET_USI_SDA_AS_OUTPUT(); /* Drive the SDA line */
	/* Clear all USI status register interrupt flags, except start condition (so SCL clock line is NOT released???) */
	USISR = (0 << TWI_START_COND_FLAG) |
			(1 << USI_OVERFLOW_FLAG) |
			(1 << TWI_STOP_COND_FLAG) |
			(1 << TWI_COLLISION_FLAG) |
			(0x0E << USICNT0); /* Set status register 4-bit counter to shift 1 bit (ACK bit) */
}
// -----------------------------------------------------
inline void SET_USI_TO_WAIT_ACK() {
	USIDR = 0;	/* Clear the USI data register to receive an ACK */
	SET_USI_SDA_AS_INPUT(); /* Float the SDA line */
	/* Clear all USI status register interrupt flags, except start condition */
	USISR = (0 << TWI_START_COND_FLAG) |
			(1 << USI_OVERFLOW_FLAG) |
			(1 << TWI_STOP_COND_FLAG) |
			(1 << TWI_COLLISION_FLAG) |
			(0x0E << USICNT0); /* Set status register 4-bit counter to shift 1 bit (ACK bit) */	
}
// -----------------------------------------------------
inline void SET_USI_TO_WAIT_FOR_START_COND_AND_ADDRESS() {
	/* Configure USI control register */
	USICR = (1 << TWI_START_COND_INT) | (0 << USI_OVERFLOW_INT) | /* Enable start condition interrupt, disable overflow interrupt */
		    (1 << USIWM1) | (0 << USIWM0) | /* Set USI in Two-wire mode, no SCL hold when the 4-bit counter overflows */
			(1 << USICS1) | (0 << USICS0) | (0 << USICLK) | /* Clock Source = External (positive edge) for data register, External (both edges) for 4-Bit counter */
			(0 << USITC); /* No toggle clock-port pin (SCL) */
	/* Clear all USI status register interrupt flags, except start condition */
	USISR = (0 << TWI_START_COND_FLAG) |
			(1 << USI_OVERFLOW_FLAG) |
			(1 << TWI_STOP_COND_FLAG) |
			(1 << TWI_COLLISION_FLAG) |
			(0x0 << USICNT0); /* Set status register 4-bit counter to shift 8 bits (address byte to be received) */	 		
}
// -----------------------------------------------------
inline void SET_USI_TO_SEND_DATA() {
	SET_USI_SDA_AS_OUTPUT(); /* Drive the SDA line */
	/* Clear all USI status register interrupt flags, except start condition */
	USISR = (0 << TWI_START_COND_FLAG) |
			(1 << USI_OVERFLOW_FLAG) |
			(1 << TWI_STOP_COND_FLAG) |
			(1 << TWI_COLLISION_FLAG) |
			(0x0 << USICNT0); /* Set status register 4-bit counter to shift 8 bits (data byte to be transmitted) */		
}
// -----------------------------------------------------
inline void SET_USI_TO_RECEIVE_DATA() {
	SET_USI_SDA_AS_INPUT(); /* Float the SDA line */
	/* Clear all USI status register interrupt flags, except start condition */
	USISR = (0 << TWI_START_COND_FLAG) |
			(1 << USI_OVERFLOW_FLAG) |
			(1 << TWI_STOP_COND_FLAG) |
			(1 << TWI_COLLISION_FLAG) |
			(0x0 << USICNT0); /* Set status register 4-bit counter to shift 8 bits (data byte to be received) */		
	
}

// Data callback functions
// -----------------------------------------------------
inline void DATA_RECEIVED_FROM_MASTER_CALLBACK() {
	if (Usi_onReceivePtr) {
		if (UsiTwiAmountDataInReceiveBuffer()) {
			Usi_onReceivePtr(UsiTwiAmountDataInReceiveBuffer());
		}
	}
}
// -----------------------------------------------------
inline void DATA_REQUESTED_BY_MASTER_CALLBACK() {
	DATA_RECEIVED_FROM_MASTER_CALLBACK();
	if(Usi_onRequestPtr) {
		Usi_onRequestPtr();
    }
}
// -----------------------------------------------------
inline void STOP_CONDITION_RECEIVED_CALLBACK() {
    if (USISR & (1 << TWI_STOP_COND_FLAG)) {
		DATA_RECEIVED_FROM_MASTER_CALLBACK();
    } 	
}

// Function FlushTwiBuffers
static void FlushTwiBuffers(void) {
    rxTail = 0;
    rxHead = 0;
    rxCount = 0;
    txTail = 0;
    txHead = 0;
    txCount = 0;
}

// Function UsiTwiSlaveInit
//void UsiTwiSlaveInit(uint8_t twi_addr) {
void UsiTwiSlaveInit(void) {
    FlushTwiBuffers();
    //twi_address = twi_addr;

    // In Two Wire mode (USIWM1, USIWM0 = 1X), the slave USI will pull SCL
    // low when a start condition is detected or a counter overflow (only
    // for USIWM1, USIWM0 = 11).  This inserts a wait state.  SCL is released
    // by the ISRs (USI_START_vect and USI_OVERFLOW_vect).

    // Set SCL and SDA as output
	SET_USI_SDA_AND_SCL_AS_OUTPUT();

    // set SCL high
    PORT_USI |= (1 << PORT_USI_SCL);

    // set SDA high
    PORT_USI |= (1 << PORT_USI_SDA);

    // Set SDA as input
	SET_USI_SDA_AS_INPUT();

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
    USISR = (1 << TWI_START_COND_FLAG) |
			(1 << USI_OVERFLOW_FLAG) |
			(1 << TWI_STOP_COND_FLAG) |
			(1 << TWI_COLLISION_FLAG);
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
    tx_buffer[txHead] = data;
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
    rtn_byte = rx_buffer[rxTail];
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
void UsiStartHandler() {
    // This triggers on second write, but claims to the callback there is only *one* byte in buffer
    //STOP_CONDITION_RECEIVED_CALLBACK();

    // This triggers on second write, but claims to the callback there is only *one* byte in buffer
    //DATA_RECEIVED_FROM_MASTER_CALLBACK();

    // set default starting conditions for new TWI package
    current_state = USI_SLAVE_CHECK_ADDRESS;

    // set SDA as input
    DDR_USI &= ~(1 << PORT_USI_SDA);

    // Wait for SCL to go low to ensure the Start Condition has completed (the
    // start detector will hold SCL low ) - if a Stop Condition arises then leave
    // the interrupt to prevent waiting forever - don't use USISR to test for Stop
    // Condition as in Application Note AVR312 because the Stop Condition Flag is
    // going to be set from the last TWI sequence
    while ((PIN_USI & (1 << PORT_USI_SCL)) && (!(PIN_USI & (1 << PORT_USI_SDA)))) {
        // SCL his high
        // (PIN_USI & (1 << PIN_USI_SCL)) &&
        // // and SDA is low
        // !((PIN_USI & (1 << PIN_USI_SDA))))
        // ;
	}

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

    USISR |= (1 << TWI_START_COND_FLAG); /* GC: Clear the TWI start condition flag in USI status register to prepare for new ints */
}

// Function UsiOverflowHandler
// GC: Interrupt-like function
void UsiOverflowHandler(uint8_t twi_address) {
    switch (current_state) {
        // Address mode: check address and send ACK (and next USI_SLAVE_SEND_DATA) if OK,
        // else reset USI
        case USI_SLAVE_CHECK_ADDRESS: {
            // #############################################################
            // # GC: Re-enable this device answers to "general calls"
            // #############################################################
            if ((USIDR == 0) || ((USIDR >> 1) == twi_address)) {
            //if (((USIDR >> 1) & 0x3F) == twi_address) {
                if (USIDR & 0x01) {             /* If 1: Slave sends data to master */
                    DATA_REQUESTED_BY_MASTER_CALLBACK();
                    current_state = USI_SLAVE_SEND_DATA;
                } else {                        /* If 0: Slave receives data from master */
                    current_state = USI_SLAVE_REQUEST_DATA;
                }
                SET_USI_TO_SEND_ACK();
            }
            else {
                SET_USI_TO_WAIT_FOR_START_COND_AND_ADDRESS();
            }
            break;
        }
        // Master write data mode: check reply and goto USI_SLAVE_SEND_DATA if OK,
        // else reset USI
        case USI_SLAVE_CHECK_REPLY_FROM_SEND_DATA: {
            if (USIDR) {
                // if NACK, the master does not want more data
                SET_USI_TO_WAIT_FOR_START_COND_AND_ADDRESS();
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
                USIDR = tx_buffer[txTail];
                txTail = (txTail + 1) & TWI_TX_BUFFER_MASK;
                txCount--;
            } else {
                // The buffer is empty
                SET_USI_TO_WAIT_ACK();  // This might be necessary sometimes see http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&p=805227#805227
                SET_USI_TO_WAIT_FOR_START_COND_AND_ADDRESS();
                return;
            }
            current_state = USI_SLAVE_REQUEST_REPLY_FROM_SEND_DATA;
            SET_USI_TO_SEND_DATA();
            break;
        }

        // Set USI to sample reply from master
        // Next USI_SLAVE_CHECK_REPLY_FROM_SEND_DATA
        case USI_SLAVE_REQUEST_REPLY_FROM_SEND_DATA: {
            current_state = USI_SLAVE_CHECK_REPLY_FROM_SEND_DATA;
            SET_USI_TO_WAIT_ACK();
            break;
        }

        // Master read data mode: set USI to sample data from master, next
        // USI_SLAVE_GET_DATA_AND_SEND_ACK
        case USI_SLAVE_REQUEST_DATA: {
            current_state = USI_SLAVE_GET_DATA_AND_SEND_ACK;
            SET_USI_TO_RECEIVE_DATA();
            break;
        }
        // Copy data from USIDR and send ACK
        // Next USI_SLAVE_REQUEST_DATA
        case USI_SLAVE_GET_DATA_AND_SEND_ACK: {
            // put data into buffer
            // check buffer size
            if (rxCount < TWI_RX_BUFFER_SIZE) {
                rx_buffer[rxHead] = USIDR;
                rxHead = (rxHead + 1) & TWI_RX_BUFFER_MASK;
                rxCount++;
            } else {
                // Overrun
                // Drop data
            }
            // Next USI_SLAVE_REQUEST_DATA
            current_state = USI_SLAVE_REQUEST_DATA;
            SET_USI_TO_SEND_ACK();
            break;
        }
    }

    USISR |= (1 << USI_OVERFLOW_FLAG); /* GC: Clear the 4-bit counter overflow flag in USI status register to prepare for new ints */
}
