/*
 * memoria.c
 *
 * Created: 21/07/2021 01:00:08
 * Author : Humberto
 */ 

#define F_CPU 1000000UL // 1 MHz clock speed

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

// SPI for EEPROM constants
#define MOSI PIND6
#define MISO PIND5
#define SELECT PIND3
#define CLK PIND4
#define MY_UBRR 12

#define MEMORY_SIZE 100

uint8_t count = 0x00;

// EEPROM Functions
void shift10bits(unsigned int data){
	PORTD |= (1 << SELECT); // señal  del selector en HIGH
	for(uint8_t count = 0; count < 10; count ++){ // bucle de 10 repeticiones
		PORTD &= ~(1 << CLK); // señal del clock en LOW
		
		// while(sclk_Read() == 1)sclk_Write(0); // bucle para asegurar señal del clock en LOW

		if(data & 0x0200) PORTD |= (1 << MOSI); // escribimos el décimo bit  (0b 0000 00B0 0000 0000)
		else PORTD &= ~(1 << MOSI);
		
		PORTD |= (1 << CLK);  // señal del clock en HIGH
		
		// while(sclk_Read() == 0)sclk_Write(1); // bucle para asegurar señal del clock en HIGH
		
		data <<= 1;             // rotamos la data hacia la izquierda para enviar el siguiente bit
	}
	PORTD &= ~(1 << CLK); // señal del clock en LOW cuando acaba el for loop
	PORTD &= ~(1 << MOSI);
}

void shiftdata(uint8_t data_write){
	for(uint8_t count = 0; count < 8; count ++){
		PORTD &= ~(1 << CLK);  // señal del clock en LOW
		
		//while(sclk_Read() == 1)sclk_Write(0);  // bucle para asegurar señal del clock en LOW
		
		if(data_write & 0x80)PORTD |= (1 << MOSI);       // escribimos el octavo bit  (0b B000 0000)
		else PORTD &= ~(1 << MOSI);
		
		PORTD |= (1 << CLK);  // señal del clock en HIGH
		
		// while(sclk_Read() == 0)sclk_Write(1); // bucle para asegurar señal del clock en HIGH
		
		data_write <<= 1;               // rotamos la data hacia la izquierda para enviar el siguiente bit
	}
	PORTD &= ~(1 << CLK);
	PORTD &= ~(1 << MOSI);
}


uint8_t getOutput(uint8_t address_local){
	uint8_t response = 0x00;
	for(uint8_t i = 0; i < 8; i++){
		PORTD &= ~(1 << CLK);
		while( (PORTD & (1<<CLK)) == (1<<CLK) ){PORTD &= ~(1 << CLK);}  // bucle para asegurar señal del clock en LOW
		PORTD |= (1 << CLK);
		while((PORTD & (1<<CLK)) != (1<<CLK) ){PORTD |= (1 << CLK);}
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
	PORTD &= ~(1 << SELECT);    // Selector is isabled
}

// USART Functions
void UART_Init(unsigned int ubrr)
{
	UCSR0A = (1<<U2X0); // Se coloca a doble velocidad
	UCSR0B = (1<<TXCIE0| 1<<TXEN0|1<<RXCIE0|1<<RXEN0); //Se habilita interrupción por RX, se habilita el RX, se habilita el TX
	UCSR0C = (0<<UMSEL00)|(0<<UPM00)|(0<<USBS0)|(3<<UCSZ00); //Asincrono. Sin bit de paridad. Se coloca 1 bit de parada, 8 bits de datos
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr; //Se coloca los baudios 9600
}

void UART_Transmit(unsigned char dato_tx)
{
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = dato_tx;
}

int main(void)
{
	
	UART_Init(MY_UBRR);
	DDRD |= (1 << PIND1);
	DDRD |= (1 << MOSI) | (1 << CLK) | (1 << SELECT); // MOSI, CLK, SELECT(CS) SALIDAS
	DDRD &= ~(1 << MISO); // MISO ENTRADA
	
	PORTD &= ~(1 << CLK); // clk = 0
	
	// LEDs
	DDRD |= (1 << PIND7);

	
    while (1) 
    {	
		
		EWEN();
		write_byte(count, rand() % (30 + 1 - 10) + 10 );
		EWDS();
	
		
		_delay_ms(5000);
		UART_Transmit(read_EEPROM(count));
		write_byte(count, read_EEPROM(count));
		
	    count++;
	}
}

