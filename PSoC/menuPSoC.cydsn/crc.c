/* 
* CRC-8 / Maxim
*/
#include "crc.h"

// polynomial
uint8_t key = 0x8C; // 1000 1100 -> x^7 + x^3 + x^2

uint8_t crc_calculate(uint8_t* datos, int len){
	uint8_t crc_value = 0x00;	
	uint8_t temp  = 0x00;
	uint8_t sum = 0x00;
	
	for(int i=0; i<len; i++)
	{
		temp = datos[i];
		
		for (uint8_t j=0; j<8; j++)
		{
			sum = (crc_value ^ temp) & 0x01; //xor
			crc_value >>= 1; // shift right
			if (sum)
			crc_value ^= key; // xor
			temp >>= 1; // shift right
		}
	}
	
	return crc_value;
}
