#include <stdint.h>
#include <avr/io.h>

#include "ioports.h"

const ioport_t input_ports[INPUT_PORTS] =
{
	{ &PINB, &DDRB, 3 },
};
