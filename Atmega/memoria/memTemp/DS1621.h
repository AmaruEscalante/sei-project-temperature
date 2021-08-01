
/*
 * DS1621.h
 *
 * Created: 8/1/2021 12:15:10 AM
 *  Author: vcave
 */ 

#include <stdint.h>
#include <stdlib.h>
// DS1621 registers
#define DS1621 0x48 //DS1621 I2C address with A2, A1 and A0 connected to ground
#define ACCESS_CONFIG 0xAC
#define READ_TEMPERATURE 0xAA
#define ACCESS_TL 0xA2
#define ACCESS_TH 0xA1
#define START_CONVERT_T 0xEE
#define STOP_CONVERT_T 0x22


void DS1621_Init();
char readTemperature();


//void readhundredtemp(uint8_t temp);
//void readmaxminprom(void);
