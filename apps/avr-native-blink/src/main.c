/**
 * Copyright (C) PlatformIO <contact@platformio.org>
 * See LICENSE for details.
 */

#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    // make the LED pin an output for PORTB5
    DDRB = 1 << 1;

    while (1)
    {
        _delay_ms(100);

        // toggle the LED
        PORTB ^= 1 << 1;
    }

    return 0;
}
