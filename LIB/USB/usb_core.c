/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : usb_core.c
* Author             : MCD Application Team
* Version            : V2.2.0
* Date               : 06/13/2008
* Description        : Standard protocol processing (USB v2.0)
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "usb_lib.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ValBit(VAR,Place)    (VAR & (1 << Place))
#define SetBit(VAR,Place)    (VAR |= (1 << Place))
#define ClrBit(VAR,Place)    (VAR &= ((1 << Place) ^ 255))

#define Send0LengthData() { _SetEPTxCount(ENDP0, 0); \
    SaveTState=EP_TX_VALID; \
  }

#define vSetEPRxStatus(st) (SaveRState = st)

#define USB_StatusIn() Send0LengthData()
#define USB_StatusOut() vSetEPRxStatus(EP_RX_VALID)

#define StatusInfo0 StatusInfo.bw.bb1 /* Reverse bb0 & bb1 */
#define StatusInfo1 StatusInfo.bw.bb0

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
u16_u8 StatusInfo;					
bool Data_Mul_MaxPacketSize = FALSE;
/* Private function prototypes -----------------------------------------------*/
static void DataStageOut(void);
static void DataStageIn(void);
static void NoData_Setup0(void);
static void Data_Setup0(void);
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : Standard_GetConfiguration.
* Description    : Return the current configuration variable address.
* Input          : Length - How many bytes are needed.
* Output         : None.
* Return         : Return 1 , if the request is invalid when "Length" is 0.
*                  Return "Buffer" if the "Length" is not 0.
*******************************************************************************/
u8 *Standard_GetConfiguration(u16 Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength =sizeof(pInformation->Current_Configuration); //待发送的或者接受的数据长度
    return 0;
  }
  pUser_Standard_Requests->User_GetConfiguration();
  return (u8 *)&pInformation->Current_Configuration;
}

/*******************************************************************************
* Function Name  : Standard_SetConfiguration.
* Description    : This routine is called to set the configuration value
*                  Then each class should configure device themself.
* Input          : None.
* Output         : None.
* Return         : Return USB_SUCCESS, if the request is performed.
*                  Return USB_UNSUPPORT, if the request is invalid.


DEVICE Device_Table =
  {
    EP_NUM,	  3
    1
  };
typedef struct _DEVICE
{
  u8 Total_Endpoint;      Number of endpoints that are used 
  u8 Total_Configuration; Number of configuration available 
}
DEVICE;
*******************************************************************************/
RESULT Standard_SetConfiguration(void)
{

  if ((pInformation->USBwValue0 <=
      Device_Table.Total_Configuration) && (pInformation->USBwValue1 == 0)
      && (pInformation->USBwIndex == 0)) /*call Back usb spec 2.0*/
  {
    pInformation->Current_Configuration = pInformation->USBwValue0;
    pUser_Standard_Requests->User_SetConfiguration();
    return USB_SUCCESS;
  }
  else
  {
    return USB_UNSUPPORT;
  }
}

/*******************************************************************************
* Function Name  : Standard_GetInterface.
* Description    : Return the Alternate Setting of the current interface.
* Input          : Length - How many bytes are needed.
* Output         : None.
* Return         : Return 0, if the request is invalid when "Length" is 0.
*                  Return "Buffer" if the "Length" is not 0.
*******************************************************************************/
u8 *Standard_GetInterface(u16 Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength =
      sizeof(pInformation->Current_AlternateSetting);
    return 0;
  }
  pUser_Standard_Requests->User_GetInterface();
  return (u8 *)&pInformation->Current_AlternateSetting;
}

/*******************************************************************************
* Function Name  : Standard_SetInterface.
* Description    : This routine is called to set the interface.
*                  Then each class should configure the interface them self.
* Input          : None.
* Output         : None.
* Return         : - Return USB_SUCCESS, if the request is performed.
*                  - Return USB_UNSUPPORT, if the request is invalid.
*******************************************************************************/
RESULT Standard_SetInterface(void)
{
  RESULT Re;
  /*Test if the specified Interface and Alternate Setting are supported by
    the application Firmware*/
  Re = (*pProperty->Class_Get_Interface_Setting)(pInformation->USBwIndex0, pInformation->USBwValue0);
  //RESULT  (*Class_Get_Interface_Setting)(u8 Interface, u8 AlternateSetting);
  if (pInformation->Current_Configuration != 0)
  {
    if ((Re != USB_SUCCESS) || (pInformation->USBwIndex1 != 0)
        || (pInformation->USBwValue1 != 0))
    {
      return  USB_UNSUPPORT;
    }
    else if (Re == USB_SUCCESS)
    {
      pUser_Standard_Requests->User_SetInterface();
      pInformation->Current_Interface = pInformation->USBwIndex0;  //Current_Interface 选中的接口
      pInformation->Current_AlternateSetting = pInformation->USBwValue0; 
      return USB_SUCCESS;	  //Current_AlternateSetting:由SetInterface和GetInterface 决定
    }

  }

  return USB_UNSUPPORT;
}

/*******************************************************************************
* Function Name  : Standard_GetStatus.
* Description    : Copy the device request data to "StatusInfo buffer".
* Input          : - Length - How many bytes are needed.
* Output         : None.
* Return         : Return 0, if the request is at end of data block,
*                  or is invalid when "Length" is 0.
*******************************************************************************/
u8 *Standard_GetStatus(u16 Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = 2;
    return 0;
  }

  StatusInfo.w = 0;
  /* Reset Status Information */

  if (Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
  {
    /*Get Device Status */
    u8 Feature = pInformation->Current_Feature;

    /* Remote Wakeup enabled */
    if (ValBit(Feature, 5))
    {
      SetBit(StatusInfo0, 1);
    }

    /* Bus-powered */
    if (ValBit(Feature, 6))
    {
      ClrBit(StatusInfo0, 0);
    }
    else /* Self-powered */
    {
      SetBit(StatusInfo0, 0);
    }
  }
  /*Interface Status*/
  else if (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
  {
    return (u8 *)&StatusInfo;
  }
  /*Get EndPoint Status*/
  else if (Type_Recipient == (STANDARD_REQUEST | ENDPOINT_RECIPIENT))
  {
    u8 Related_Endpoint;
    u8 wIndex0 = pInformation->USBwIndex0;

    Related_Endpoint = (wIndex0 & 0x0f);
    if (ValBit(wIndex0, 7))
    {
      /* IN endpoint */
      if (_GetTxStallStatus(Related_Endpoint))
      {
        SetBit(StatusInfo0, 0); /* IN Endpoint stalled */
      }
    }
    else
    {
      /* OUT endpoint */
      if (_GetRxStallStatus(Related_Endpoint))
      {
        SetBit(StatusInfo0, 0); /* OUT Endpoint stalled */
      }
    }

  }
  else
  {
    return NULL;
  }
  pUser_Standard_Requests->User_GetStatus();
  return (u8 *)&StatusInfo;
}

/*******************************************************************************
* Function Name  : Standard_ClearFeature.
* Description    : Clear or disable a specific feature.
* Input          : None.
* Output         : None.
* Return         : - Return USB_SUCCESS, if the request is performed.
*                  - Return USB_UNSUPPORT, if the request is invalid.
*******************************************************************************/
RESULT Standard_ClearFeature(void)
{
  u32     Type_Rec = Type_Recipient;
  u32     Status;


  if (Type_Rec == (STANDARD_REQUEST | DEVICE_RECIPIENT))
  {/*Device Clear Feature*/
    ClrBit(pInformation->Current_Feature, 5);
    return USB_SUCCESS;
  }
  else if (Type_Rec == (STANDARD_REQUEST | ENDPOINT_RECIPIENT))
  {/*EndPoint Clear Feature*/
    DEVICE* pDev;
    u32 Related_Endpoint;
    u32 wIndex0;
    u32 rEP;

    if ((pInformation->USBwValue != ENDPOINT_STALL)
        || (pInformation->USBwIndex1 != 0))
    {
      return USB_UNSUPPORT;
    }

    pDev = &Device_Table;
    wIndex0 = pInformation->USBwIndex0;
    rEP = wIndex0 & ~0x80;
    Related_Endpoint = ENDP0 + rEP;

    if (ValBit(pInformation->USBwIndex0, 7))
    {
      /*Get Status of endpoint & stall the request if the related_ENdpoint
      is Disabled*/
      Status = _GetEPTxStatus(Related_Endpoint);
    }
    else
    {
      Status = _GetEPRxStatus(Related_Endpoint);
    }

    if ((rEP >= pDev->Total_Endpoint) || (Status == 0)
        || (pInformation->Current_Configuration == 0))
    {
      return USB_UNSUPPORT;
    }


    if (wIndex0 & 0x80)
    {
      /* IN endpoint */
      if (_GetTxStallStatus(Related_Endpoint ))
      {
        ClearDTOG_TX(Related_Endpoint);
        SetEPTxStatus(Related_Endpoint, EP_TX_VALID);
      }
    }
    else
    {
      /* OUT endpoint */
      if (_GetRxStallStatus(Related_Endpoint))
      {
        if (Related_Endpoint == ENDP0)
        {
          /* After clear the STALL, enable the default endpoint receiver */
          SetEPRxCount(Related_Endpoint, Device_Property.MaxPacketSize);
          _SetEPRxStatus(Related_Endpoint, EP_RX_VALID);
        }
        else
        {
          ClearDTOG_RX(Related_Endpoint);
          _SetEPRxStatus(Related_Endpoint, EP_RX_VALID);
        }
      }
    }
    pUser_Standard_Requests->User_ClearFeature();
    return USB_SUCCESS;
  }

  return USB_UNSUPPORT;
}

/*******************************************************************************
* Function Name  : Standard_SetEndPointFeature
* Description    : Set or enable a specific feature of EndPoint
* Input          : None.
* Output         : None.
* Return         : - Return USB_SUCCESS, if the request is performed.
*                  - Return USB_UNSUPPORT, if the request is invalid.
*******************************************************************************/
RESULT Standard_SetEndPointFeature(void)
{
  u32    wIndex0;
  u32    Related_Endpoint;
  u32    rEP;
  u32   Status;

  wIndex0 = pInformation->USBwIndex0;
  rEP = wIndex0 & ~0x80;
  Related_Endpoint = ENDP0 + rEP;

  if (ValBit(pInformation->USBwIndex0, 7))
  {
    /* get Status of endpoint & stall the request if the related_ENdpoint
    is Disabled*/
    Status = _GetEPTxStatus(Related_Endpoint);
  }
  else
  {
    Status = _GetEPRxStatus(Related_Endpoint);
  }

  if (Related_Endpoint >= Device_Table.Total_Endpoint
      || pInformation->USBwValue != 0 || Status == 0
      || pInformation->Current_Configuration == 0)
  {
    return USB_UNSUPPORT;
  }
  else
  {
    if (wIndex0 & 0x80)
    {
      /* IN endpoint */
      _SetEPTxStatus(Related_Endpoint, EP_TX_STALL);
    }

    else
    {
      /* OUT endpoint */
      _SetEPRxStatus(Related_Endpoint, EP_RX_STALL);
    }
  }
  pUser_Standard_Requests->User_SetEndPointFeature();
  return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : Standard_SetDeviceFeature.
* Description    : Set or enable a specific feature of Device.
* Input          : None.
* Output         : None.
* Return         : - Return USB_SUCCESS, if the request is performed.
*                  - Return USB_UNSUPPORT, if the request is invalid.
*******************************************************************************/
RESULT Standard_SetDeviceFeature(void)
{
  SetBit(pInformation->Current_Feature, 5);
  pUser_Standard_Requests->User_SetDeviceFeature();
  return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : Standard_GetDescriptorData.
* Description    : Standard_GetDescriptorData is used for descriptors transfer.
*                : This routine is used for the descriptors resident in Flash
*                  or RAM
*                  pDesc can be in either Flash or RAM
*                  The purpose of this routine is to have a versatile way to
*                  response descriptors request. It allows user to generate
*                  certain descriptors with software or read descriptors from
*                  external storage part by part.
* Input          : - Length - Length of the data in this transfer.
*                  - pDesc - A pointer points to descriptor struct.
*                  The structure gives the initial address of the descriptor and
*                  its original size.
* Output         : None.
* Return         : Address of a part of the descriptor pointed by the Usb_
*                  wOffset The buffer pointed by this address contains at least
*                  Length bytes.
*******************************************************************************/
u8 *Standard_GetDescriptorData(u16 Length, ONE_DESCRIPTOR *pDesc)
{
  u32  wOffset;

  wOffset = pInformation->Ctrl_Info.Usb_wOffset;
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = pDesc->Descriptor_Size - wOffset;
    return 0;
  }

  return pDesc->Descriptor + wOffset;
}

/*******************************************************************************
* Function Name  : DataStageOut.
* Description    : Data stage of a Control Write Transfer.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void DataStageOut(void)
{
  ENDPOINT_INFO *pEPinfo = &pInformation->Ctrl_Info;
  u32 save_rLength;

  save_rLength = pEPinfo->Usb_rLength;

  if (pEPinfo->CopyData && save_rLength)
  {
    u8 *Buffer;
    u32 Length;

    Length = pEPinfo->PacketSize;
    if (Length > save_rLength)
    {
      Length = save_rLength;
    }

    Buffer = (*pEPinfo->CopyData)(Length);
    pEPinfo->Usb_rLength -= Length;
    pEPinfo->Usb_rOffset += Length;

    PMAToUserBufferCopy(Buffer, GetEPRxAddr(ENDP0), Length);
  }

  if (pEPinfo->Usb_rLength != 0)
  {
    vSetEPRxStatus(EP_RX_VALID);/* re-enable for next data reception */
    _SetEPTxCount(ENDP0, 0);
    SaveTState=EP_TX_VALID;/* Expect the host to abort the data OUT stage */
  }
  /* Set the next State*/
  if (pEPinfo->Usb_rLength >= pEPinfo->PacketSize)
  {
    pInformation->ControlState = OUT_DATA;
  }
  else
  {
    if (pEPinfo->Usb_rLength > 0)
    {
      pInformation->ControlState = LAST_OUT_DATA;
    }
    else if (pEPinfo->Usb_rLength == 0)
    {
      pInformation->ControlState = WAIT_STATUS_IN;
      USB_StatusIn();
    }
  }
}

/*******************************************************************************
* Function Name  : DataStageIn.
* Description    : Data stage of a Control Read Transfer.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void DataStageIn(void)
{
  ENDPOINT_INFO *pEPinfo = &pInformation->Ctrl_Info;
  u32 save_wLength = pEPinfo->Usb_wLength;		 //设置包中wLength的副本
  u32 ControlState = pInformation->ControlState;	 //内核状态

  u8 *DataBuffer;
  u32 Length;

  if ((save_wLength == 0) && (ControlState == LAST_IN_DATA))   //  LAST_IN_DATA=4	传输完
  {
    if(Data_Mul_MaxPacketSize == TRUE)	 //bool Data_Mul_MaxPacketSize = FALSE;	 1
    {
      /* No more data to send and empty packet */
      //Send0LengthData();
	  _SetEPTxCount(ENDP0, 0);
      SaveTState=EP_TX_VALID;
      ControlState = LAST_IN_DATA;	 //  LAST_IN_DATA=4
      Data_Mul_MaxPacketSize = FALSE;
    }
    else 
    {
      /* No more data to send so STALL the TX Status*/
      ControlState = WAIT_STATUS_OUT;
      SaveTState=EP_TX_STALL;
    }
    
    goto Expect_Status_Out;
  }
  //传输没有结束
  Length = pEPinfo->PacketSize;	
  ControlState = (save_wLength <= Length) ? LAST_IN_DATA : IN_DATA;

  if (Length > save_wLength)
  {
    Length = save_wLength;
  }

  DataBuffer = (*pEPinfo->CopyData)(Length);   //取地址

  UserToPMABufferCopy(DataBuffer, GetEPTxAddr(ENDP0), Length);
								  //获得EP0的地址
  _SetEPTxCount(ENDP0, Length);	  //设置EP0发送数据字节数

  pEPinfo->Usb_wLength -= Length;
  pEPinfo->Usb_wOffset += Length;
  SaveTState=EP_TX_VALID;

  USB_StatusOut();/* Expect the host to abort the data IN stage 等待主机取消数据输入*/
				  //#define USB_StatusOut() vSetEPRxStatus(EP_RX_VALID)
Expect_Status_Out:
  pInformation->ControlState = ControlState;
}

/*******************************************************************************
* Function Name  : NoData_Setup0.
* Description    : Proceed the processing of setup request without data stage.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void NoData_Setup0(void)
{
  RESULT Result = USB_UNSUPPORT;  //USB_UNSUPPORT=1
  u32 RequestNo = pInformation->USBbRequest;
  u32 ControlState;
						   //0x00				  //0
  if (Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
  {	  //#define Type_Recipient (pInformation->USBbmRequestType & (REQUEST_TYPE | RECIPIENT))
    /* Device Request*/												  //0x60		 0x1F
    /* SET_CONFIGURATION*/
	switch(RequestNo)
	{
    case SET_CONFIGURATION:	  //  SET_CONFIGURATION=9
      Result = Standard_SetConfiguration();
	  break;
    /*SET ADDRESS*/
    case SET_ADDRESS:
      if ((pInformation->USBwValue0 > 127) || (pInformation->USBwValue1 != 0)
          || (pInformation->USBwIndex != 0)
          || (pInformation->Current_Configuration != 0))
        /* Device Address should be 127 or less*/
      {
        ControlState = STALLED;
        goto exit_NoData_Setup0;
      }
      else
        Result = USB_SUCCESS;
	  break;
    /*SET FEATURE for Device
	typedef enum _FEATURE_SELECTOR
	{
	  ENDPOINT_STALL,
	  DEVICE_REMOTE_WAKEUP
	} FEATURE_SELECTOR;
	*/
    case SET_FEATURE:
      if ((pInformation->USBwValue0 == DEVICE_REMOTE_WAKEUP)
          && (pInformation->USBwIndex == 0)
          && (ValBit(pInformation->Current_Feature, 5)))
      {
        Result = Standard_SetDeviceFeature();
      }
      else
        Result = USB_UNSUPPORT;
	  break;
    /*Clear FEATURE for Device */
    case CLEAR_FEATURE:
      if (pInformation->USBwValue0 == DEVICE_REMOTE_WAKEUP
          && pInformation->USBwIndex == 0
          && ValBit(pInformation->Current_Feature, 5))
		  //	#define ValBit(VAR,Place)    (VAR & (1 << Place))
      {
        Result = Standard_ClearFeature();
      }
      else
        Result = USB_UNSUPPORT;
	  break;
	  }

  }

  /* Interface Request*/
  else if (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
  {
    /*SET INTERFACE*/
    if (RequestNo == SET_INTERFACE)
    {
      Result = Standard_SetInterface();
    }
  }

  /* EndPoint Request*/
  else if (Type_Recipient == (STANDARD_REQUEST | ENDPOINT_RECIPIENT))
  {								  //0x60
    /*CLEAR FEATURE for EndPoint*/
    if (RequestNo == CLEAR_FEATURE)
    {
      Result = Standard_ClearFeature();
    }
    /* SET FEATURE for EndPoint*/
    else if (RequestNo == SET_FEATURE)
    {
      Result = Standard_SetEndPointFeature();
    }
  }
  else
  {
    Result = USB_UNSUPPORT;
  }


  if (Result != USB_SUCCESS)
  {
    Result = (*pProperty->Class_NoData_Setup)(RequestNo);
    if (Result == USB_NOT_READY)
    {
      ControlState = PAUSE;
      goto exit_NoData_Setup0;
    }
  }

  if (Result != USB_SUCCESS)
  {
    ControlState = STALLED;	  // 9
    goto exit_NoData_Setup0;
  }

  ControlState = WAIT_STATUS_IN;/* After no data stage SETUP  7 */

  USB_StatusIn(); //#define USB_StatusIn() Send0LengthData()

exit_NoData_Setup0:
  pInformation->ControlState = ControlState;
  return;
}

/*******************************************************************************
* Function Name  : Data_Setup0.
* Description    : Proceed the processing of setup request with data stage.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Data_Setup0(void)
{
  u8 *(*CopyRoutine)(u16);
  RESULT Result;
  u32 Request_No = pInformation->USBbRequest;

  u32 Related_Endpoint, Reserved;
  u32 wOffset, Status;



  CopyRoutine = NULL;
  wOffset = 0;

  if (Request_No == GET_DESCRIPTOR)	   //如果是获取描述符请求
  {								//0x00				  //0
    if (Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
    {
      u8 wValue1 = pInformation->USBwValue1; //wValue1表示描述符的类型编号
	  switch(wValue1)
     	{
	  case DEVICE_DESCRIPTOR:	   //圈圈 p78 描述符类型   设备描述符
        CopyRoutine = pProperty->GetDeviceDescriptor;
      	break;
      case CONFIG_DESCRIPTOR: //圈圈 p78 配置描述符
        CopyRoutine = pProperty->GetConfigDescriptor;
      	break;
      case STRING_DESCRIPTOR:  //圈圈 p78 字符串描述符
        CopyRoutine = pProperty->GetStringDescriptor;
		break;
		}		
    }
  }

  /*GET STATUS*/   
  else if ((Request_No == GET_STATUS) && (pInformation->USBwValue == 0)
           && (pInformation->USBwLength == 0x0002)
           && (pInformation->USBwIndex1 == 0))
  {
    /* GET STATUS for Device*/	//0x00			   //0
    if ((Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
        && (pInformation->USBwIndex == 0))
    {
      CopyRoutine = Standard_GetStatus;
    }

    /* GET STATUS for Interface*/
    else if (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
    {
      if (((*pProperty->Class_Get_Interface_Setting)(pInformation->USBwIndex0, 0) == USB_SUCCESS)
          && (pInformation->Current_Configuration != 0))
      {
        CopyRoutine = Standard_GetStatus;
      }
    }

    /* GET STATUS for EndPoint*/
    else if (Type_Recipient == (STANDARD_REQUEST | ENDPOINT_RECIPIENT))
    {
      Related_Endpoint = (pInformation->USBwIndex0 & 0x0f);
      Reserved = pInformation->USBwIndex0 & 0x70;

      if (ValBit(pInformation->USBwIndex0, 7))
      {
        /*Get Status of endpoint & stall the request if the related_ENdpoint
        is Disabled*/
        Status = _GetEPTxStatus(Related_Endpoint);
      }
      else
      {
        Status = _GetEPRxStatus(Related_Endpoint);
      }

      if ((Related_Endpoint < Device_Table.Total_Endpoint) && (Reserved == 0)
          && (Status != 0))
      {
        CopyRoutine = Standard_GetStatus;
      }
    }

  }

  /*GET CONFIGURATION*/
  else if (Request_No == GET_CONFIGURATION)
  {							//0x00			   //0
    if (Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
    {
      CopyRoutine = Standard_GetConfiguration;
    }
  }
  /*GET INTERFACE*/
  else if (Request_No == GET_INTERFACE)
  {
    if ((Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
        && (pInformation->Current_Configuration != 0) && (pInformation->USBwValue == 0)
        && (pInformation->USBwIndex1 == 0) && (pInformation->USBwLength == 0x0001)
        && ((*pProperty->Class_Get_Interface_Setting)(pInformation->USBwIndex0, 0) == USB_SUCCESS))
    {
      CopyRoutine = Standard_GetInterface;
    }

  }
  
  if (CopyRoutine)
  {
    pInformation->Ctrl_Info.Usb_wOffset = wOffset;
    pInformation->Ctrl_Info.CopyData = CopyRoutine;
    /* sb in the original the cast to word was directly */
    /* now the cast is made step by step */
    (*CopyRoutine)(0);
    Result = USB_SUCCESS;
  }
  else
  {
    Result = (*pProperty->Class_Data_Setup)(pInformation->USBbRequest);
    if (Result == USB_NOT_READY)
    {
      pInformation->ControlState = PAUSE;
      return;
    }
  }

  if (pInformation->Ctrl_Info.Usb_wLength == 0xFFFF)
  {
    /* Data is not ready, wait it */
    pInformation->ControlState = PAUSE;
    return;
  }
  if ((Result == USB_UNSUPPORT) || (pInformation->Ctrl_Info.Usb_wLength == 0))
  {
    /* Unsupported request */
    pInformation->ControlState = STALLED;
    return;
  }


  if (ValBit(pInformation->USBbmRequestType, 7))
  {
    /* Device ==> Host */
    vu32 wLength = pInformation->USBwLength;
     
    /* Restrict the data length to be the one host asks */
    if (pInformation->Ctrl_Info.Usb_wLength > wLength)
    {
      pInformation->Ctrl_Info.Usb_wLength = wLength;
    }
    
    else if (pInformation->Ctrl_Info.Usb_wLength < pInformation->USBwLength)
    {
      if (pInformation->Ctrl_Info.Usb_wLength < pProperty->MaxPacketSize)
      {
        Data_Mul_MaxPacketSize = FALSE;
      }
      else if ((pInformation->Ctrl_Info.Usb_wLength % pProperty->MaxPacketSize) == 0)
      {
        Data_Mul_MaxPacketSize = TRUE;
      }
    }   

    pInformation->Ctrl_Info.PacketSize = pProperty->MaxPacketSize;
    DataStageIn();
  }
  else
  {
    pInformation->ControlState = OUT_DATA;
    vSetEPRxStatus(EP_RX_VALID); /* enable for next data reception */
  }

  return;
}

/*******************************************************************************
* Function Name  : Setup0_Process
* Description    : Get the device request data and dispatch to individual process.
* Input          : None.
* Output         : None.
* Return         : Post0_Process.
*******************************************************************************/
u8 Setup0_Process(void)
{  //首先是主机发送SETUP分组的SETUP阶段

  union		//共同提只能对第一个成员赋初值
  {
    u8* b;
    u16* w;
  } pBuf;

  pBuf.b = PMAAddr + (u8 *)(_GetEPRxAddr(ENDP0) * 2); /* *2 for 32 bits addr */
		  //PMAAddr  (0x40006000L)	分组缓冲区的首地址
		  //_GetEPRxAddr 获得EP0的接受缓冲区地址
  if (pInformation->ControlState != PAUSE)
  {
    pInformation->USBbmRequestType = *pBuf.b++; /* bmRequestType */
    pInformation->USBbRequest = *pBuf.b++; /* bRequest */
    pBuf.w++;  /* word not accessed because of 32 bits addressing */
    pInformation->USBwValue = ByteSwap(*pBuf.w++); /* wValue */
    pBuf.w++;  /* word not accessed because of 32 bits addressing */
    pInformation->USBwIndex  = ByteSwap(*pBuf.w++); /* wIndex */
    pBuf.w++;  /* word not accessed because of 32 bits addressing */
    pInformation->USBwLength = *pBuf.w; /* wLength */
  }

  pInformation->ControlState = SETTING_UP;
  if (pInformation->USBwLength == 0)	  //然后是主机发送零个或多个数据的数据阶段
  {
    /* Setup with no data stage */
    NoData_Setup0();
  }
  else
  {
    /* Setup with data stage */
    Data_Setup0();
  }
  return Post0_Process();
}

/*******************************************************************************
* Function Name  : In0_Process
* Description    : Process the IN token on all default endpoint.
* Input          : None.
* Output         : None.
* Return         : Post0_Process.
*******************************************************************************/
u8 In0_Process(void)
{
  u32 ControlState = pInformation->ControlState;   //这个内核状态是怎么得到的？由谁来决定

  if ((ControlState == IN_DATA) || (ControlState == LAST_IN_DATA))	  //2  4
  {
    DataStageIn();
    /* ControlState may be changed outside the function */
    ControlState = pInformation->ControlState;
  }

  else if (ControlState == WAIT_STATUS_IN)
  {
    if ((pInformation->USBbRequest == SET_ADDRESS) &&		//  SET_ADDRESS=5
        (Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT)))	//STANDARD_REQUEST=0x00  DEVICE_RECIPIENT=0
    {	//Type_Recipient (pInformation->USBbmRequestType & (REQUEST_TYPE | RECIPIENT))   0x60	0x1F
      SetDeviceAddress(pInformation->USBwValue0);
      pUser_Standard_Requests->User_SetDeviceAddress();
    }
    (*pProperty->Process_Status_IN)();	   //Process_Status_IN()空函数
    ControlState = STALLED;
  }

  else
  {
    ControlState = STALLED;
  }

  pInformation->ControlState = ControlState;

  return Post0_Process();	// return (pInformation->ControlState == PAUSE);
}

/*******************************************************************************
* Function Name  : Out0_Process
* Description    : Process the OUT token on all default endpoint.
* Input          : None.
* Output         : None.
* Return         : Post0_Process.
*******************************************************************************/
u8 Out0_Process(void)
{
  u32 ControlState = pInformation->ControlState;

  if ((ControlState == OUT_DATA) || (ControlState == LAST_OUT_DATA))
  {
    DataStageOut();
    ControlState = pInformation->ControlState; /* may be changed outside the function */
  }

  else if (ControlState == WAIT_STATUS_OUT)
  {
    (*pProperty->Process_Status_OUT)();
    ControlState = STALLED;
  }

  else if ((ControlState == IN_DATA) || (ControlState == LAST_IN_DATA))
  {
    /* host aborts the transfer before finish */
    ControlState = STALLED;
  }

  /* Unexpect state, STALL the endpoint */
  else
  {
    ControlState = STALLED;
  }

  pInformation->ControlState = ControlState;

  return Post0_Process();
}

/*******************************************************************************
* Function Name  : Post0_Process
* Description    : Stall the Endpoint 0 in case of error.
* Input          : None.
* Output         : None.
* Return         : - 0 if the control State is in PAUSE
*                  - 1 if not.
*******************************************************************************/
u8 Post0_Process(void)
{
  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);

  if (pInformation->ControlState == STALLED)
  {
    vSetEPRxStatus(EP_RX_STALL);
    SaveTState=EP_TX_STALL;
  }

  return (pInformation->ControlState == PAUSE);
}

/*******************************************************************************
* Function Name  : SetDeviceAddress.
* Description    : Set the device and all the used Endpoints addresses.
* Input          : - Val: device adress.
* Output         : None.
* Return         : None.
*******************************************************************************/
void SetDeviceAddress(u8 Val)
{
  u32 i;
  u32 nEP = Device_Table.Total_Endpoint;  //设置端点使用的数量  nEP=3

  /* set address in every used endpoint */
  for (i = 0; i < nEP; i++)		//i=0 
  {								//i=1
    _SetEPAddress((u8)i, (u8)i);//i=2 设置端点地址											   0x8f8f
  } /* for */ //#define _SetEPAddress(bEpNum,bAddr) _SetENDPOINT(bEpNum,_GetENDPOINT(bEpNum) & EPREG_MASK | bAddr)
  _SetDADDR(Val | DADDR_EF); /* set device address and enable function */
}						   //DADDR_EF=0x80

/*******************************************************************************
* Function Name  : NOP_Process
* Description    : No operation function.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void NOP_Process(void)
{
}

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
