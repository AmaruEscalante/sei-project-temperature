#include "project.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "monitor_temperature.h"

#define MAX_LENGTH 2 // 1 extra for the "return" character
#define RETURN 0x0D

CY_ISR_PROTO(stop_temperature_conversion);

bool IS_READING_TEMPERATURE = false;

char findChar(int n)
{
  if(n == 0)return 'A';
  else if(n ==1) return 'D';
  else if(n ==2) return 'F';
  else if(n ==3) return 'G';
  else if(n ==4) return 'V';
  else return 'X';
}

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

float read_temp()
{
    float prom = 0;

    uint8 data_h;
    uint8 data_l;

    // Configurar como modo conversión continua
    DS_WriteConfigRegister(0x2);
    // Empezar conversión
    DS_StartConvert();
    
    CyDelay(1000);
    
        
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
    return prom;
    
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
    char buffer[100];
    float prom;
        
    UART_PutString("Option 1 selected\r");
    UART_PutString("Start temperature monitoring\r");
    I2C_Start();
    Rx_ISR_StartEx(stop_temperature_conversion);
    // Pone Atmel en modo esclavo para no interrumpir el monitoreo
    SR_Write(1);
    IS_READING_TEMPERATURE = true;
    for(;;)
    {
        if(IS_READING_TEMPERATURE == false){
            break;
        }
        prom = read_temp();
        sprintf(buffer, "Temperatura promedio: %0.1f\n\r", prom);
        UART_PutString(buffer);
        CyDelay(2000);
    }
}

void option2()
{
    char buffer[26];
    float prom;
        
    UART_PutString("Option 2 selected\r");
    UART_PutString("Start temperature monitoring\r");
    I2C_Start();
    Rx_ISR_StartEx(stop_temperature_conversion);
    // Pone Atmel en modo esclavo para no interrumpir el monitoreo
    SR_Write(1);
    IS_READING_TEMPERATURE = true;
    for(;;)
    {
        if(IS_READING_TEMPERATURE == false){
            break;
        }
        prom = read_temp();
        sprintf(buffer, "TEMPERATURA PROMEDIO: %0.1f", prom);
        // Encriptacion
        // Llave ADFGVX
        int llave[6][6] = { 
              //  A   D   F   G   V   X 
                {'A','B','C','D','E','F'}, // A = 65
                {'G','H','I','J','K','L'}, // D = 68
                {'M','N','O','P','Q','R'}, // F = 70
                {'S','T','U','V','.',':'}, // G = 71
                {'Y','Z','6','5','4','3'}, // V = 86
                {'7','8','9',' ','1','2'}, // X = 88
              };
        
        // Llave 'FPGA'
        char llave2[4] = {'F','P','G','A'};
        int orden[4];
        char temp;
        int temp2;
        // Arreglo de posiciones de llave 2
        for(int i=0; i<4; i++)orden[i] = i;
        
        // Bubble sort
        for (int i = 0; i < 4-1; i++){   
           for (int j = 0; j < 4-i-1; j++)
               if (llave2[j] > llave2[j+1])
               {
                 temp = llave2[j];
                 llave2[j] = llave2[j+1];
                 llave2[j+1] = temp;
                 temp2 = orden[j];
                 orden[j] = orden[j+1];
                 orden[j+1] = temp2;
               }
        }
        
        int size = sizeof(buffer)/sizeof(buffer[0]);
        
        // Coding
        char mensaje_cifrado[2*size];
       
        const int rows = 1 + 2*size/4;

        char fase2[rows][4];
        int counter = 0, counter2 = 0;
        // Generar matriz 
        for(int n=0; n<size; n++){
            for(int i=0; i<6; i++){
                for(int j=0; j<6; j++){

                  if(llave[i][j] == buffer[n]){
                    fase2[counter2][2*counter] = findChar(i); // Horizontal
                    fase2[counter2][2*counter+1] = findChar(j); // Vertical
                    
                    counter++;
                    if (counter > 1){
                      counter = 0;
                      counter2++;
                    }
                  }
                }
            }
        }
        // Obtener mensaje cifrado de matriz
        int indice = 0;

        for(int j=0; j<4; j++){
            for(int i=0; i<rows; i++){
              if(i != rows-1){
                mensaje_cifrado[indice] = fase2[i][orden[j]];
                UART_PutChar(mensaje_cifrado[indice]);
                indice++;
              }
            }
        }
        //UART_PutString(mensaje_cifrado);
        UART_PutString("\n\r");
        CyDelay(2000);
    }
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
        // Atmel en modo maestro
        SR_Write(0);
    }
}