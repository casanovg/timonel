#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#include <usitwislave.h>

#include "adc_temp.h"
#include "ioports.h"
#include "timer0_simple.h"

enum {
    LED_OFF_TIMEOUT = 64
};

typedef struct {
    uint32_t counter;
} counter_meta_t;

static counter_meta_t counters_meta[INPUT_PORTS];
static uint8_t slot;

static uint8_t led_timeout_command = 0;

static void put_word(uint16_t from, uint8_t *to) {
    to[0] = (from & 0xff00) >> 8;
    to[1] = (from & 0x00ff) >> 0;
}

static void put_long(uint32_t from, uint8_t *to) {
    to += 4;

    *(--to) = from & 0xff;
    from >>= 8;
    *(--to) = from & 0xff;
    from >>= 8;
    *(--to) = from & 0xff;
    from >>= 8;
    *(--to) = from & 0xff;
}

ISR(TIMER0_OVF_vect) {   // timer0 overflow
    if (led_timeout_command > 1)
        led_timeout_command--;
    else {
        if (led_timeout_command == 1) {
            PORTB &= ~_BV(4);
            led_timeout_command = 0;
        }
    }
}

ISR(PCINT0_vect) {
    slot = *input_ports[0].port & _BV(input_ports[0].bit);

    counters_meta[0].counter++;

    if (slot)
        PORTB &= ~_BV(1);
    else
        PORTB |= _BV(1);
}

static void build_reply(uint8_t volatile *output_buffer_length, volatile uint8_t *output_buffer,
                        uint8_t command, uint8_t error_code, uint8_t reply_length, const uint8_t *reply_string) {
    uint8_t checksum;
    uint8_t ix;

    output_buffer[0] = 3 + reply_length;
    output_buffer[1] = error_code;
    output_buffer[2] = command;

    for (ix = 0; ix < reply_length; ix++)
        output_buffer[3 + ix] = reply_string[ix];

    for (ix = 1, checksum = 0; ix < (3 + reply_length); ix++)
        checksum += output_buffer[ix];

    output_buffer[3 + reply_length] = checksum;
    *output_buffer_length = 3 + reply_length + 1;
}

static void extended_command(uint8_t buffer_size, volatile uint8_t input_buffer_length, const volatile uint8_t *input_buffer,
                             uint8_t volatile *output_buffer_length, volatile uint8_t *output_buffer) {
    uint8_t command = input_buffer[1];

    if (command < 5) {
        struct
        {
            uint8_t amount;
            uint8_t data[4];
        } control_info;

        switch (input_buffer[1]) {
            case (0x00):  // get digital inputs
            {
                control_info.amount = INPUT_PORTS;
                put_long(0x3fffffff, &control_info.data[0]);
                break;
            }

            case (0x01):  // get analog inputs
            {
                control_info.amount = 1;
                put_word(0x0000, &control_info.data[0]);
                put_word(0x03ff, &control_info.data[2]);
                break;
            }

            case (0x02):  // get digital outputs
            {
                control_info.amount = 0;
                put_word(0x0000, &control_info.data[0]);
                put_word(0x0000, &control_info.data[2]);
                break;
            }

            case (0x03):  // get pwm outputs
            {
                control_info.amount = 0;
                put_word(0x0000, &control_info.data[0]);
                put_word(0x0000, &control_info.data[2]);
                break;
            }

            default: {
                return (build_reply(output_buffer_length, output_buffer, input_buffer[0], 7, 0, 0));
            }
        }

        return (build_reply(output_buffer_length, output_buffer, input_buffer[0], 0, sizeof(control_info), (uint8_t *)&control_info));
    }

    return (build_reply(output_buffer_length, output_buffer, input_buffer[0], 7, 0, 0));
}

static void command_interpreter(uint8_t buffer_size, volatile uint8_t input_buffer_length, const volatile uint8_t *input_buffer,
                         uint8_t volatile *output_buffer_length, volatile uint8_t *output_buffer) {
    uint8_t input;
    uint8_t command;
    uint8_t io;

    PORTB |= _BV(4);
    led_timeout_command = LED_OFF_TIMEOUT;

    if (input_buffer_length < 1)
        return (build_reply(output_buffer_length, output_buffer, 0, 1, 0, 0));

    input = input_buffer[0];
    command = input & 0xf8;
    io = input & 0x07;

    switch (command) {
        case (0x00):  // short / no-io
        {
            switch (io) {
                case (0x00):  // identify
                {
                    struct
                    {
                        uint8_t id1, id2;
                        uint8_t model, version, revision;
                        uint8_t name[16];
                    } reply =
                        {
                            0x4a,
                            0xfb,
                            0x05,
                            0x01,
                            0x00,
                            "attiny85",
                        };

                    return (build_reply(output_buffer_length, output_buffer, input, 0, sizeof(reply), (uint8_t *)&reply));
                }

                case (0x01):  // read adc (temperature)
                {
                    uint16_t temp = adc_temperature_read();
                    uint8_t result[2];
                    put_word(temp, result);
                    return (build_reply(output_buffer_length, output_buffer, input, 0, sizeof(result), result));
                }

                case (0x07):  // extended command
                {
                    return (extended_command(buffer_size, input_buffer_length, input_buffer, output_buffer_length, output_buffer));
                }

                default: {
                    return (build_reply(output_buffer_length, output_buffer, input, 7, 0, 0));
                }
            }

            break;
        }

        case (0x10):  // 0x10 read counter
        case (0x20):  // 0x20 read / reset counter
        {
            if (io >= INPUT_PORTS)
                return (build_reply(output_buffer_length, output_buffer, input, 3, 0, 0));

            uint32_t counter = counters_meta[io].counter;

            if (command == 0x20)
                counters_meta[io].counter = 0;

            uint8_t replystring[4];
            put_long(counter, replystring);
            return (build_reply(output_buffer_length, output_buffer, input, 0, sizeof(replystring), replystring));
        }

        case (0x30):  //	read input
        {
            uint8_t value;

            if (io >= INPUT_PORTS)
                return (build_reply(output_buffer_length, output_buffer, input, 3, 0, 0));

            value = !!(*input_ports[io].port & (1 << input_ports[io].bit));

            return (build_reply(output_buffer_length, output_buffer, input, 0, 1, &value));
        }

        case (0xc0):  // select / read / start analog input (no-op)
        {
            if (io > 0)
                return (build_reply(output_buffer_length, output_buffer, input, 3, 0, 0));

            return (build_reply(output_buffer_length, output_buffer, input, 0, 0, 0));
        }

        case (0xf0):  // twi stats
        {
            uint8_t replystring[2];
            uint16_t stats;

            switch (io) {
                case (0x00):  //	disable
                {
                    usi_twi_enable_stats(0);
                    return (build_reply(output_buffer_length, output_buffer, input, 0, 0, 0));
                }

                case (0x01):  //	enable
                {
                    usi_twi_enable_stats(1);
                    return (build_reply(output_buffer_length, output_buffer, input, 0, 0, 0));
                }

                case (0x02):  //	read start conditions
                {
                    stats = usi_twi_stats_start_conditions();
                    break;
                }

                case (0x03):  //	read stop conditions
                {
                    stats = usi_twi_stats_stop_conditions();
                    break;
                }

                case (0x04):  //	read error conditions
                {
                    stats = usi_twi_stats_error_conditions();
                    break;
                }

                case (0x05):  //	read overflow conditions
                {
                    stats = usi_twi_stats_overflow_conditions();
                    break;
                }

                case (0x06):  //	read local frames
                {
                    stats = usi_twi_stats_local_frames();
                    break;
                }

                case (0x07):  //	read idle calls
                {
                    stats = usi_twi_stats_idle_calls();
                    break;
                }
            }

            put_word(stats, replystring);
            return (build_reply(output_buffer_length, output_buffer, input, 0, sizeof(replystring), replystring));
        }
        default: {
            return (build_reply(output_buffer_length, output_buffer, input, 2, 0, 0));
        }
    }

    return (build_reply(output_buffer_length, output_buffer, input, 2, 0, 0));
}

int main(void) {
    PCMSK = _BV(PCINT3);  //	PCINT on pb3
    GIMSK = _BV(PCIE);    //	enable PCINT

    PRR = (0 << 7) |
          (0 << 6) |  // reserved
          (0 << 5) |
          (0 << 4) |
          (1 << PRTIM1) |  // timer1
          (0 << PRTIM0) |  // timer0
          (0 << PRUSI) |   // usi
          (0 << PRADC);    // adc / analog comperator

    DDRB = _BV(1) | _BV(4);

    for (slot = 0; slot < INPUT_PORTS; slot++) {
        // *input_ports[slot].port |= _BV(input_ports[slot].bit);  // enable pullup
        counters_meta[slot].counter = 0;
    }

    PORTB |= _BV(4);
    led_timeout_command = LED_OFF_TIMEOUT;

    timer0_init(TIMER0_PRESCALER_64);
    timer0_start();

    usi_twi_slave(0x03, 1, command_interpreter, 0);

    return (0);
}
