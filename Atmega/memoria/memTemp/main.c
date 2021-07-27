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
#define SELECT PIND2
#define CLK PIND3

#define MEMORY_SIZE 100

uint8_t count = 0x00;
uint8_t minuteflag = 0x00;
uint8_t maxTemp = 0x00;
uint8_t minTemp = 0xFF;


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
		//while( PORTD & (1<<CLK) ){PORTD &= ~(1 << CLK);}  // bucle para asegurar señal del clock en LOW
		PORTD |= (1 << CLK);
		//while(~(PORTD & (1<<CLK))){PORTD |= (1 << CLK);}
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
	char temperature;
	uint8_t temp;
	PORTD ^= (1<<PIND6);
	temperature = readTemperature();
	EWEN();
	
	if (temperature > maxTemp){
		temp = read_EEPROM(MEMORY_SIZE) +1;
		write_byte(MEMORY_SIZE+1, temperature); // Almacenando temperatura maxima
		write_byte(MEMORY_SIZE+2, ((int)count+1)*(int)temp);
		maxTemp = temperature;
	}
	if (temperature < minTemp){
		temp = read_EEPROM(MEMORY_SIZE) +1;
		write_byte(MEMORY_SIZE+3, temperature); // Almacenando temperatura minima
		write_byte(MEMORY_SIZE+4, ((int)count+1)*(int)temp); // Almacenando tiempo
		minTemp = temperature;
	}
		
	write_byte(count, temperature); // Almacenar temperatura
	count++;
	if (count >= MEMORY_SIZE){
		count = 0;
		temp = read_EEPROM(MEMORY_SIZE);
		write_byte(MEMORY_SIZE, temp+1);	
	}
	
	EWDS();
}


void readhundredtemp();

int main(void)
{	
	TWI_init();
	DS1621_Init();
	
	DDRD |= (1 << MOSI) | (1 << CLK) | (1 << SELECT); // MOSI, CLK, SELECT(CS) SALIDAS
	DDRD &= ~(1 << PIND5); // MISO ENTRADA
	
	PORTD &= ~(1 << CLK); // clk = 0
	
	// LEDs
	DDRD |= (1 << 6);
		
	sei();
	timerInit();
	
	EWEN();
	
	write_byte(MEMORY_SIZE, 0x00);
	
	EWDS();
	
	while (1)
	{
		
		_delay_ms(60000);
		readhundredtemp();

	}
}


void readhundredtemp()
{
    // Recieve last 100 temperature
	char temperature;
	uint8_t contar = 0;
	
	
	// verificar si ya tiene 100 datos
	contar = read_EEPROM(MEMORY_SIZE); // direccion del contador de cuantas veces se llego a 100 datos
	if(contar > 0){
		// If the memory have already saved more than 100 data.
		
		// Elegir entre ultimo a reciente o de reciente a ultimo valor guardado
		contar = (count + 1 >= MEMORY_SIZE)? 0: count + 1; // de mas antiguo a reciente
		//contar = ((count - 1 < 0) || (count - 1 >= MEMORY_SIZE))? MEMORY_SIZE - 1: count; // de mas reciente a antiguo
		
		for(uint8_t i = 0; i < MEMORY_SIZE; i++)
		{
			temperature = read_EEPROM(contar);
		
			// Write program to use temperature;
			
			
			// Elegir entre ultimo a reciente o de reciente a ultimo valor guardado
			contar = (contar + 1 >= MEMORY_SIZE)? 0: contar + 1; // de mas antiguo a reciente
			//contar = ((contar - 1 < 0) || (contar - 1 >= MEMORY_SIZE))? MEMORY_SIZE - 1: count - 1; // de mas reciente a antiguo
		}
	}else{
		// If there aren't 100 data, read all temperature saved until the moment
		
		contar = 0; // antiguo a reciente
		//contar = count; // reciente a antiguo
		
		for(uint8_t i = 0; i < count; i++)
		{
			temperature = read_EEPROM(contar);
			
			// Write program to use temperature
			
			contar++; // antiguo a reciente
			//contar--;  // reciente a antiguo
		}	
		
	}
	
}