/*
 * memTemp.c
 *
 * Created: 25/07/2021 09:57:00
 * Author : Humberto
 */

#include "I2CSlave.h"
#include "crc.h"

#define F_CPU 1000000UL // 1 MHz clock speed

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

// Atmel Slave variables
#define I2C_ADDR 0x20

volatile uint8_t command;
volatile uint8_t data;

uint8_t mean_counter = 1;
uint8_t data_counter = 0;

#define SR PIND2

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

// SPI for EEPROM constants
#define MOSI PIND4
#define MISO PIND5
#define SELECT PIND6
#define CLK PIND3

#define MEMORY_SIZE 99

uint8_t count = 0x00;
uint8_t minuteflag = 0x00;
uint8_t maxTemp = 0x00;
uint8_t minTemp = 0xFF;
float promedio = 0x00;

uint8_t buffer_prom[6];
uint8_t buffer_raw[MEMORY_SIZE];

void SR_Interrupt_init()
{
	// Habilita la interrupcion del PIN D2
	EICRA = (1 << ISC00);
	EIMSK = (1 << INT0);
}

// EEPROM Functions
void shift10bits(unsigned int data)
{
	PORTD |= (1 << SELECT); // se�al  del selector en HIGH
	for (uint8_t count = 0; count < 10; count++)
	{						  // bucle de 10 repeticiones
		PORTD &= ~(1 << CLK); // se�al del clock en LOW
		if (data & 0x0200)
			PORTD |= (1 << MOSI); // escribimos el d�cimo bit  (0b 0000 00B0 0000 0000)
		else
			PORTD &= ~(1 << MOSI);
		PORTD |= (1 << CLK);					 // se�al del clock en HIGH
		while (PORTD & (1 << CLK) != (1 << CLK)) // se�al del clock en HIGH
		{
			PORTD |= (1 << CLK);
		}
		data <<= 1; // rotamos la data hacia la izquierda para enviar el siguiente bit
	}
	PORTD &= ~(1 << CLK); // se�al del clock en LOW cuando acaba el for loop
	PORTD &= ~(1 << MOSI);
}

void shiftdata(uint8_t data_write)
{
	for (uint8_t count = 0; count < 8; count++)
	{
		PORTD &= ~(1 << CLK); // se�al del clock en LOW

		if (data_write & 0x80)
			PORTD |= (1 << MOSI); // escribimos el octavo bit  (0b B000 0000)
		else
			PORTD &= ~(1 << MOSI);

		PORTD |= (1 << CLK);

		while (PORTD & (1 << CLK) != (1 << CLK)) // se�al del clock en HIGH
		{
			PORTD |= (1 << CLK);
		}

		data_write <<= 1; // rotamos la data hacia la izquierda para enviar el siguiente bit
	}
	PORTD &= ~(1 << CLK);
	PORTD &= ~(1 << MOSI);
}

uint8_t getOutput(uint8_t address_local)
{
	uint8_t response = 0x00;
	for (uint8_t i = 0; i < 8; i++)
	{
		PORTD &= ~(1 << CLK);
		PORTD |= (1 << CLK);
		while (PORTD & (1 << CLK) != (1 << CLK))
		{
			PORTD |= (1 << CLK);
		}

		if (PIND & (1 << MISO))
			response |= 1 << (7 - i);
	}
	PORTD &= ~(1 << CLK);
	PORTD &= ~(1 << SELECT);
	return response;
}

uint8_t read_EEPROM(uint8_t address)
{
	//start bit = 1
	//read op code = 10
	//Address A6-A0
	unsigned int full_address = 0x0000;
	full_address = 1 << 9 | 10 << 7 | (address & 0b01111111); // creamos el dato para enviar
	shift10bits(full_address);								  // Enviamos los 10 bits con el Op code y la direccion donde leer
	return getOutput(address);								  // Leemos los bits del miso
}

void write_byte(uint8_t address_write, uint8_t data_write)
{
	//Mascara para escribir 0b 0000 0010 1AAA AAAA
	unsigned int write_mask = 1 << 9 | 01 << 7 | (address_write & 0b01111111); // creamos el dato para enviar
	shift10bits(write_mask);												   // Enviamos la data
	shiftdata(data_write);
	PORTD &= ~(1 << SELECT); // Desactivamos el selector
	PORTD |= (1 << SELECT);	 // Activamos el check status
	while ((PIND & (1 << MISO)) != (1 << MISO))
	{
	}; // Bucle para esperar

	PORTD &= ~(1 << SELECT); // Terminamos el check status
}

void EWEN()
{
	// x8
	// Start bit = 1
	// read op code = 00
	// Address A6-A0 A6=1 A5=1 A4-A0=X

	unsigned int data = 0x0260; // 0b 0000 0010 0110 0000
	shift10bits(data);			// Enviamos los 10 bits uno por uno con el orden de MSB
	PORTD &= ~(1 << SELECT);	// Desactivamos el selector
}

void EWDS()
{
	// Mode x8 , Start bit=1, Read OpCode=00, Address A6-A0 A6=0 A5=0

	unsigned int data = 0x0200; // 0b 0000 0010 0000 0000
	shift10bits(data);			// Send data to disable writing and erasing
	PORTD &= ~(1 << SELECT);	// Selector is disabled
}

void eraseAll()
{
	unsigned int data = 0x0240; // 0b 0000 0010 0100 0000
	shift10bits(data);			// Send data to erase all
	PORTD &= ~(1 << SELECT);	// Selector is disabled
}

void writeAll(uint8_t data_write)
{
	//Mascara para escribir 0b 0000 0010 1AAA AAAA DDDD
	unsigned int write_mask = 1 << 9 | 00 << 7 | 01 << 5; // creamos el dato para enviar
	shift10bits(write_mask);
	shiftdata(data_write);
	PORTD &= ~(1 << SELECT); // Desactivamos el selector
	PORTD |= (1 << SELECT);	 // Activamos el check status
	while ((PIND & (1 << MISO)) != (1 << MISO))
	{
	};						 // Bucle para esperar
	PORTD &= ~(1 << SELECT); // Terminamos el check status
}

void timerInit()
{
	// Configure Timer 1
	TCCR1A = 0x00;									   // Normal mode, OC1A/OC1B disconnected
	TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC and prescaler 1024
	TIMSK1 |= (1 << OCIE1A);
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
	TWCR &= ~(1 << TWEA | (1 << TWIE)); // Clear TWEA and TWIE from slave mode
	TWCR |= (1 << TWEN);				// Enable I2C protocol
	// Setea la velocidad de comunicacion
	TWSR = 0x00; // Prescaler = 1
	TWBR = 0x0C; // Bit Rate = 12 -> 1M/(16+2*12*1) = 25K
}

void TWI_Start()
{
	TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
		;
	if ((TWSR & 0xF8) != 0x08)
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
	TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
		;
	if ((TWSR & 0xF8) != 0x10)
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
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void TWI_RegisterSelect(uint8_t addr, uint8_t reg)
{
	TWDR = (addr << 1) | 0x00; // Last bit = 0 (Write)
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
		;
	if ((TWSR & 0xF8) != 0x18)
	{
		Error();
	}
	else
	{
		Success();
		TWDR = reg; // Register to Write
		TWCR = (1 << TWINT) | (1 << TWEN);
		while (!(TWCR & (1 << TWINT)))
			;
		if ((TWSR & 0xF8) != 0x28)
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
	TWDR = (addr << 1) | 0x01; // Last bit = 1 (Read)
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
		;
	if ((TWSR & 0xF8) != 0x40)
	{
		Error();
	}
	else
	{
		Success();
		TWCR = (1 << TWINT) | (1 << TWEN) | (N_ACK << TWEA);
		while (!(TWCR & (1 << TWINT)))
			;

		if (N_ACK == 1) // Read Again
		{
			if ((TWSR & 0xF8) != 0x50)
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
			if ((TWSR & 0xF8) != 0x58)
			{
				Error();
			}
			else
			{
				Success();
			}
		}
	}

	return (TWDR);
}

void TWI_Write(uint8_t data)
{
	TWDR = data; // Write data on previous selected register
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
		;
	if ((TWSR & 0xF8) != 0x28)
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

void readhundredtemp(uint8_t temp);
void readmaxminprom();

ISR(TIMER1_COMPA_vect)
{
	minuteflag = 0x01;
	char temperature;
	uint8_t hundred_count;
	//PORTD ^= (1 << PIND6);
	temperature = readTemperature(); // Leer temperatura

	uint8_t entero, decimal; // promedio
	float conthist;			 //

	hundred_count = read_EEPROM(MEMORY_SIZE);				  // Contador de cuantas veces se llego a 100 datos guardados
	conthist = (((float)hundred_count * 100) + (float)count); // contador historico

	if (hundred_count == 0 && count == 0)
		promedio = (float)temperature;
	else
		promedio = ((float)temperature + promedio * conthist) / (conthist + 1);

	entero = (uint8_t)promedio; // obtenemos el numero
	decimal = (promedio - (float)entero) * 100;

	EWEN();

	if (temperature > maxTemp)
	{
		write_byte(MEMORY_SIZE + 1, temperature); // Almacenando temperatura maxima
		write_byte(MEMORY_SIZE + 2, conthist);
		// write_byte(MEMORY_SIZE + 2, 0x0A);
		maxTemp = temperature;
	}

	if (temperature < minTemp)
	{
		write_byte(MEMORY_SIZE + 3, temperature); // Almacenando temperatura minima
		write_byte(MEMORY_SIZE + 4, conthist);	  // Almacenando tiempo
		// write_byte(MEMORY_SIZE + 4, 0xA0); // Almacenando tiempo
		minTemp = temperature;
	}

	write_byte(MEMORY_SIZE + 5, entero);
	write_byte(MEMORY_SIZE + 6, decimal);

	write_byte(count, temperature); // Almacenar temperatura

	count++;
	if (count >= MEMORY_SIZE)
	{
		count = 0;
		hundred_count++;
		write_byte(MEMORY_SIZE, hundred_count);
	}
	EWDS();
}

//Atmel I2C Slave Interupts
void I2C_received(uint8_t received_data)
{
	command = received_data;
}

void I2C_requested()
{
	if (command == 0x1C)
	{
		if (mean_counter < 7)
		{
			buffer_prom[mean_counter - 1] = read_EEPROM(MEMORY_SIZE + mean_counter);
			I2C_transmitByte(buffer_prom[mean_counter - 1]);
			mean_counter++;
		}
		else if (mean_counter >= 7)
		{
			uint8_t crc_value;
			crc_value = crc_calculate(buffer_prom, 6);
			I2C_transmitByte(crc_value);
			mean_counter = 1;
		}
	}
	else if (command == 0x1B)
	{

		if (data_counter < MEMORY_SIZE)
		{

			buffer_raw[data_counter] = read_EEPROM(data_counter);
			I2C_transmitByte(buffer_raw[data_counter]);
			data_counter++;
		}

		else if (data_counter >= MEMORY_SIZE)
		{
			uint8_t crc_value;
			crc_value = crc_calculate(buffer_raw, MEMORY_SIZE);
			I2C_transmitByte(crc_value);
			data_counter = 0;
		}
	}
}

ISR(INT0_vect)
{
	if ((PIND & (1 << SR)) == (1 << SR)) // Changes Atmega to Slave mode
	{
		// Turns on pind7
		PORTD |= (1 << PIND7);
		// set received/requested callbacks
		I2C_setCallbacks(I2C_received, I2C_requested);

		// init I2C
		I2C_init(I2C_ADDR);
		// Disable timer 1
		TIMSK1 &= ~(1 << OCIE1A);
	}
	else // Changes Atmega to Master mode
	{
		TWI_init();
		TIMSK1 |= (1 << OCIE1A);
	}
}

int main(void)
{
	TWI_init(); // Inicia I2C Master
	DS1621_Init();
	SR_Interrupt_init();

	DDRD |= (1 << MOSI) | (1 << CLK) | (1 << SELECT); // MOSI, CLK, SELECT(CS) SALIDAS
	DDRD &= ~(1 << MISO);							  // MISO ENTRADA

	// Sets pin d7 as output
	DDRD |= (1 << PIND7);
	// Turns on pind7

	PORTD &= ~(1 << CLK); // clk = 0

	// LEDs
	DDRD |= (1 << 6);

	sei();
	timerInit();

	// Inicializa los valores de la EEPROM en 0
	EWEN();
	writeAll(0x00);
	EWDS();

	while (1)
	{

		if (minuteflag == 0x01)
		{

			minuteflag = 0x00;
		}

		//_delay_ms(1000);
		//readhundredtemp();
	}
}

void readhundredtemp(uint8_t temp)
{
	// Recieve last 100 temperature
	char temperature;
	uint8_t contar = temp;

	// verificar si ya tiene 100 datos
	//contar = read_EEPROM(MEMORY_SIZE); // direccion del contador de cuantas veces se llego a 100 datos
	if (contar > 0)
	{
		// If the memory have already saved more than 100 data.

		// Elegir entre ultimo a reciente o de reciente a ultimo valor guardado
		contar = (count + 1 >= MEMORY_SIZE) ? 0 : count + 1; // de mas antiguo a reciente
		//contar = ((count - 1 < 0) || (count - 1 >= MEMORY_SIZE))? MEMORY_SIZE - 1: count; // de mas reciente a antiguo

		for (uint8_t i = 0; i < MEMORY_SIZE; i++)
		{
			temperature = read_EEPROM(contar);

			// Write program to use temperature;

			// Elegir entre ultimo a reciente o de reciente a ultimo valor guardado
			contar = (contar + 1 >= MEMORY_SIZE) ? 0 : contar + 1; // de mas antiguo a reciente
																   //contar = ((contar - 1 < 0) || (contar - 1 >= MEMORY_SIZE))? MEMORY_SIZE - 1: count - 1; // de mas reciente a antiguo
		}
	}
	else
	{
		// If there aren't 100 data, read all temperature saved until the moment

		contar = 0; // antiguo a reciente
		//contar = count; // reciente a antiguo

		for (uint8_t i = 0; i < count; i++)
		{
			temperature = read_EEPROM(contar);

			// Write program to use temperature

			contar++; // antiguo a reciente
					  //contar--;  // reciente a antiguo
		}
	}
}