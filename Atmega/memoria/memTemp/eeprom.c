
/*
 * eeprom.c
 *
 * Created: 8/1/2021 12:07:11 AM
 *  Author: vcave
 */ 

#include "eeprom.h"

// EEPROM Functions
void shift10bits(unsigned int data){
	PORTD |= (1 << SELECT); // señal  del selector en HIGH
	for(uint8_t count = 0; count < 10; count ++){ // bucle de 10 repeticiones
		PORTD &= ~(1 << CLK); // señal del clock en LOW
		if(data & 0x0200) PORTD |= (1 << MOSI); // escribimos el décimo bit  (0b 0000 00B0 0000 0000)
		else PORTD &= ~(1 << MOSI);
		PORTD |= (1 << CLK);  // señal del clock en HIGH
		data <<= 1;             // rotamos la data hacia la izquierda para enviar el siguiente bit
	}
	PORTD &= ~(1 << CLK); // señal del clock en LOW cuando acaba el for loop
	PORTD &= ~(1 << MOSI);
}

void shiftdata(uint8_t data_write){
	for(uint8_t count = 0; count < 8; count ++){
		PORTD &= ~(1 << CLK);  // señal del clock en LOW
		
		if(data_write & 0x80)PORTD |= (1 << MOSI);       // escribimos el octavo bit  (0b B000 0000)
		else PORTD &= ~(1 << MOSI);
		
		PORTD |= (1 << CLK);  // señal del clock en HIGH
		
		data_write <<= 1;               // rotamos la data hacia la izquierda para enviar el siguiente bit
	}
	PORTD &= ~(1 << CLK);
	PORTD &= ~(1 << MOSI);
}


uint8_t getOutput(uint8_t address_local){
	uint8_t response = 0x00;
	for(uint8_t i = 0; i < 8; i++){
		PORTD &= ~(1 << CLK);
		PORTD |= (1 << CLK);
		if(PIND & (1 << MISO))response |= 1 << (7-i);
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
	full_address = 1 << 9 | 10 << 7 | (address & 0b01111111);   // creamos el dato para enviar
	shift10bits(full_address); // Enviamos los 10 bits con el Op code y la direccion donde leer
	return getOutput(address); // Leemos los bits del miso
}

void write_byte(uint8_t address_write,uint8_t data_write){
	//Mascara para escribir 0b 0000 0010 1AAA AAAA
	unsigned int write_mask =  1 << 9 | 01 << 7  | (address_write & 0b01111111); // creamos el dato para enviar
	shift10bits(write_mask);    // Enviamos la data
	shiftdata(data_write);
	PORTD &= ~(1 << SELECT);         // Desactivamos el selector
	PORTD |= (1 << SELECT);          // Activamos el check status
	while((PIND && (1 << MISO)) == 0){};         // Bucle para esperar
	PORTD &= ~(1 << SELECT);         // Terminamos el check status
	
}

void EWEN()
{
	// x8
	// Start bit = 1
	// read op code = 00
	// Address A6-A0 A6=1 A5=1 A4-A0=X
	
	unsigned int data = 0x0260; // 0b 0000 0010 0110 0000
	shift10bits(data);        // Enviamos los 10 bits uno por uno con el orden de MSB
	PORTD &= ~(1 << SELECT);    // Desactivamos el selector
}

void EWDS()
{
	// Mode x8 , Start bit=1, Read OpCode=00, Address A6-A0 A6=0 A5=0
	
	unsigned int data = 0x0200; // 0b 0000 0010 0000 0000
	shift10bits(data);          // Send data to disable writing and erasing
	PORTD &= ~(1 << SELECT);    // Selector is disabled
}

void eraseAll(){
	unsigned int data = 0x0240; // 0b 0000 0010 0100 0000
	shift10bits(data);          // Send data to erase all
	PORTD &= ~(1 << SELECT);    // Selector is disabled
}

void writeAll(uint8_t data_write){
	//Mascara para escribir 0b 0000 0010 1AAA AAAA DDDD
	unsigned int write_mask =  1 << 9 | 00 << 7  |  01 << 5; // creamos el dato para enviar
	shift10bits(write_mask);
	shiftdata(data_write);
	PORTD &= ~(1 << SELECT);         // Desactivamos el selector
	PORTD |= (1 << SELECT);          // Activamos el check status
	while((PIND && (1 << MISO)) == 0){};         // Bucle para esperar
	PORTD &= ~(1 << SELECT);         // Terminamos el check status
	
}
