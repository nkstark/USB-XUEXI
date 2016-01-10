/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : usb_pwr.c
* Author             : MCD Application Team
* Version            : V2.2.0
* Date               : 06/13/2008
* Description        : Connection/disconnection & power management
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_pwr.h"
#include "hw_config.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
vu32 bDeviceState = UNCONNECTED; /* USB device status */
volatile bool fSuspendEnabled = TRUE;  /* true when suspend is possible */

struct
{
  volatile RESUME_STATE eState;
  volatile u8 bESOFcnt;
}ResumeS;

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Extern function prototypes ------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : PowerOn
* Description    :
* Input          : None.
* Output         : None.
* Return         : USB_SUCCESS.
*******************************************************************************/
RESULT PowerOn(void)
{
  /*** CNTR_PWDN = 0 ***/
 *CNTR=0x0001;

  /*** CNTR_FRES = 0 ***/
  wInterrupt_Mask = 0;
  *CNTR=0;					//清除复位信号
  /*** Clear pending interrupts ***/
  *ISTR=0;					 //清除中断
  /*** Set interrupt mask ***/
  wInterrupt_Mask=0x1c00;
  *CNTR=wInterrupt_Mask;  //开中断

  return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : PowerOff
* Description    : handles switch-off conditions
* Input          : None.
* Output         : None.
* Return         : USB_SUCCESS.
*******************************************************************************/
RESULT PowerOff()
{
  /* disable all ints and force USB reset */
  *CNTR=0x0001;
  /* clear interrupt status register */
  *ISTR=0;
  /* Disable the Pull-Up*/
//  USB_Cable_Config(DISABLE);
  /* switch-off device */
  *CNTR=0x0003;
  /* sw variables reset */
  /* ... */

  return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : Suspend
* Description    : sets suspend mode operating conditions
* Input          : None.
* Output         : None.
* Return         : USB_SUCCESS.
*******************************************************************************/
void Suspend(void)
{
  u16 wCNTR;
  /* suspend preparation */
  /* ... */

  /* macrocell enters suspend mode */
  wCNTR = (*CNTR);
  wCNTR |= 0x0008;
  *CNTR=wCNTR;

  /* ------------------ ONLY WITH BUS-POWERED DEVICES ---------------------- */
  /* power reduction */
  /* ... on connected devices */


  /* force low-power mode in the macrocell */
  wCNTR = (*CNTR);
  wCNTR |= 0x0004;
  *CNTR=wCNTR;

  /* switch-off the clocks */
  /* ... */
  Enter_LowPowerMode();

}

/*******************************************************************************
* Function Name  : Resume_Init
* Description    : Handles wake-up restoring normal operations
* Input          : None. 
* Output         : None.
* Return         : USB_SUCCESS.
*******************************************************************************/
void Resume_Init(void)	 //取消低功耗模式
{
  u16 wCNTR;

  /* ------------------ ONLY WITH BUS-POWERED DEVICES ---------------------- */
  /* restart the clocks */
  /* ...  */

  /* CNTR_LPMODE = 0 */
  wCNTR = *CNTR;
  wCNTR &= (~0x0004);
  *CNTR=wCNTR;

  /* restore full power */
  /* ... on connected devices */
  Leave_LowPowerMode();

  /* reset FSUSP bit */
  *CNTR=0xBF00;

  /* reverse suspend preparation */
  /* ... */

}

/*******************************************************************************
* Function Name  : Resume
* Description    : This is the state machine handling resume operations and
*                 timing sequence. The control is based on the Resume structure
*                 variables and on the ESOF interrupt calling this subroutine
*                 without changing machine state.
* Input          : a state machine value (RESUME_STATE)
*                  RESUME_ESOF doesn't change ResumeS.eState allowing
*                  decrementing of the ESOF counter in different states.
* Output         : None.
* Return         : None.
typedef enum _RESUME_STATE
{
  RESUME_EXTERNAL,	  //0
  RESUME_INTERNAL,	  //1
  RESUME_LATER,		  //2
  RESUME_WAIT,		  //3
  RESUME_START,		  //4
  RESUME_ON,		  //5
  RESUME_OFF,		  //6
  RESUME_ESOF		  //7
} RESUME_STATE;					枚举型
struct
{
  volatile RESUME_STATE eState;
  volatile u8 bESOFcnt;
}ResumeS;
*******************************************************************************/
void Resume(RESUME_STATE eResumeSetVal)	 //eResumeSetVal=0
{
  u16 wCNTR;

  if (eResumeSetVal != RESUME_ESOF)		 //	  RESUME_ESOF=7	 ESOF：期望帧首标识位
    ResumeS.eState = eResumeSetVal;

  switch (ResumeS.eState)	//RESUME_STATE eState
  {
    case RESUME_EXTERNAL:				  //0
      Resume_Init();
      ResumeS.eState = RESUME_OFF;		  //6
      break;
    case RESUME_INTERNAL:				  //1
      Resume_Init();
      ResumeS.eState = RESUME_START;	  //4
      break;
    case RESUME_LATER:					  //2
      ResumeS.bESOFcnt = 2;
      ResumeS.eState = RESUME_WAIT;		 //3
      break;
    case RESUME_WAIT:					 //3
      ResumeS.bESOFcnt--;
      if (ResumeS.bESOFcnt == 0)
        ResumeS.eState = RESUME_START;	  //4
      break;
    case RESUME_START:					  //4
      wCNTR = *CNTR;
      wCNTR |= 0x0010;
      *CNTR=wCNTR;
      ResumeS.eState = RESUME_ON;		  //5
      ResumeS.bESOFcnt = 10;			   //难道ESOF还有专门的计数器？
      break;
    case RESUME_ON:						  //5
      ResumeS.bESOFcnt--;
      if (ResumeS.bESOFcnt == 0)
      {
        wCNTR = *CNTR;
        wCNTR &= (~CNTR_RESUME);
        *CNTR=wCNTR;
        ResumeS.eState = RESUME_OFF;	 //6
      }
      break;
    case RESUME_OFF:					//6
    case RESUME_ESOF:					//7
    default:
      ResumeS.eState = RESUME_OFF;		//6
      break;
  }
}

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
