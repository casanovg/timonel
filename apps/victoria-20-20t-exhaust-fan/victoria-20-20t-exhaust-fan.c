/*
 *  Roca Victoria 20/20T exhaust fan control
 *  Author: Gustavo Casanova / Nicebots
 *  ...........................................
 *  File: main.cpp (Application source)
 *  ........................................... 
 *  Version: 1.0 / 2019-07-12
 *  gustavo.casanova@nicebots.com
 *  ...........................................
 */

// Includes
#include "victoria-20-20t-exhaust-fan.h"

#define LONG_DELAY 0xFFFF

// Global variables
uint8_t command[32] = {0};           /* I2C Command received from master  */
uint8_t commandLength = 0;           /* I2C Command number of bytes  */

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
    DisableWatchDog();                  /* Disable watchdog to avoid continuous loop after reset */
    SetCPUSpeed1MHz();                  /* Set prescaler = 1 (System clock / 1) */
    // Set output pins
    LED_DDR |= (1 << LED_PIN);          /* Set led control pin Data Direction Register for output */
    LED_PORT &= ~(1 << LED_PIN);        /* Turn led off */
    FAN_PORT &= ~(1 << LED_PIN);        /* Turn fan on */    
    _delay_ms(250);                     /* Delay to allow programming at 1 MHz after power on */
    SetCPUSpeed8MHz();                  /* Set the CPU prescaler for 8 MHz */
    sei();                              /* Enable Interrupts */
    
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
