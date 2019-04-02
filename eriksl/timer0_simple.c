#include <stdint.h>
#include <avr/io.h>

#include "timer0_simple.h"

static uint8_t cs0[3];

void timer0_init(uint8_t prescaler)
{
	if(prescaler > 5)
		return;

	uint8_t temp, mask;

	cs0[0] = (prescaler & (1 << 0)) >> 0;
	cs0[1] = (prescaler & (1 << 1)) >> 1;
	cs0[2] = (prescaler & (1 << 2)) >> 2;

	timer0_stop();

	GTCCR =		(0 << TSM)		|	// synchronisation mode
				(0 << 6)		|
				(0 << 5)		|
				(0 << 4)		|
				(0 << 3)		|
				(0 << 2)		|
				(0 << 1)		|
				(0 << PSR0);		// prescaler reset

	TCCR0A =	(0 << COM0A1)	|	// compare match output a mode = off
				(0 << COM0A0)	|
				(0 << COM0B1)	|	// compare match output b mode = off
				(0 << COM0B0)	|
				(0 << 3)		|
				(0 << 2)		|
				(0 << WGM01)	|	// WGM02:WGM00 = 000 = normal, top is 0xff
				(0 << WGM00);

	TCCR0B =	(0 << FOC0A)	|	// force compare output a
				(0 << FOC0B)	|	// force compare output b
				(0 << 5)		|
				(0 << 4)		|
				(0 << WGM02)	|	
				(0 << CS02)		|	// stop clock
				(0 << CS01)		|
				(0 << CS00);

	mask =		(0 << 7)		|
				(0 << OCIE1A)	|	// enable output compare match 1 a interrupt
				(0 << OCIE1B)	|	// enable output compare match 1 b interrupt
				(1 << OCIE0A)	|	// enable output compare match 0 a interrupt
				(1 << OCIE0B)	|	// enable output compare match 0 b interrupt
				(0 << TOIE1)	|	// enable timer 1 overflow interrupt
				(1 << TOIE0)	|	// enable timer 0 overflow interrupt
				(0 << 0);

	temp = TIMSK & ~mask;

	temp |=		(0 << 7)		|
				(0 << OCIE1A)	|	// enable output compare match 1 a interrupt
				(0 << OCIE1B)	|	// enable output compare match 1 b interrupt
				(0 << OCIE0A)	|	// enable output compare match 0 a interrupt
				(0 << OCIE0B)	|	// enable output compare match 0 b interrupt
				(0 << TOIE1)	|	// enable timer 1 overflow interrupt
				(1 << TOIE0)	|	// enable timer 0 overflow interrupt
				(0 << 0);

	TIMSK = temp;

	TIFR =		(0 << 7)		|
				(0 << OCF1A)	|	// clear output compare flag 1 a
				(0 << OCF1B)	|	// clear output compare flag 1 b
				(1 << OCF0A)	|	// clear output compare flag 0 a
				(1 << OCF0B)	|	// clear output compare flag 0 b
				(0 << TOV1)		|	// clear timer 1 overflow flag
				(1 << TOV0)		|	// clear timer 0 overflow flag
				(0 << 0);
}

void timer0_start(void)
{
	timer0_stop();
	timer0_reset_counter();

	TCCR0B =	(0 << FOC0A)		|	// force compare output a
				(0 << FOC0B)		|	// force compare output b
				(0 << 5)			|
				(0 << 4)			|
				(0 << WGM02)		|	
				(cs0[2] << CS02)	|	// start clock
				(cs0[1] << CS01)	|
				(cs0[0] << CS00);
}

void timer0_stop(void)
{
	TCCR0B =	(0 << FOC0A)		|	// force compare output a
				(0 << FOC0B)		|	// force compare output b
				(0 << 5)			|
				(0 << 4)			|
				(0 << WGM02)		|	
				(0 << CS02)			|	// stop clock
				(0 << CS01)			|
				(0 << CS00);
}

uint8_t timer0_status(void)
{
	return((TCCR0B & (_BV(CS02) | _BV(CS01) | _BV(CS00))) >> CS00);
}
