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
#include "monitor_temperature.h"

uint8 DS_ReadConfigRegister(){
    // Definir data a enviar y data a leer
    uint8 count = 0xAC;
    uint8 data;
    // Enviar por I2C
    I2C_MasterWriteBuf(I2C_SLAVE_ADDR, &count, 1, I2C_MODE_NO_STOP);
    // Esperar a que la transmisión se complete
    while(0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)){}
    // Enviar Repeated Start
    I2C_MasterSendRestart(I2C_SLAVE_ADDR, 1);
    // Leer byte 
    data = I2C_MasterReadByte(I2C_NAK_DATA);
    // Terminar comunicación
    I2C_MasterSendStop();
    // Borrar Buffer
    I2C_MasterClearWriteBuf();
    return data;
}

void DS_WriteConfigRegister(uint8 data){
    uint8 count[2];
    count[0] = Access_Config;
    count[1] = data;
    I2C_MasterWriteBuf(I2C_SLAVE_ADDR, count, 2, I2C_MODE_COMPLETE_XFER);
    while(0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)){}
    I2C_MasterClearWriteBuf();
    return;
}

void DS_StartConvert(){
    uint8 op = Start_Conv;
    I2C_MasterWriteBuf(I2C_SLAVE_ADDR, &op, 1, I2C_MODE_COMPLETE_XFER);
    while(0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)){}
    I2C_MasterClearWriteBuf();
    return;
}

void DS_ReadTemp(uint8 *data_l, uint8 *data_h ){
    uint8 op = Read_Temp;
    I2C_MasterWriteBuf(I2C_SLAVE_ADDR, &op, 1, I2C_MODE_NO_STOP);
    while(0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)){}
    I2C_MasterSendRestart(I2C_SLAVE_ADDR, 1);
    *data_h = I2C_MasterReadByte(I2C_ACK_DATA);
    *data_l = I2C_MasterReadByte(I2C_NAK_DATA);
    I2C_MasterSendStop();
    I2C_MasterClearWriteBuf();
    return;
}