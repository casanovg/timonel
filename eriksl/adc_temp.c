#include <avr/io.h>

#include "adc_temp.h"

uint16_t adc_temperature_read(void)
{
	uint16_t result;

	DIDR0 = 	(0 << 7)		|	// disable digital inputs
				(0 << 6)		|
				(0 << ADC0D)	|
				(0 << ADC2D)	|
				(0 << ADC3D)	|
				(0 << ADC1D)	|
				(0 << 1)		|
				(0 << 0);

	ACSR =		(1 << ACD)		|	// disable comperator
				(0 << ACBG)		|	// bandgap select (n/a)
				(0 << ACO)		|	// enable analog comperator output
				(1 << ACI)		|	// clear comperator interrupt flag
				(0 << ACIE)		|	// enable analog comperator interrupt
				(0 << 2)		|	// reserved
				(0 << ACIS1)	|	// interrupt mode select (n/a)
				(0 << ACIS0);

	ADMUX	=	(1	<< REFS1)	|	// Vref = 010 = internal 1.1V reference
				(0	<< REFS0)	|
				(0	<< ADLAR)	|	// right adjust result
				(0	<< REFS2)	|
				(1	<< MUX3)	|
				(1	<< MUX2)	|	// input = 1111 = internal temperature sensor
				(1	<< MUX1)	|
				(1	<< MUX0);

	ADCSRB	=	(0	<< BIN)		|	// enable bipolair input
				(0	<< ACME)	|	// analog comperator multiplexer enable
				(0	<< IPR)		|	// input polarity reversal
				(0	<< 4)		|
				(0	<< 3)		|
				(0	<< ADTS2)	|
				(0	<< ADTS1)	|	// auto trigger source (n/a)
				(0	<< ADTS0);

	ADCSRA =	(1	<< ADEN)	|	// enable ADC
				(1	<< ADSC)	|	// start conversion
				(0	<< ADATE)	|	// auto trigger enable
				(1	<< ADIF)	|	// clear interrupt flag
				(0	<< ADIE)	|	// enable interrupt
				(1	<< ADPS2)	|
				(1	<< ADPS1)	|
				(0	<< ADPS0);		// select clock scaler 110 = 64

	while(ADCSRA & _BV(ADSC));

	ADCSRA =	(1	<< ADEN)	|	// enable ADC
				(1	<< ADSC)	|	// start conversion
				(0	<< ADATE)	|	// auto trigger enable
				(1	<< ADIF)	|	// clear interrupt flag
				(0	<< ADIE)	|	// enable interrupt
				(1	<< ADPS2)	|
				(1	<< ADPS1)	|
				(0	<< ADPS0);		// select clock scaler 110 = 64

	while(ADCSRA & _BV(ADSC));

	result = ADCL;
	result |= ADCH << 8;

	ADCSRA =	(0	<< ADEN)	|	// enable ADC
				(0	<< ADSC)	|	// start conversion
				(0	<< ADATE)	|	// auto trigger enable
				(1	<< ADIF)	|	// clear interrupt flag
				(0	<< ADIE)	|	// enable interrupt
				(1	<< ADPS2)	|
				(1	<< ADPS1)	|
				(0	<< ADPS0);		// select clock scaler 110 = 64

	return(result);
};
