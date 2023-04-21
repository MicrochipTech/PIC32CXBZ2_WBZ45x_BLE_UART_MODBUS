/*
 * FreeModbus Libary: Atmel AT91SAM3S Demo Application
 * Copyright (C) 2010 Christian Walter <cwalter@embedded-solutions.at>
 *
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * IF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * File: $Id$
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "config\default\peripheral\gpio\plib_gpio.h"
#include "config\default\peripheral\sercom\usart\plib_sercom1_usart.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "../modbus/include/mb.h"
#include "port.h"
#include "../modbus/include/mbport.h"
#include "app.h"
#include<stdio.h>

/* ----------------------- Defines ------------------------------------------*/


void eMB_Read_Data(SERCOM_USART_EVENT event, uintptr_t context)
{
    BOOL            bTaskWoken = FALSE;
    vMBPortSetWithinException( TRUE );
    bTaskWoken = pxMBFrameCBByteReceived(  );
    vMBPortSetWithinException( FALSE );
    portEND_SWITCHING_ISR( bTaskWoken ? pdTRUE : pdFALSE );
    
}

void eMB_Write_Data(void)
{
    BOOL            bTaskWoken = FALSE;
    vMBPortSetWithinException( TRUE );
    bTaskWoken = pxMBFrameCBTransmitterEmpty(  );
    vMBPortSetWithinException( FALSE );
    portEND_SWITCHING_ISR( bTaskWoken ? pdTRUE : pdFALSE );
}



void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    if(xRxEnable)
    {
        RS485RxEnable();
    }
    if(xTxEnable)
    {
        RS485TxEnable();
    }
}



/*******************************************************************************
 * void RS485TxEnable(void)
 *
 * Function to enable transmission of RS485 
 *
 * @param None
 * @return None
 ******************************************************************************/
void RS485TxEnable(void)
{
    DE_PIN_Set();  //Enable transmission 
    RE_PIN_Set();  //Disable reception  
}

/*******************************************************************************
 * void RS485RxEnable(void)
 *
 * Function to enable reception of RS485 
 *
 * @param None
 * @return None
 ******************************************************************************/
void RS485RxEnable(void)
{
    RE_PIN_Clear();    //Enable reception
    DE_PIN_Clear();    //Disable transmission
}

BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{

    USART_SERIAL_SETUP usart_setup;
    BOOL    bStatus = false;
    
    usart_setup.baudRate = ulBaudRate;
    usart_setup.dataWidth = ucDataBits;              //SERCOM_USART_INT_CTRLB_CHSIZE_8_BIT; 
    usart_setup.parity = USART_PARITY_NONE;
    usart_setup.stopBits = USART_STOP_1_BIT;
    
    bStatus = SERCOM1_USART_SerialSetup( &usart_setup, 0 );
    
    // Enable UART Read
    if( bStatus == true)
    {
        SERCOM1_USART_ReadNotificationEnable(true, true);
        // Set UART RX notification threshold to be 1
        SERCOM1_USART_ReadThresholdSet(1);
        // Register the UART RX callback function
        SERCOM1_USART_ReadCallbackRegister(eMB_Read_Data, (uintptr_t)NULL);
    }
    return bStatus;
}

void
vMBPortSerialClose( void )
{
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
    void *memadrs=&ucByte;
    BOOL write = SERCOM1_USART_Write(memadrs , sizeof(ucByte));
    return write;
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
    BOOL read = SERCOM1_USART_Read((uint8_t *)pucByte , sizeof(pucByte));
    return read;
}

