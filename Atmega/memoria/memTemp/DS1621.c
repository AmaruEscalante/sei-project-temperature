
/*
 * DS1621.c
 *
 * Created: 8/1/2021 12:14:59 AM
 *  Author: vcave
 */ 

#include "DS1621.h"
#include <stdint.h>
#include "I2CMaster.h"

void DS1621_Init(void)
{
	TWI_Start();
	TWI_RegisterSelect(DS1621, ACCESS_CONFIG);
	TWI_Write(0x03); // LSB (1SHOT) set to 1 = 1-shot mode conversions and POL = 1
	
	TWI_Stop();
}

char readTemperature(void)
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



