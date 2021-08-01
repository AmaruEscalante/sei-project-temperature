
/*
 * I2CMaster.h
 *
 * Created: 8/1/2021 12:12:27 AM
 *  Author: vcave
 */ 

// TWI constants
#define ACK 1
#define NACK 0



void TWI_init();

void TWI_Start();
void TWI_Repeat_Start();

void TWI_Stop();

void TWI_RegisterSelect(uint8_t addr, uint8_t reg);

int TWI_Read(uint8_t addr, uint8_t N_ACK);

void TWI_Write(uint8_t data);