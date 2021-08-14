/*
 *  Timonel - TWI Bootloader for ATtiny MCUs
 *  Author: Gustavo Casanova
 *  ...............................................
 *  File: clock-tweaking.h (Clock tweaking macros)
 *  ...............................................
 *  Version: 1.6 "Sandra" / 2021-08-10
 *  gustavo.casanova@gmail.com
 *  ...............................................
 */

/*
 *******************************************************************************
 * This piece of code is intended to be included at the beginning of main().   *
 * There are no reusable functions, it can't be run individually. It is in a   *
 * separate file to handle the speed setting complexity outside the main code. *
 *******************************************************************************
 */

#ifndef CLOCK_TWEAKING_H
#define CLOCK_TWEAKING_H

#if AUTO_CLK_TWEAK // Automatic clock tweaking made at run time, based on low fuse value

    //#pragma message "AUTO CLOCK TWEAKING SELECTED: Clock adjustments will be made at run time ..."
    uint8_t factory_osccal = OSCCAL;            // Preserve factory oscillator calibration
    if ((boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS) & 0x0F) == RCOSC_CLK_SRC) {
        // RC oscillator (8 MHz) clock source set in low fuse, calibrating oscillator up ...
        OSCCAL += OSC_FAST;                     // Speed oscillator up for TWI to work
    } else if ((boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS) & 0x0F) == HFPLL_CLK_SRC) {
        // HF PLL (16 MHz) clock source set in low fuse. No clock tweaking needed ...
    } else {
        // Unknown clock source set in low fuse! the prescaler will be reset to 1 to use the external clock as is
        ResetPrescaler();                       // If using an external CPU clock source, don't reduce its frequency
    }
    if (!((boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS) >> LFUSE_PRESC_BIT) & true)) {
        // Prescaler fuse bit set to divide clock by 8, setting the CPU prescaler division factor to 1
        ResetPrescaler();                       // Reset prescaler to divide by 1
    }

#else // Clock tweaking made at compile time, based on LOW_FUSE variable

    #define XSTR(x) STR(x)
    #define STR(x) #x

    //#pragma message "CLOCK TWEAKING AT COMPILE TIME BASED ON LOW_FUSE VARIABLE: " XSTR(LOW_FUSE)
    #if ((LOW_FUSE & 0x0F) == RCOSC_CLK_SRC)    // RC oscillator (8 MHz) clock source
        //#pragma message "RC oscillator (8 MHz) clock source selected, calibrating oscillator up ..."
        uint8_t factory_osccal = OSCCAL;        // With 8 MHz clock source, preserve factory oscillator
        OSCCAL += OSC_FAST;                     // calibration and speed it up for TWI to work.
    #elif ((LOW_FUSE & 0x0F) == HFPLL_CLK_SRC)  // HF PLL (16 MHz) clock source
        //#pragma message "HF PLL (16 MHz) clock source selected. No clock tweaking needed ..."
    #else // Unknown clock source
        //#pragma GCC warning "UNKNOWN LOW_FUSE CLOCK SETTING! VALID VALUES ARE 0xE1, 0x61, 0xE2 and 0x62"
        ResetPrescaler();  // If using an external CPU clock source, don't reduce its frequency
    #endif // LOW_FUSE CLOCK SOURCE

    #if ((LOW_FUSE & 0x80) == 0) // Prescaler dividing clock by 8
        //#pragma message "Prescaler dividing clock by 8, setting the CPU prescaler division factor to 1 ..."
        ResetPrescaler();                       // Reset prescaler to divide by 1
    #endif // LOW_FUSE PRESCALER BIT

#endif // AUTO_CLK_TWEAK

#endif // CLOCK_TWEAKING_H
