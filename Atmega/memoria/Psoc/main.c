#include "I2CSlave.h"

#define I2C_ADDR 0x20

volatile uint8_t command;
volatile uint8_t data;

uint8_t mean_counter = 1;
uint8_t data_counter = 0;

uint8_t temp_data[3] = {0x30,0x40,0x50};

void I2C_received(uint8_t received_data)
{
	command = received_data;
}

void I2C_requested()
{
	if (command == 0x1C) {
		if (mean_counter == 7){
			mean_counter = 0;
		}
		I2C_transmitByte(read_EEPROM(MEMORY_SIZE+mean_counter));
		mean_counter++;
		} else if (command == 0x1B){
		if (data_counter == MEMORY_SIZE){
			data_counter = 0;
		}
		I2C_transmitByte(read_EEPROM(data_counter));
		data_counter++;
	}
}

void setup()
{
	// set received/requested callbacks
	I2C_setCallbacks(I2C_received, I2C_requested);

	// init I2C
	I2C_init(I2C_ADDR);
}

int main()
{
	setup();

	// Main program loop
	while(1);
}