This is a simple i2c/twi slave implementation using
the USI module found on several attiny models.

It has been loosely based on work by Atmel (Application
Note 312) and work by Donald Blake (several fixes to
the Atmel code, these are included in this code).

I have found a few other situations that would cause
mayhem and have been fixed:

 - Bus lockup after the master has written to a
	non-existent slave address.
 - Slave would receive bytes not addressed to it
	when a master would address two or more slaves
	after each other, without sending a stop
	condition in between ("repeated start").

Probably there are still other bugs / conditions left,
which I haven't run in yet...

Differences to existing code (by Atmel and Donald Blake):
 - Actually works, slave can reply data (as opposed to the
	Atmel version).
 - Completely different coding style (higher level, but
	keeping same compiler output) than Donald Blake's
	version, you may or may not like that ;-).
 - Compiles standalone to a library file (.a) and a header
	(.h) that can be included into several projects
	independantly. Please make sure you're using a version
	that was compiled for the device you're using (see Makefile).
 - Added support for a few newer attiny devices
 - Added support for USI bus on port A when using attiny*61
	devices (this is a library compile time option though,
	because it MUST work with #defines). Add -DUSI_ON_PORT_A
	in the Makefile to the "CFLAGS" section to enable it.
 - Last but not least: support for "complete" transactions,
 	i.e. start-data-stop and act upon it, instead of waiting
	for the next start condition. Due to poor design of
	USI, the stop condition can only be polled, so the mcu
	needs to busy wait on the stop condition flag. I've found
	a way to only do busy polling when it is necessary, i.e. 
	when a transaction is actually running. Otherwise,
	the mcu waits in sleep mode (if specified using use_sleep,
	see below).

Usage is quite simple: in your main loop, after all initialisations
are done, call usi_twi_slave(slave_address, use_sleep, data_callback,
idle_callback) where slave_address is you slave's requested address,
use_sleep is != 0 if it's okay to sleep when idle (save's a few
milliWatts, but causes the idle callback to be called rather less
frequent, don't use it if you depend on the idle callback) and callback
is a pointer to a function that is called when a stop condition occurs
after a valid transaction has been completed. Your callback should be
defined like this (see header file as well):

static void twi_callback(uint8_t buffer_size,
						volatile uint8_t input_buffer_length, volatile const uint8_t *input_buffer,
                        volatile uint8_t *output_buffer_length, volatile uint8_t *output_buffer)

buffer_size = the size of the internal input and output buffers,
	currently this is 16 bytes, but it may enlarged by recompiling
	the library. Do not write more bytes than the buffer_size or
	mayhem will be the result!
input_buffer_length = the amount of bytes received from the master
input_buffer = the bytes received from the master
output_buffer_length = the amount of bytes you want to send back to
	the master
output_buffer = the bytes you want to send back to the master

The input buffer is cleared after every valid transaction so you'll
never see the same bytes from the master twice. The output buffer
is cleared after all bytes are sent as well.

The idle_callback is an optional callback that will be called when
there is nothing to do (and we're not sleeping, see use_sleep),
it's defined as void idle_callback(void). If you're not using it,
specify NULL.