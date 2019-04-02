#if !defined(_IOPORTS_H_)
#define _IOPORTS_H_ 1
#include <stdint.h>
#include <avr/io.h>

typedef struct
{
	volatile	uint8_t		*port;
	volatile	uint8_t		*ddr;
				uint8_t		bit;
} ioport_t;

enum
{
	INPUT_PORTS		= 1,
};

extern const ioport_t input_ports[];

#endif
