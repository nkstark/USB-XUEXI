#include "stm32f10x.h"
#include "usb_lib.h"
#include "hw_config.h"
#include "delay.h"

void USB_SendString(u8 *str);
extern u8 Receive_Buffer[22];
extern u8 Transi_Buffer[22];
int main(void)
{
		
		Set_System();
		delay_init(72);
		Set_USBClock();
		USB_Interrupts_Config();
		USB_Init();
	     while(1) //
	     {
	             USB_SendString("MiniSTM32");
	             delay_ms(1500);
	     }
}
void USB_SendString(u8 *str)	   // 
{
     u8 ii=0;   
     while(*str)
     {
         Transi_Buffer[ii++]=*(str++);
         if (ii ==22) break;
     }
     UserToPMABufferCopy(Transi_Buffer, ENDP2_TXADDR, 22);
     _SetEPTxStatus(ENDP2, EP_TX_VALID);
}


