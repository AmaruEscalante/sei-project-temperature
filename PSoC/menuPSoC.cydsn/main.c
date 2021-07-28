#include "project.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "monitor_temperature.h"

#define MAX_LENGTH 2 // 1 extra for the "return" character
#define RETURN 0x0D

CY_ISR_PROTO(stop_temperature_conversion);

bool IS_READING_TEMPERATURE = false;

char readUART()
{
    char ch;
    while (1)
    {
        ch = UART_GetChar();
        // when no data is received, the Rx buffer is full of zeros
        if (ch > 0u)
        {
            break; // only return the value if there was an actual input
        }
    }
    return ch;
}

int getNewSize(char *arr)
{
    // Count the number bytes before the return key was presssed
    int newSize = 0;
    for (int i = 0; i < MAX_LENGTH; i++)
    {
        if (arr[i] == RETURN)
        {
            break;
        }
        newSize++;
    }
    return newSize;
}

void read_temp()
{
    UART_PutString("Start temperature monitoring\r");
    I2C_Start();
    Rx_ISR_StartEx(stop_temperature_conversion);

    float prom = 0;
    char buffer[100];

    uint8 data_h;
    uint8 data_l;

    // Configurar como modo conversión continua
    DS_WriteConfigRegister(0x2);
    // Empezar conversión
    DS_StartConvert();
    IS_READING_TEMPERATURE = true;
    CyDelay(1000);
    for(;;)
    {
        if(IS_READING_TEMPERATURE == false){
            break;
        }
        prom = 0;

        for (uint8_t i = 0; i < 5; i++)
        {
            // Leer temperatura
            DS_ReadTemp(&data_l, &data_h);
            // Sumar valor del MSByte
            prom += (float)data_h;
            // Sumar valor del LSByte
            if (data_l == 0x80)
            {
                prom += 0.5;
            }
            CyDelay(10);
        }
        // Promedio
        prom = prom / 5;

        sprintf(buffer, "Temperatura promedio: %0.1f\n\r", prom);
        UART_PutString(buffer);

        CyDelay(1000);
    }
}

void printMenu()
{
    UART_PutString("--------------------------------------------------------\r");
    UART_PutString("Please insert the number of an option:\r");
    UART_PutString("1. Simple temperature monitoring\r");
    UART_PutString("2. Encripted temperature monitoring\r");
    UART_PutString("3. Read EEPROM data\r");
    UART_PutString("4. Get max, min and mean\r");
    UART_PutString("--------------------------------------------------------\r");
}

void option1()
{
    UART_PutString("Option 1 selected\r");
    read_temp();
}

void option2()
{
    UART_PutString("Option 2 selected\r");
}

void option3()
{
    UART_PutString("Option 3 selected\r");
}

void option4()
{
    UART_PutString("Option 4 selected\r");
}

int main(void)
{
    CyGlobalIntEnable;
    UART_Start();

    char ch;
    char option;
    int iter;
    char received_data[MAX_LENGTH];

    ch = 0x00;
    iter = 0;
    
    for(;;)
    {
        ch = 0x00;
        iter = 0;
        printMenu();

        while (ch != RETURN && iter < MAX_LENGTH + 1)
        {
            ch = readUART();
            received_data[iter] = ch;
            iter++;
        }

        if (iter > MAX_LENGTH)
        {
            UART_PutString("Word length exceeded. Please try again\r");
            CyDelay(2000);        // Delay so the next word won't be mixed with previous one
            UART_ClearRxBuffer(); // Clear is necessary, otherwise previous chars might be included on the new word
            continue;
        }

        // Resize array
        option = received_data[0];

        switch (option)
        {
        case '1':
            option1();
            break;

        case '2':
            option2();
            break;

        case '3':
            option3();
            break;

        case '4':
            option4();
            break;

        default:
            UART_PutString("Input was not a number between 1 and 4. Please try Again\r");
            continue;
        }
    }
}

CY_ISR(stop_temperature_conversion)
{
    // Si lee 'c' o 'C' se regresa al menu principal
    char tmp = UART_GetChar();
    if (tmp == 'c' || tmp == 'C'){
        UART_PutString("Se presiono C, termina de leer \r");
        IS_READING_TEMPERATURE = false;
        Rx_ISR_Stop();
    }
}