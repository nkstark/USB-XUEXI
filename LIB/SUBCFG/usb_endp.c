/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : usb_endp.c
* Author             : MCD Application Team
* Version            : V2.2.0
* Date               : 06/13/2008
* Description        : Endpoint routines
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
#include "stm32f10x.h"
#include "usb_lib.h"
#include "usb_istr.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
u8 Receive_Buffer[22];
u8 Transi_Buffer[22];
u8 USB_ReceiveFlg = FALSE;
// 未知作用vu8 MsgCmd;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : EP1_OUT_Callback.
* Description    : EP1 OUT Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP1_OUT_Callback(void)
{
     USB_ReceiveFlg = TRUE;
     PMAToUserBufferCopy(Receive_Buffer, ENDP1_RXADDR,22);
//未知作用   MsgCmd = Receive_Buffer[21];
     SetEPRxStatus(ENDP1, EP_RX_VALID);
}

//void EP2_IN_Callback(void)
//{
//     u8 ii;
//     for (ii=0;ii<22;ii++) Transi_Buffer[ii] = 0x00;
//     if (GPIOA->ODR & 0x0c )  GPIOA->ODR &= (~0x0c);
//    else GPIOA->ODR |= 0x0c;
//}

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/

