/*
 *  Roca Victoria 20/20T exhaust fan control
 *  Author: Gustavo Casanova / Nicebots
 *  ......................................................
 *  File: main.cpp (Application source)
 *  ......................................................
 *  Version: 1.0 / 2019-07-12
 *  gustavo.casanova@nicebots.com
 *  ......................................................
 *  This program for ATtiny85 microcontrollers replaces
 *  the factory mechanism of a Roca Victoria 20/20T
 *  (or F) to turn the exhaust fan on and off before and
 *  after a hot water request by the DHW or CH functions.
 *  The original mechanism was damaged by a relay that,
 *  after failing, burned the microcontroller output pin
 *  that handled the fan switching.
 */

// Includes
#include "victoria-20-20t-exhaust-fan.h"

#define LONG_DELAY 0xFFFF
#define START_DELAY 0x07
#define POWER_ON_DELAY 0x85

// Global variables
uint8_t command[32] = {0};           /* I2C Command received from master  */
uint8_t commandLength = 0;           /* I2C Command number of bytes  */

bool slow_ops_enabled = false;
volatile bool blink = true;
volatile uint16_t long_delay = LONG_DELAY;
volatile uint8_t power_on_delay = POWER_ON_DELAY;
volatile uint8_t start_delay = START_DELAY;

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
    DisableWatchDog();                  /* Disable watchdog to avoid continuous loop after reset */
    SetCPUSpeed1MHz();                  /* Set prescaler = 1 (System clock / 1) */
    // Set output pins
    LED_DDR |= (1 << LED_PIN);          /* Set led control pin Data Direction Register for output */
    LED_PORT &= ~(1 << LED_PIN);        /* Turn led off */
    FAN_DDR |= (1 << FAN_PIN);          /* Set fan control pin Data Direction Register for output */
    FAN_PORT |= (1 << FAN_PIN);         /* Turn fan off (inverse logic: high = off, low = on) */
    // Set input pins
    DHW_DDR &= ~(1 << DHW_PIN);         /* Set DHW pin Data Direction Register for input */
    //DHW_PORT |= (1 << DHW_PIN)        /* Enable pull-up resistor on DHW pin */
    DHW_PORT &= ~(1 << DHW_PIN);        /* Set DHW pin in high-impedance mode */
    CEH_DDR &= ~(1 << CEH_PIN);         /* Set CEH pin Data Direction Register for input */
    //CEH_PORT |= (1 << CEH_PIN);       /* Enable pull-up resistor on CEH pin */
    CEH_PORT &= ~(1 << CEH_PIN);        /* Set CEH pin in high-impedance mode */    
    _delay_ms(250);                     /* Delay to allow programming at 1 MHz after power on */
    SetCPUSpeed8MHz();                  /* Set the CPU prescaler for 8 MHz */
    sei();                              /* Enable Interrupts */
    
    // Power on fan test
    FAN_PORT &= ~(1 << FAN_PIN);    /* Turn fan on (inverse logic: high = off, low = on) */    
    while (long_delay-- > 0) {
        if (blink == true) {
            LED_PORT ^= (1 << LED_PIN);     /* Toggle LED */
        }         
        while (power_on_delay-- > 0) {}
        power_on_delay = POWER_ON_DELAY;
    }
    long_delay = LONG_DELAY;
    LED_PORT &= ~(1 << LED_PIN);        /* Turn led off */    
    FAN_PORT |= (1 << FAN_PIN);     /* Turn fan off (inverse logic: high = off, low = on) */
    
    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */
    for (;;) {
        if (((DHW_INP & (1 << DHW_PIN)) == 0) || ((CEH_INP & (1 << CEH_PIN)) == 0)) {
            if (long_delay-- == 0) {
                if (blink == true) {
                    LED_PORT ^= (1 << LED_PIN);     /* Toggle LED */
                }                
                if (start_delay-- == 0) {
                    FAN_PORT &= ~(1 << FAN_PIN);    /* Turn fan on (inverse logic: high = off, low = on) */
                    start_delay = START_DELAY;
                }
                long_delay = LONG_DELAY;
            }  
        } else {
            LED_PORT &= ~(1 << LED_PIN);            /* Turn led off */
            FAN_PORT |= (1 << FAN_PIN);             /* Turn fan off (inverse logic: high = off, low = on) */
            start_delay = START_DELAY;
            long_delay = LONG_DELAY;            
        }
    }
    return 0;
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
    LED_PORT &= ~(1 << LED_PIN);     /* Turn led off */
    wdt_enable(WDTO_15MS);
    for (;;) {}
}
