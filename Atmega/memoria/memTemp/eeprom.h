
/*
 * eeprom.h
 *
 * Created: 8/1/2021 12:06:52 AM
 *  Author: vcave
 */ 

#include <avr/io.h>

// SPI for EEPROM constants
#define MOSI PIND4
#define MISO PIND5
#define SELECT PIND6
#define CLK PIND3

void shift10bits(unsigned int data);
void shiftdata(uint8_t data_write);
uint8_t getOutput(uint8_t address_local);
uint8_t read_EEPROM(uint8_t address);
void write_byte(uint8_t address_write,uint8_t data_write);
void EWEN();
void EWDS();
void eraseAll();
void writeAll(uint8_t data_write);