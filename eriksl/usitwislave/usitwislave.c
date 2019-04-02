/*	See LICENSE for Copyright etc. */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include "usitwislave.h"
#include "usitwislave_devices.h"

#define always_inline __attribute__((always_inline))

enum {
    of_state_check_address,
    of_state_send_data,
    of_state_request_ack,
    of_state_check_ack,
    of_state_receive_data,
    of_state_store_data_and_send_ack
} overflow_state_t;

enum {
    ss_state_before_start,
    ss_state_after_start,
    ss_state_address_selected,
    ss_state_address_not_selected,
    ss_state_data_processed
} startstop_state_t;

enum {
    buffer_size = 32
};

static void (*idle_callback)(void);
static void (*data_callback)(uint8_t buffer_size,
                             uint8_t volatile input_buffer_length, const volatile uint8_t *input_buffer,
                             uint8_t volatile *output_buffer_length, uint8_t volatile *output_buffer);

static volatile uint8_t of_state;
static volatile uint8_t ss_state;

static volatile uint8_t slave_address;

static volatile uint8_t input_buffer[buffer_size];
static volatile uint8_t input_buffer_length;
static volatile uint8_t output_buffer[buffer_size];
static volatile uint8_t output_buffer_length;
static volatile uint8_t output_buffer_current;

static uint8_t stats_enabled;
static volatile uint16_t start_conditions_count;
static volatile uint16_t stop_conditions_count;
static volatile uint16_t error_conditions_count;
static volatile uint16_t overflow_conditions_count;
static volatile uint16_t local_frames_count;
static volatile uint16_t idle_call_count;

static always_inline void set_sda_to_input(void) {
    DDR_USI &= ~_BV(PORT_USI_SDA);
}

static always_inline void set_sda_to_output(void) {
    DDR_USI |= _BV(PORT_USI_SDA);
}

static always_inline void set_scl_to_input(void) {
    DDR_USI &= ~_BV(PORT_USI_SCL);
}

static always_inline void set_scl_to_output(void) {
    DDR_USI |= _BV(PORT_USI_SCL);
}

static always_inline void set_sda_low(void) {
    PORT_USI &= ~_BV(PORT_USI_SDA);
}

static always_inline void set_sda_high(void) {
    PORT_USI |= _BV(PORT_USI_SDA);
}

static always_inline void set_scl_low(void) {
    PORT_USI &= ~_BV(PORT_USI_SCL);
}

static always_inline void set_scl_high(void) {
    PORT_USI |= _BV(PORT_USI_SCL);
}

static always_inline void twi_reset_state(void) {
    USISR =
        (1 << USISIF) |     // clear start condition flag
        (1 << USIOIF) |     // clear overflow condition flag
        (0 << USIPF) |      // don't clear stop condition flag
        (1 << USIDC) |      // clear arbitration error flag
        (0x00 << USICNT0);  // set counter to "8" bits

    USICR =
        (1 << USISIE) |                                  // enable start condition interrupt
        (0 << USIOIE) |                                  // disable overflow interrupt
        (1 << USIWM1) | (0 << USIWM0) |                  // set usi in two-wire mode, disable bit counter overflow hold
        (1 << USICS1) | (0 << USICS0) | (0 << USICLK) |  // shift register clock source = external, positive edge, 4-bit counter source = external, both edges
        (0 << USITC);                                    // don't toggle clock-port pin
}

static always_inline void twi_reset(void) {
    // make sure no sda/scl remains pulled up or down

    set_sda_to_input();  //	deactivate internal pullup on sda/scl
    set_sda_low();
    set_scl_to_input();
    set_scl_low();

    set_sda_to_output();  //	release (set high) on sda/scl
    set_sda_high();
    set_sda_to_input();
    set_scl_to_output();
    set_scl_high();

    twi_reset_state();
}

static always_inline void twi_init(void) {
#if defined(USIPP)
#if defined(USI_ON_PORT_A)
    USIPP |= _BV(USIPOS);
#else
    USIPP &= ~_BV(USIPOS);
#endif
#endif

    twi_reset();
}

ISR(USI_START_vect) {
    set_sda_to_input();

    // wait for SCL to go low to ensure the start condition has completed (the
    // start detector will hold SCL low) - if a stop condition arises then leave
    // the interrupt to prevent waiting forever - don't use USISR to test for stop
    // condition as in Application Note AVR312 because the stop condition Flag is
    // going to be set from the last TWI sequence

    while (!(PIN_USI & _BV(PIN_USI_SDA)) &&
           (PIN_USI & _BV(PIN_USI_SCL)))

        // possible combinations
        //	sda = low	scl = low		break	start condition
        // 	sda = low	scl = high		loop
        //	sda = high	scl = low		break	stop condition
        //	sda = high	scl = high		break	stop condition

        if ((PIN_USI & _BV(PIN_USI_SDA)))  // stop condition
        {
            twi_reset();

            if (stats_enabled)
                error_conditions_count++;
            return;
        }

    if (stats_enabled)
        start_conditions_count++;

    of_state = of_state_check_address;
    ss_state = ss_state_after_start;

    USIDR = 0xff;

    USICR =
        (1 << USISIE) |                                  // enable start condition interrupt
        (1 << USIOIE) |                                  // enable overflow interrupt
        (1 << USIWM1) | (1 << USIWM0) |                  // set usi in two-wire mode, enable bit counter overflow hold
        (1 << USICS1) | (0 << USICS0) | (0 << USICLK) |  // shift register clock source = external, positive edge, 4-bit counter source = external, both edges
        (0 << USITC);                                    // don't toggle clock-port pin

    USISR =
        (1 << USISIF) |     // clear start condition flag
        (1 << USIOIF) |     // clear overflow condition flag
        (0 << USIPF) |      // don't clear stop condition flag
        (1 << USIDC) |      // clear arbitration error flag
        (0x00 << USICNT0);  // set counter to "8" bits
}

ISR(USI_OVERFLOW_VECTOR) {
    // bit shift register overflow condition occured
    // scl forced low until overflow condition is cleared!

    uint8_t data = USIDR;
    uint8_t set_counter = 0x00;  // send 8 bits (16 edges)

    if (stats_enabled)
        overflow_conditions_count++;

again:
    switch (of_state) {
            // start condition occured and succeed
            // check address, if not OK, reset usi
            // note: not using general call address

        case (of_state_check_address): {
            uint8_t address;
            uint8_t direction;

            direction = data & 0x01;
            address = (data & 0xfe) >> 1;

            if (address == slave_address) {
                ss_state = ss_state_address_selected;

                if (direction)  // read request from master
                    of_state = of_state_send_data;
                else  // write request from master
                    of_state = of_state_receive_data;

                USIDR = 0x00;
                set_counter = 0x0e;   // send 1 bit (2 edges)
                set_sda_to_output();  // initiate send ack
            } else {
                USIDR = 0x00;
                set_counter = 0x00;
                twi_reset_state();
                ss_state = ss_state_address_not_selected;
            }

            break;
        }

            // process read request from master

        case (of_state_send_data): {
            ss_state = ss_state_data_processed;
            of_state = of_state_request_ack;

            if (output_buffer_current < output_buffer_length)
                USIDR = output_buffer[output_buffer_current++];
            else
                USIDR = 0xfe;

            set_counter = 0x00;
            set_sda_to_output();  // initiate send data

            break;
        }

            // data sent to master, request ack (or nack) from master

        case (of_state_request_ack): {
            of_state = of_state_check_ack;

            USIDR = 0x00;
            set_counter = 0x0e;  //	receive 1 bit (2 edges)
            set_sda_to_input();  //	initiate receive ack

            break;
        }

            // ack/nack from master received

        case (of_state_check_ack): {
            if (data)  // if NACK, the master does not want more data
            {
                of_state = of_state_check_address;
                set_counter = 0x00;
                twi_reset();
            } else {
                of_state = of_state_send_data;
                goto again;  // from here we just drop straight into state_send_data
            }                // don't wait for another overflow interrupt

            break;
        }

            // process write request from master

        case (of_state_receive_data): {
            ss_state = ss_state_data_processed;

            of_state = of_state_store_data_and_send_ack;

            set_counter = 0x00;  // receive 1 bit (2 edges)
            set_sda_to_input();  // initiate receive data

            break;
        }

            // data received from master, store it and wait for more data

        case (of_state_store_data_and_send_ack): {
            of_state = of_state_receive_data;

            if (input_buffer_length < (buffer_size - 1))
                input_buffer[input_buffer_length++] = data;

            USIDR = 0x00;
            set_counter = 0x0e;   // send 1 bit (2 edges)
            set_sda_to_output();  // initiate send ack

            break;
        }
    }

    USISR =
        (0 << USISIF) |            // don't clear start condition flag
        (1 << USIOIF) |            // clear overflow condition flag
        (0 << USIPF) |             // don't clear stop condition flag
        (1 << USIDC) |             // clear arbitration error flag
        (set_counter << USICNT0);  // set counter to 8 or 1 bits
}

void usi_twi_slave(uint8_t slave_address_in, uint8_t use_sleep,
                   void (*data_callback_in)(uint8_t buffer_size,
                                            volatile uint8_t input_buffer_length, volatile const uint8_t *input_buffer,
                                            volatile uint8_t *output_buffer_length, volatile uint8_t *output_buffer),
                   void (*idle_callback_in)(void)) {
    slave_address = slave_address_in;
    data_callback = data_callback_in;
    idle_callback = idle_callback_in;

    input_buffer_length = 0;
    output_buffer_length = 0;
    output_buffer_current = 0;
    ss_state = ss_state_before_start;

    if (use_sleep)
        set_sleep_mode(SLEEP_MODE_IDLE);

    twi_init();

    sei();

    for (;;) {
        if (use_sleep && (ss_state == ss_state_before_start))
            sleep_mode();

        if (USISR & _BV(USIPF)) {
            cli();

            if (stats_enabled)
                stop_conditions_count++;

            USISR |= _BV(USIPF);  // clear stop condition flag

            switch (ss_state) {
                case (ss_state_after_start): {
                    twi_reset();
                    break;
                }

                case (ss_state_data_processed): {
                    if (stats_enabled)
                        local_frames_count++;

                    output_buffer_length = 0;
                    output_buffer_current = 0;

                    data_callback(buffer_size, input_buffer_length, input_buffer, &output_buffer_length, output_buffer);

                    input_buffer_length = 0;

                    break;
                }
            }

            ss_state = ss_state_before_start;

            sei();
        }

        if (idle_callback) {
            idle_callback();

            if (stats_enabled)
                idle_call_count++;
        }
    }
}

void usi_twi_enable_stats(uint8_t onoff) {
    stats_enabled = onoff;
    start_conditions_count = 0;
    stop_conditions_count = 0;
    error_conditions_count = 0;
    overflow_conditions_count = 0;
    local_frames_count = 0;
    idle_call_count = 0;
}

uint16_t usi_twi_stats_start_conditions(void) {
    return (start_conditions_count);
}

uint16_t usi_twi_stats_stop_conditions(void) {
    return (stop_conditions_count);
}

uint16_t usi_twi_stats_error_conditions(void) {
    return (error_conditions_count);
}

uint16_t usi_twi_stats_overflow_conditions(void) {
    return (overflow_conditions_count);
}

uint16_t usi_twi_stats_local_frames(void) {
    return (local_frames_count);
}

uint16_t usi_twi_stats_idle_calls(void) {
    return (idle_call_count);
}
