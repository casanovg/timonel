/*-------------------------------------*\
| TinyStepper Unipolar Stepper Driver	|
| ATTiny2313-based unipolar driver for	|
| 6-wire or 8-wire stepper motors.		|
|										|
| Serial and i2c interface				|
| 										|
| Adam Honse (amhb59@mail.mst.edu/		|
|			  calcprogrammer1@gmail.com)|
| 3/31/2012								|
\*-------------------------------------*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "serial.h"
#include "usi_i2c_slave.h"
#include "usi_i2c_master.h"

char			step_current;
char			step_state;
char			step_max;

char					step_mode;
char 					step_dir;
char 					step_enabled;
volatile int			step_count		= 0;
volatile int			step_compare	= 0;

char serial_i2c_buffer[8];
char serial_i2c_buffer_pos;

extern char usi_i2c_slave_address;
extern char* USI_Slave_register_buffer[];
//Normal
volatile char step_table[16] = {0b00001000, 0b00100000, 0b00010000, 0b01000000,

//Double Phase
							    0b00101000, 0b00110000, 0b01010000, 0b01001000,
								
//Half Stepping
								0b00001000, 0b00101000, 0b00100000, 0b00110000,
								
								0b00010000, 0b01010000, 0b01000000, 0b01001000};

inline void status_led_on();
inline void status_led_off();
inline void process_serial_message();

void set_rgb_led(char led);
void initialize_timer();
void process_i2c_message();
void fill_i2c_buffer_from_serial(char len, char addr, char rw);

inline void status_led_on()
{
	PORTD |= 0b00000100;
}

inline void status_led_off()
{
	PORTD &= 0b11111011;
}

void set_rgb_led(char led)
{
	led = led << 2;
	PORTB |= (led & 0b00011100);
	PORTB &= (led | 0b11100011);
}

void initialize_timer()
{
	//OCR1A - Timer 1 Compare A - Used for motor speed
	OCR1A = step_compare;

	//OCR2A/B - Timer 0 Compare A/B - Used for motor PWM
	OCR0A = 20;
	OCR0B = 100;

	//Reset step states
	step_current = 0x00000000;
	step_state = 0;
	step_mode = 0;
	step_dir = 1;
	step_enabled = 0;
	step_max = 3;

	//Enable output compare A interrupt for timers 0 and 1
	TIMSK = (1<<OCIE1A | 1<<OCIE0A | 1<<OCIE0B);

	TCCR1B = (1<<CS10 | 1<<CS11 | 0<<CS12);

	TCCR0B = (0<<CS00 | 1<<CS01 | 0<<CS02);
}

ISR(TIMER1_COMPA_vect)
{
	if(step_enabled == 1)
	{
		if(step_count > 0)
		{
			if(step_dir == 1)
			{
				set_rgb_led(0b00000010);
  				if(++step_state > step_max) step_state = 0;
			}
			else
			{
				set_rgb_led(0b00000001);
				if(--step_state < 0) step_state = step_max;
			}
		
			if(step_enabled == 1)
			{
				if(step_mode == 0)
				{
					 step_current = step_table[step_state];
					 step_max = 3;
				}
				else if(step_mode == 1)
				{
					step_current = step_table[step_state+4];
					step_max = 3;
				}
				else if(step_mode == 2)
				{
					step_current = step_table[step_state+8];
					step_max = 7;
				}
			}
			step_count--;
		}
		if(step_count == 0)
		{
			step_enabled = 0;
			set_rgb_led(0b00000100);
		}
	}
	else
	{
		set_rgb_led(0b00000100);
	}

	TCNT1 = 0;
}

ISR(TIMER0_COMPB_vect)
{
	PORTD &= 0b10000111;
	TCNT0 = 0;
}

ISR(TIMER0_COMPA_vect)
{	
	if(step_enabled == 1)
	{
		PORTD |= step_current;
	}
}

int main()
{
	DDRB = 0b00011100;
	DDRD = 0b01111100;

	sei();
	initialize_timer();
	serial_init(65);

	usi_i2c_slave_address = eeprom_read_byte((uint8_t*)1);
	if((usi_i2c_slave_address < 0x04) || (usi_i2c_slave_address > 0x77))
	{
		//If the stored address is out of range, default to 0x66.
		usi_i2c_slave_address = 0x66;
	}

	//USI_I2C_Init(usi_i2c_slave_address);

//master init
	DDR_USI  |= (1 << PORT_USI_SDA) | (1 << PORT_USI_SCL);
	PORT_USI |= (1 << PORT_USI_SCL);
	PORT_USI |= (1 << PORT_USI_SDA);
		USIDR = 0xFF;
		USICR = (0 << USISIE) | (0 << USIOIE) | (1 << USIWM1) | (0 << USIWM0) | (1 << USICS1) | (0 << USICS0) | (1 << USICLK) | (0 << USITC);
		USISR = (1 << USISIF) | (1 << USIOIF) | (1 << USIPF)  | (1 << USIDC)  | (0x00 << USICNT0);
	
	USI_Slave_register_buffer[0] = (unsigned char*)&step_compare;
	USI_Slave_register_buffer[1] = (unsigned char*)(&step_compare)+1;
	USI_Slave_register_buffer[2] = (unsigned char*)&step_count;
	USI_Slave_register_buffer[3] = (unsigned char*)(&step_count)+1;
	USI_Slave_register_buffer[4] = &step_dir;
	USI_Slave_register_buffer[5] = &step_mode;
	USI_Slave_register_buffer[6] = &step_enabled;
	USI_Slave_register_buffer[7] = &step_enabled;
	while(1)
	{
		if(step_compare != OCR1A)
		{
			OCR1A = step_compare;
		}
		process_serial_message();
	}
}

inline void process_serial_message()
{
	if(serial_available() > 2)
	{
		status_led_on();
		
		char buffer[3];
		serial_read_buffer(buffer, 3);

		switch(buffer[0])
		{		
			//Set I2C Address
			case 0x22:
				usi_i2c_slave_address = buffer[1];
				eeprom_write_byte((uint8_t*)1, usi_i2c_slave_address);
				break;

			//Read I2C Address
			case 0x23:
				serial_transmit_byte(usi_i2c_slave_address);
				break;

			//Send I2C Write
			case 0x24:
				fill_i2c_buffer_from_serial(buffer[1], buffer[2], 0);
				serial_transmit_byte(USI_I2C_Master_Start_Transmission(serial_i2c_buffer, buffer[1]+1));
				break;

			//Send I2C Read
			case 0x25:
				{
					char addr = buffer[2] << 1 | 1;
					serial_i2c_buffer[0] = addr;
					USI_I2C_Master_Start_Transmission(serial_i2c_buffer, buffer[1]+1);
					for(char i = 1; i <= buffer[1]; i++)
					{
						serial_transmit_byte(serial_i2c_buffer[i]);
					}
				}
				break;
		}
		status_led_off();	
	}
}


void fill_i2c_buffer_from_serial(char len, char addr, char rw)
{
	//Set R/W bit of address
	addr = addr << 1 | rw;

	//Put address into i2c buffer
	serial_i2c_buffer[0] = addr;

	for(char i = 1; i <= len; i++)
	{
		while(serial_available() < 1);
		serial_i2c_buffer[i] = serial_read();
	}
}
