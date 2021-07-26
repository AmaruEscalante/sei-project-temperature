/*
 * main.c
 *
 * Created: 7/20/2021 10:02:52 PM
 *  Author: Alessio
 */ 

#define F_CPU 1000000UL // 1 MHz clock speed

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// DS1621 registers
#define DS1621 0x48 //DS1621 I2C address with A2, A1 and A0 connected to ground
#define ACCESS_CONFIG 0xAC
#define READ_TEMPERATURE 0xAA
#define ACCESS_TL 0xA2
#define ACCESS_TH 0xA1
#define START_CONVERT_T 0xEE
#define STOP_CONVERT_T 0x22

// TWI constants
#define ACK 1
#define NACK 0

void timerInit()
{
	// Configure Timer 1
	TCCR1A = 0x00; // Normal mode, OC1A/OC1B disconnected
	TCCR1B = (1<<WGM12)|(1<<CS12)|(1<<CS10); // CTC and prescaler 1024
	TIMSK1 |= (1<<OCIE1A);
	// formula: x seconds / 1024/10^6
	//OCR1A = 58594; // 60 seconds
	OCR1A = 4883; // 5 seconds
}

void Error()
{
	//PORTD = 0x02;
}

void Success()
{
	//PORTD = 0x04;
}

void TWI_init()
{
	TWCR = (1<<TWEN); // Enable I2C protocol
	TWSR = 0x00; // Prescaler = 1
	TWBR = 0x0C; // Bit Rate = 12 -> 1M/(16+2*12*1) = 25K
}

void TWI_Start()
{
	TWCR = (1<<TWINT) | (1<<TWEA) | (1<<TWSTA) | (1<<TWEN);
	while(!(TWCR&(1<<TWINT)));
	if((TWSR & 0xF8) != 0x08)
	{
		Error();
	}
	else
	{
		Success();
	}
}

void TWI_Repeat_Start()
{
	TWCR = (1<<TWINT) | (1<<TWEA) | (1<<TWSTA) | (1<< TWEN);
	while(!(TWCR&(1<<TWINT)));
	if((TWSR & 0xF8) != 0x10)
	{
		Error();
	}
	else
	{
		Success();
	}
}

void TWI_Stop()
{
	TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
}

void TWI_RegisterSelect(uint8_t addr, uint8_t reg)
{
	TWDR = (addr<<1) | 0x00; // Last bit = 0 (Write)
	TWCR = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR&(1<<TWINT)));
	if((TWSR & 0xF8) != 0x18)
	{
		Error();
	}
	else
	{
		Success();
		TWDR = reg; // Register to Write
		TWCR = (1<<TWINT) | (1<<TWEN);
		while(!(TWCR&(1<<TWINT)));
		if((TWSR & 0xF8) != 0x28)
		{
			Error();
		}
		else
		{
			Success();
		}
	}
}

int TWI_Read(uint8_t addr, uint8_t N_ACK)
{
	TWDR = (addr<<1) | 0x01; // Last bit = 1 (Read)
	TWCR = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR&(1<<TWINT)));
	if((TWSR & 0xF8) != 0x40)
	{
		Error();
	}
	else
	{
		Success();
		TWCR = (1<<TWINT) | (1<<TWEN) | (N_ACK<<TWEA);
		while(!(TWCR&(1<<TWINT)));

		if(N_ACK == 1) // Read Again
		{
			if((TWSR & 0xF8) != 0x50)
			{
				Error();
			}
			else
			{
				Success();
			}

		}
		else
		{
			if((TWSR & 0xF8) != 0x58)
			{
				Error();
			}
			else
			{
				Success();
			}
		}
	}

	return(TWDR);

}

void TWI_Write(uint8_t data)
{
	TWDR = data; // Write data on previous selected register
	TWCR = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR&(1<<TWINT)));
	if((TWSR & 0xF8) != 0x28)
	{
		Error();
	}
	else
	{
		//Success();
	}

}

void DS1621_Init()
{
	TWI_Start();
	TWI_RegisterSelect(DS1621, ACCESS_CONFIG);
	TWI_Write(0x03); // LSB (1SHOT) set to 1 = 1-shot mode conversions and POL = 1
	
	TWI_Stop();
}

char readTemperature()
{
	char temperatureMSB;
	char temperatureLSB;
	
	TWI_Start();
	TWI_RegisterSelect(DS1621, START_CONVERT_T);
	// No further data is required
	
	TWI_Repeat_Start();
	TWI_RegisterSelect(DS1621, READ_TEMPERATURE);
	TWI_Repeat_Start();
	temperatureMSB = TWI_Read(DS1621, NACK); // ACK not required
	temperatureLSB = TWI_Read(DS1621, NACK); // ACK not required
	
	TWI_Stop();
	
	return temperatureMSB;
}

ISR(TIMER1_COMPA_vect)
{
	//char temperature;
	//temperature = readTemperature();
	PORTD ^= (1 << PIND6);
}

int main(void)
{	
	//TWI_init();
	//DS1621_Init();
	
	// LEDs
	DDRD = 0xFF;
	DDRD &= ~(1 << 0);
	DDRD &= ~(1 << 5);
		
	
	sei();
	timerInit();
	
	PORTD ^= (1 << PIND6);
    while(1)
    {
        //TODO:: Please write your application code
		PORTD ^= (1 << PIND7);
		_delay_ms(1000); //1 second delay
    }
}
