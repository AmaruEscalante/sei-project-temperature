
/*
 * I2CMaster.c
 *
 * Created: 8/1/2021 12:12:13 AM
 *  Author: vcave
 */ 

#include <avr/io.h>
#include <stdlib.h>
#include "I2CMaster.h"

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
	TWCR &= ~(1<<TWEA | (1<<TWIE)); // Clear TWEA and TWIE from slave mode
	TWCR |= (1<<TWEN); // Enable I2C protocol
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