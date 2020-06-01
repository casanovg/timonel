/*
 *  AVR BLINK TWIS
 *  Author: Gustavo Casanova / Nicebots
 *  ...........................................
 *  File: avr-blink-twis.c (Blink application)
 *  ........................................... 
 *  Version: 1.0 / 2019-06-22
 *  gustavo.casanova@nicebots.com
 *  ...........................................
 */

/* This a led blink test program compatible with
   the NB command-set through TWI (I2C).

   Available NB TWI commands:
   ------------------------------
   a - (SETIO1_1) Start blinking
   s - (SETIO1_0) Stop blinking
   x - (RESETMCU) Reset device
*/

// Includes
#include "avr-blink-twis.h"

#define LONG_DELAY 0xFFFF

// Global variables
uint8_t command[32] = {0}; /* I2C Command received from master  */
uint8_t commandLength = 0; /* I2C Command number of bytes  */

bool reset_now = false;
bool slow_ops_enabled = false;
volatile bool blink = true;
volatile uint16_t toggle_delay = LONG_DELAY;

// Prototypes
void DisableWatchDog(void);
void SetCPUSpeed1MHz(void);
void SetCPUSpeed8MHz(void);
void ReceiveEvent(uint8_t);
void EnableSlowOps(void);
void ResetMCU(void);

// Main function
int main(void) {
    /*  ___________________
       |                   | 
       |    Setup Block    |
       |___________________|
    */
    DisableWatchDog(); /* Disable watchdog to avoid continuous loop after reset */
    SetCPUSpeed1MHz(); /* Set prescaler = 1 (System clock / 1) */
    // Set output pins
    LED_DDR |= (1 << LED_PIN);   /* Set led control pin Data Direction Register for output */
    LED_PORT &= ~(1 << LED_PIN); /* Turn led off */
    _delay_ms(250);              /* Delay to allow programming at 1 MHz after power on */
    SetCPUSpeed8MHz();           /* Set the CPU prescaler for 8 MHz */
    // Initialize I2C
    p_receive_event = ReceiveEvent;    /* Pointer to TWI receive event function */
    p_enable_slow_ops = EnableSlowOps; /* Pointer to enable slow options function */
    UsiTwiDriverInit(TWI_ADDR);        /* NOTE: TWI_ADDR is defined in Makefile! */
    sei();                             /* Enable Interrupts */

    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */
    for (;;) {
        if (reset_now == true) {
            ResetMCU();
        }

        if (toggle_delay-- == 0) {
            if (blink == true) {
                LED_PORT ^= (1 << LED_PIN); /* Toggle PB1 */
            }
            toggle_delay = LONG_DELAY;
        }
    }
    return 0;
}

/*  ________________________
   |                        |
   | TWI data receive event |
   |________________________|
*/
void ReceiveEvent(uint8_t received_bytes) {
    for (uint8_t i = 0; i < received_bytes; i++) {
        command[i] = UsiTwiReceiveByte(); /* Store the data sent by the TWI master in the data buffer */
    }
    uint8_t opCodeAck = ~command[0]; /* Command Operation Code acknowledge => Command Bitwise "Not". */
    switch (command[0]) {
        // ******************
        // * SETIO1_1 Reply *
        // ******************
        case SETIO1_1: {
            LED_DDR |= (1 << LED_PIN);  /* Set led control pin Data Direction Register for output */
            LED_PORT |= (1 << LED_PIN); /* Turn PB1 on (Power control pin) */
            blink = true;
            UsiTwiTransmitByte(opCodeAck);
            break;
        }
        // ******************
        // * SETIO1_0 Reply *
        // ******************
        case SETIO1_0: {
            LED_DDR |= (1 << LED_PIN);   /* Set led control pin Data Direction Register for output */
            LED_PORT &= ~(1 << LED_PIN); /* Turn PB1 off (Power control pin) */
            blink = false;
            UsiTwiTransmitByte(opCodeAck);
            break;
        }
        // ******************
        // * RESETMCU Reply *
        // ******************
        case RESETMCU: {
            LED_PORT &= ~(1 << LED_PIN); /* Turn power off */
            UsiTwiTransmitByte(opCodeAck);
            reset_now = true;
            break;
        }
        // *************************
        // * Unknown Command Reply *
        // *************************
        default: {
            UsiTwiTransmitByte(UNKNOWNC);
            break;
        }
    }
}

/*  ________________________
   |                        |
   | Enable slow operations |
   |________________________|
*/
void EnableSlowOps(void) {
    slow_ops_enabled = true;
}

/*  __________________________
   |                          |
   | Function SetCPUSpeed1MHz |
   |__________________________|
*/
void SetCPUSpeed1MHz(void) {
    cli();                                   /* Disable interrupts */
    CLKPR = (1 << CLKPCE);                   /* Mandatory for setting prescaler */
    CLKPR = ((1 << CLKPS1) | (1 << CLKPS0)); /* Set prescaler 8 (System clock / 8) */
    sei();                                   /* Enable interrupts */
}

/*  __________________________
   |                          |
   | Function SetCPUSpeed8MHz |
   |__________________________|
*/
void SetCPUSpeed8MHz(void) {
    cli();                 /* Disable interrupts */
    CLKPR = (1 << CLKPCE); /* Mandatory for setting CPU prescaler */
    CLKPR = (0x00);        /* Set CPU prescaler 1 (System clock / 1) */
    sei();                 /* Enable interrupts */
}

/*  __________________________
   |                          |
   | Function DisableWatchDog |
   |__________________________|
*/
void DisableWatchDog(void) {
    MCUSR = 0;
    WDTCR = ((1 << WDCE) | (1 << WDE));
    WDTCR = ((1 << WDP2) | (1 << WDP1) | (1 << WDP0));
}

/*  ________________
   |                |
   | Function Reset |
   |________________|
*/
void ResetMCU(void) {
    LED_PORT &= ~(1 << LED_PIN); /* Turn led off */
    wdt_enable(WDTO_15MS);
    for (;;) {
    }
}
