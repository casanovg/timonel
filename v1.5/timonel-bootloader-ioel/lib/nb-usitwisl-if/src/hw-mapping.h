/*
  hw-mapping.h
  =================================
  Library hardware mapping
  ---------------------------------
  Version: 1.0.2 / 2020-09-19
  ---------------------------------
*/

#ifndef HARDWARE_MAPPING_H
#define HARDWARE_MAPPING_H

// ATtinyX5
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
#endif

// ATtinyX4
#if defined(__AVR_ATtiny44__) | \
    defined(__AVR_ATtiny84__)
#define DDR_USI DDRA
#define PORT_USI PORTA
#define PIN_USI PINA
#define PORT_USI_SDA PORTA6
#define PORT_USI_SCL PORTA4
#define PIN_USI_SDA PINA6
#define PIN_USI_SCL PINA4
#define TWI_START_COND_FLAG USISIF  // This status register flag indicates that an I2C START condition occurred on the bus (can trigger an interrupt)
#define USI_OVERFLOW_FLAG USIOIF    // This status register flag indicates that the bits reception or transmission is complete (can trigger an interrupt)
#define TWI_STOP_COND_FLAG USIPF    // This status register flag indicates that an I2C STOP condition occurred on the bus
#define TWI_COLLISION_FLAG USIDC    // This status register flag indicates that a data output collision occurred on the bus
#define TWI_START_COND_INT USISIE   // This control register bit defines whether an I2C START condition will trigger an interrupt
#define USI_OVERFLOW_INT USIOIE     // This control register bit defines whether an USI 4-bit counter overflow will trigger an interrupt
#endif

#endif  // HARDWARE_MAPPING_H
