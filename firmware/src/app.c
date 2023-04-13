// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2022 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include <stdlib.h>
#include "app.h"
#include "definitions.h"
#include "app_ble.h"
#include "modbus/rtu/mbrtu.h"
#include "modbus/include/mb.h"
#include "peripheral/sercom/usart/plib_sercom0_usart.h"
#include "config/default/peripheral/eic/plib_eic.h"
#include "ble_trsps/ble_trsps.h"

#define REG_INPUT_START                 ( 1000 )
#define REG_INPUT_NREGS                 ( 64 )

#define REG_HOLDING_START               ( 1 )
#define REG_HOLDING_NREGS               ( 32 )
#define COIL_NREGS                      ( 8 )
#define MB_SERIAL_PDU_SIZE_MAX     256
#define SERVER_ADDRESS    0x01
// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

static USHORT   usRegInputStart = REG_INPUT_START;
static USHORT   usRegInputBuf[REG_INPUT_NREGS];
static USHORT   usRegHoldingStart = REG_HOLDING_START;
static USHORT   usRegHoldingBuf[REG_HOLDING_NREGS];
static USHORT   usCoilBuf[REG_HOLDING_NREGS];
uint16_t conn_hdl; 
// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/

uint8_t asciiNum_to_hex(uint8_t ascii_val)
{
    uint8_t hex_val = 0xFF;
    if((ascii_val>='0') && (ascii_val<='9'))
    {
        hex_val = ascii_val - 48;
    }
    else if( (ascii_val>='A') && (ascii_val<='F')  )
    {
         hex_val = ascii_val - 55;
    }
    else if ( (ascii_val>='a') && (ascii_val<='f') )
    {
        hex_val = ascii_val - 87;
    }
    else
    {
        hex_val = 0xFF;
    }
    return hex_val;
}

#ifdef RTU_CLIENT
void BLEtoModbusData(uint8_t * buffer,int len)
{
    int count=0;
    for(int i=0;i<len;i+=2)
    {
        buffer[i]=asciiNum_to_hex(buffer[i]);
        buffer[i+1]=asciiNum_to_hex(buffer[i+1]);
        mbRTUFrame[count]=(buffer[i]<<4)|(buffer[i+1]);
        printf("\r\n\t%x",mbRTUFrame[count]);
        count++;
    }
    writeHoldingReg(mbRTUFrame[0],count-1);
}
#endif


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;


    appData.appQueue = xQueueCreate( 64, sizeof(APP_Msg_T) );
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */


void APP_Tasks ( void )
{
    APP_Msg_T    appMsg[1];
    APP_Msg_T   *p_appMsg;
    p_appMsg=appMsg;
    eMBErrorCode eMBstatus;
    const UCHAR     ucServerID[] = { 0xAA, 0xBB, 0xCC };

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;
            APP_BleStackInit();
            RTC_Timer32Start();
            BLE_GAP_SetAdvEnable(0x01, 0);
            printf("[BLE] Started Advertising!!!\r\n");
            eMBstatus = eMBInit( MB_RTU, SERVER_ADDRESS , 1, 9600, MB_PAR_NONE );
            if(eMBstatus != MB_ENOERR)
            {
                printf("[Error] eMBInit\r\n");
            }
            else
            {
                eMBstatus = eMBSetServerID( 0x34, TRUE, ucServerID, 3 );
                if(eMBstatus != MB_ENOERR)
                {
                    printf("[Error] eMBSetServerID\r\n");
                }
                else
                {
                    eMBstatus = eMBEnable();
                    if(eMBstatus != MB_ENOERR)
                    {
                        printf("[Error] eMBEnable\r\n");
                        /* Enable failed. */
                    }
                    else
                    {
                        p_appMsg->msgId = APP_MSG_MB_DATA_POLL;
                        OSAL_QUEUE_Send(&appData.appQueue, p_appMsg, 0);
                    }
                }
            }

            if (appInitialized)
            {

                appData.state = APP_STATE_SERVICE_TASKS;
            }
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            if (OSAL_QUEUE_Receive(&appData.appQueue, &appMsg, 1))
            {
                if(p_appMsg->msgId==APP_MSG_BLE_STACK_EVT)
                {
                    // Pass BLE Stack Event Message to User Application for handling
                    APP_BleStackEvtHandler((STACK_Event_T *)p_appMsg->msgData);
                }
                else if(p_appMsg->msgId==APP_MSG_BLE_STACK_LOG)
                {
                    // Pass BLE LOG Event Message to User Application for handling
                    APP_BleStackLogHandler((BT_SYS_LogEvent_T *)p_appMsg->msgData);
                }
                else if(p_appMsg->msgId == APP_MSG_MB_DATA_POLL)
                {
                    ( void )eMBPoll(); 
                }
                else if(p_appMsg->msgId == APP_MSG_MODBUS_TX)
                {
                    extern void eMB_Write_Data(void);
                    eMB_Write_Data();
                }
                else if(p_appMsg->msgId == APP_MSG_BLE_DISPLAY_DATA)
                {
                    BLE_TRSPS_SendData(conn_hdl,appMsg->msgData[BLE_DATA_LEN],(uint8_t *)&p_appMsg->msgData[BLE_DATA]);
                }
                else if(p_appMsg->msgId == APP_MSG_BLE_DISPLAY_EVT)
                {
                   SERCOM0_USART_Write((uint8_t *)"\r\n\t[BLE Data]",13);
                   SERCOM0_USART_Write((uint8_t *)&p_appMsg->msgData[BLE_DATA],appMsg->msgData[BLE_DATA_LEN]);
#ifdef RTU_CLIENT
                   BLEtoModbusData((uint8_t *)&p_appMsg->msgData[BLE_DATA],appMsg->msgData[BLE_DATA_LEN]);
#endif
                }
            }
            break;
        }

        /* TODO: implement your application state machine.*/


        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;
    APP_Msg_T    appMsg;
    char buffer1[100],buffer2[100];
    sprintf(buffer1,"Function code:0x04 \r\n\tNo of Regs: %d\r\n\tMode: READ\r\n",usNRegs);
    int len=strlen(buffer1);
    appMsg.msgId = APP_MSG_BLE_DISPLAY_DATA;
    appMsg.msgData[BLE_DATA_LEN] = len;
    memcpy(&appMsg.msgData[BLE_DATA ], buffer1, len);
    appMsg.msgData[DISP_DATA_OFFSET+len]= '\0';
    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
    
    if( ( usAddress >= REG_INPUT_START )
        && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegInputStart );
        while( usNRegs > 0 )
        {
            usRegInputBuf[iRegIndex] = 10*iRegIndex;
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
            
            sprintf(buffer2,"Input Register %d Value: %d",(int)iRegIndex,(int)usRegInputBuf[iRegIndex]);
            BLE_TRSPS_SendData(conn_hdl,strlen(buffer2),(uint8_t *)&buffer2);
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;
    int temp=0;
    APP_Msg_T    appMsg;
    char buffer1[100],buffer2[100];
    uint8_t func_code;
    
    if( ( usAddress >= REG_HOLDING_START ) && ( usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS ) )
    {
        iRegIndex =(int)( usAddress - usRegHoldingStart);
        switch ( eMode )
        {
        case MB_REG_READ:
            func_code=0x03;
            sprintf(buffer1,"Function code:0x%x \r\n\tNo of Regs: %d\r\n\tMode: READ\r\n", func_code, usNRegs);
            while( usNRegs > 0 )
            {
                *pucRegBuffer++ = ( unsigned char )( usRegHoldingBuf[iRegIndex] >> 8 );
                *pucRegBuffer++ = ( unsigned char )( usRegHoldingBuf[iRegIndex] & 0xFF );
                sprintf(buffer2,"Holding Register %d Data: %d",(int)iRegIndex,(int)usRegHoldingBuf[iRegIndex]);
                BLE_TRSPS_SendData(conn_hdl,strlen(buffer2),(uint8_t *)&buffer2);
                iRegIndex++;
                usNRegs--;
            }
            
            break;

        case MB_REG_WRITE:
            if(usNRegs>1)
            {
                func_code=0x16;
            }
            else
            {
                func_code=0x06;
            }
            sprintf(buffer1,"Function code:0x%x \r\n\tNo of Regs: %d\r\n\tMode: WRITE\r\n", func_code, usNRegs);
            while( usNRegs > 0 )
            {
                usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
                usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
                printf("\r\n usRegHoldingBuf[%d]: %d\r\n",iRegIndex,usRegHoldingBuf[iRegIndex]);
                if((usRegHoldingBuf[iRegIndex])>0)
                {
                    sprintf(buffer2,"Holding Register %d Value: %d",(int)iRegIndex,(int)usRegHoldingBuf[iRegIndex]);
                    BLE_TRSPS_SendData(conn_hdl,strlen(buffer2),(uint8_t *)&buffer2);
                }
                else
                {
                    sprintf(buffer2,"Holding Register %d Value: %d",(int)iRegIndex,(int)usRegHoldingBuf[iRegIndex]);
                    BLE_TRSPS_SendData(conn_hdl,strlen(buffer2),(uint8_t *)&buffer2);
                }
                temp=temp+usRegHoldingBuf[iRegIndex];
                iRegIndex++;
                usNRegs--;
            }
            if(temp>0)
            {
                BLUE_LED_Set();
            }
            else
            {
                BLUE_LED_Clear();
            }
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    int len=strlen(buffer1);
    appMsg.msgId = APP_MSG_BLE_DISPLAY_DATA;
    appMsg.msgData[BLE_DATA_LEN] = len;
    memcpy(&appMsg.msgData[BLE_DATA ], buffer1, len);
    appMsg.msgData[DISP_DATA_OFFSET+len]= '\0';
    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
    return eStatus;
}

eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils,
               eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;
    APP_Msg_T    appMsg;
    char buffer1[100],buffer2[100];
    uint8_t func_code;
    if( ( usAddress >= REG_HOLDING_START ) && ( usAddress + usNCoils <= REG_HOLDING_START + COIL_NREGS ) )
    {
        iRegIndex =(int)( usAddress - usRegHoldingStart);
        switch ( eMode )
        {
        case MB_REG_READ:
            func_code=0x01;
            sprintf(buffer1,"Function code:0x%x \r\n\tNo of Regs: %d\r\n\tMode: READ\r\n", func_code, usNCoils);
            while( usNCoils > 0 )
            {
                *pucRegBuffer++ = ( unsigned char )( usCoilBuf[iRegIndex] >> 8 );
                *pucRegBuffer++ = ( unsigned char )( usCoilBuf[iRegIndex] & 0xFF );
                sprintf(buffer2,"Coil Register %d Value: %d",(int)iRegIndex,(int)usCoilBuf[iRegIndex]);
                BLE_TRSPS_SendData(conn_hdl,strlen(buffer2),(uint8_t *)&buffer2);
                iRegIndex++;
                usNCoils--;
            }
            
            break;

        case MB_REG_WRITE:
            if(usNCoils>1)
            {
                func_code=0x16;
            }
            else
            {
                func_code=0x06;
            }
            sprintf(buffer1,"Function code:0x%x \r\n\tNo of Regs: %d\r\n\tMode: WRITE\r\n", func_code, usNCoils);
            while( usNCoils > 0 )
            {
                usCoilBuf[iRegIndex] = *pucRegBuffer;
                printf("\r\n usCoilBuf[%d]: %d\r\n",iRegIndex,usRegHoldingBuf[iRegIndex]);
                if((usRegHoldingBuf[iRegIndex])>0)
                {
                    sprintf(buffer2,"Relay %d: On",(int)iRegIndex);
                    BLE_TRSPS_SendData(conn_hdl,strlen(buffer2),(uint8_t *)&buffer2);
                }
                else
                {
                    sprintf(buffer2,"Relay %d: OFF",(int)iRegIndex);
                    BLE_TRSPS_SendData(conn_hdl,strlen(buffer2),(uint8_t *)&buffer2);
                }
                iRegIndex++;
                usNCoils--;
            }
            
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    int len=strlen(buffer1);
    appMsg.msgId = APP_MSG_BLE_DISPLAY_DATA;
    appMsg.msgData[BLE_DATA_LEN] = len;
    memcpy(&appMsg.msgData[BLE_DATA ], buffer1, len);
    appMsg.msgData[DISP_DATA_OFFSET+len]= '\0';
    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
    return eStatus;
}

eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    return MB_ENOERR;
}

/*******************************************************************************
 End of File
 */
