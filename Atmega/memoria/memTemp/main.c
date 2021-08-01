/*
 * memTemp.c
 *
 * Created: 25/07/2021 09:57:00
 * Author : Humberto
 */ 


#define F_CPU 1000000UL // 1 MHz clock speed

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "I2CSlave.h"
#include "crc.h"
#include "eeprom.h"
#include "I2CMaster.h"
#include "DS1621.h"

// Atmel Slave variables
#define I2C_ADDR 0x20

volatile uint8_t command;
volatile uint8_t data;

uint8_t mean_counter = 1;
uint8_t data_counter = 0;


#define SR PIND2

#define MEMORY_SIZE 99

uint8_t count = 0x00;
uint8_t minuteflag = 0x00;
uint8_t maxTemp = 0x00;
uint8_t minTemp = 0xFF;
float promedio = 0x00;

uint8_t buffer_prom[6];
uint8_t buffer_raw[MEMORY_SIZE];

bool minute_flag=false;


void SR_Interrupt_init()
{
	EICRA = (1 << ISC00);
	EIMSK = (1 << INT0);
}

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



ISR(TIMER1_COMPA_vect)
{

	minute_flag=true;
	
}


//Atmel I2C Slave Interupts
void I2C_received(uint8_t received_data)
{
	command = received_data;
}

void I2C_requested()
{
	if (command == 0x1C) {
		if (mean_counter < 7){
			buffer_prom[mean_counter-1]=read_EEPROM(MEMORY_SIZE+mean_counter);
			I2C_transmitByte(buffer_prom[mean_counter-1]);
			mean_counter++;
		
		}
		else if(mean_counter==7){
			uint8_t crc_value;
			crc_value = crc_calculate(buffer_prom,6);
			I2C_transmitByte(crc_value);
			mean_counter++;
		}
		else{
			mean_counter=1;
		}
	} else if (command == 0x1B){
		
		if(data_counter<MEMORY_SIZE){
			
			buffer_raw[data_counter]=read_EEPROM(data_counter);
			I2C_transmitByte(buffer_raw[data_counter]);
			data_counter++;			
		}
		
		else if (data_counter == MEMORY_SIZE){
			uint8_t crc_value;
			crc_value = crc_calculate(buffer_raw,MEMORY_SIZE);
			I2C_transmitByte(crc_value);				
		}
		
		else{
			data_counter = 0;
		}

	}
	
}

ISR(INT0_vect){
	
	if ((PIND & (1 << SR)) == 2)
	{
		// set received/requested callbacks
		I2C_setCallbacks(I2C_received, I2C_requested);

		// init I2C
		I2C_init(I2C_ADDR);
		TIMSK1 &= ~(1<<OCIE1A);
		
	} else
	{
		TWI_init();
		TIMSK1 |= (1<<OCIE1A);
	}
	
}



int main(void)
{	
	TWI_init();
	DS1621_Init();
	
	
	DDRD |= (1 << MOSI) | (1 << CLK) | (1 << SELECT); // MOSI, CLK, SELECT(CS) SALIDAS
	DDRD &= ~((1 << PIND5) | (1<<SR)); // MISO ENTRADA	
	PORTD &= ~(1 << CLK); // clk = 0
		
	SR_Interrupt_init();

	// LEDs
	DDRD |= (1 << 6);
		
	sei();
	timerInit();
	
	// EEPROM start
	EWEN();	
	eraseAll();	
	EWDS();
	
	while (1)
	{
		
		if(minute_flag)
		{
			char temperature;
			uint8_t hundred_count;
			PORTD ^= (1<<PIND6);
			temperature = readTemperature(); // Leer temperatura
	
			uint8_t entero, decimal; // promedio
			float conthist; //
	
			hundred_count = read_EEPROM(MEMORY_SIZE); // Contador de cuantas veces se llego a 100 datos guardados
			conthist =  (((float)hundred_count*100) + (float)count); // contador historico
	
			if (hundred_count == 0 && count == 0)promedio = (float)temperature;
			else promedio = ((float)temperature + promedio*conthist )/ (conthist+1);
	
			entero = (uint8_t)promedio; // obtenemos el numero
			decimal = (promedio - (float)entero) * 100;
	
			EWEN();
	
			if (temperature > maxTemp){
				write_byte(MEMORY_SIZE+1, temperature); // Almacenando temperatura maxima
				write_byte(MEMORY_SIZE+2, conthist);
				maxTemp = temperature;
			}
	
			if (temperature < minTemp){
				write_byte(MEMORY_SIZE+3, temperature); // Almacenando temperatura minima
				write_byte(MEMORY_SIZE+4, conthist);    // Almacenando tiempo
				minTemp = temperature;
			}
	
			write_byte(MEMORY_SIZE+5,entero);
			write_byte(MEMORY_SIZE+6,decimal);
	
			write_byte(count, temperature); // Almacenar temperatura
	
			count++;
			if(count >= MEMORY_SIZE){
				count = 0;
				hundred_count++;
				write_byte(MEMORY_SIZE, hundred_count);
			}
			EWDS();

			minute_flag = false;
		}
		

	}
}
