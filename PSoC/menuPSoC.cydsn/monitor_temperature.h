/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

/* [] END OF FILE */
#ifndef MONITOR_TEMPERATURE_H
#define MONITOR_TEMPERATURE_H
// Codigos de Operacion dadas en la hoja de datos del sensor
#define I2C_SLAVE_ADDR  0x4D

// Comandos de lectura
#define Read_Temp       0xAA // lee temperatura
#define Read_Cont       0xA8 // lee el contador
#define Read_Slope      0xA9 // lee el cambio entre dos puntos
// Comandos de operacion del sensor
#define Start_Conv      0xEE // Inicia la conversion de tmp
#define Stop_Conv       0x22 // Detiene la conversion de tmp

// Comandos de acceso
#define Access_TH       0xA1 // Accede al registro TH
#define Access_TL       0xA2 // Accede al registro TL
#define Access_Config   0xAC // Accede al registro de configuracion

#include "project.h"
#include <stdio.h>
#include <string.h>
    
uint8 DS_ReadConfigRegister();

void DS_WriteConfigRegister(uint8 data);

void DS_StartConvert();

void DS_ReadTemp(uint8 *data_l, uint8 *data_h );

#endif

