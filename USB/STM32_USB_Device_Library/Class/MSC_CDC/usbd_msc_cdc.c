//
// Created by MightyPork on 2017/11/07.
//

#include "platform.h"
#include "framework/system_settings.h"
#include "usbd_desc.h"
#include "task_main.h"
#include "usbd_msc.h"
#include "usbd_cdc.h"
#include "usbd_msc_cdc.h"

#define USBD_MSC_CDC_CONFIG_DESC_SIZ 98
/* USB Mass storage device Configuration Descriptor */
/*   All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */

// NOTE: This must NOT be in flash, otherwise the driver / peripheral crashes
__ALIGN_BEGIN uint8_t USBD_MSC_CDC_CfgFSDesc[USBD_MSC_CDC_CONFIG_DESC_SIZ]  __ALIGN_END =
    {
/*0*/   0x09,   /* bLength: Configuation Descriptor size */
        USB_DESC_TYPE_CONFIGURATION,   /* bDescriptorType: Configuration */
        LOBYTE(USBD_MSC_CDC_CONFIG_DESC_SIZ), // TODO
        HIBYTE(USBD_MSC_CDC_CONFIG_DESC_SIZ),
        3,  /* bNumInterfaces - 1 MSC + 2 composite CDC ACM */
        0x01,   /* bConfigurationValue: */
        0,   /* iConfiguration: - string descriptor */
        0xC0,   /* bmAttributes: - self powered */
        150,   /* MaxPower 300 mA */

            /********************  Mass Storage interface ********************/
/*9*/       0x09,   /* bLength: Interface Descriptor size */
            USB_DESC_TYPE_INTERFACE,   /* bDescriptorType: */
            0x00,   /* bInterfaceNumber: Number of Interface */
            0x00,   /* bAlternateSetting: Alternate setting */
            0x02,   /* bNumEndpoints*/
            0x08,   /* bInterfaceClass: MSC Class */ // #14
            0x06,   /* bInterfaceSubClass : SCSI transparent*/
            0x50,   /* nInterfaceProtocol */
            0x04,   /* iInterface: String descriptor */

                /********************  Mass Storage Endpoints ********************/
/*18*/          0x07,   /*Endpoint descriptor length = 7*/
                0x05,   /*Endpoint descriptor type */
                MSC_EPIN_ADDR,   /*Endpoint address (IN, address 1) */
                0x02,   /*Bulk endpoint type */
                LOBYTE(MSC_MAX_FS_PACKET),
                HIBYTE(MSC_MAX_FS_PACKET),
                0x00,   /*Polling interval in milliseconds */

/*25*/          0x07,   /*Endpoint descriptor length = 7 */
                0x05,   /*Endpoint descriptor type */
                MSC_EPOUT_ADDR,   /*Endpoint address (OUT, address 1) */
                0x02,   /*Bulk endpoint type */
                LOBYTE(MSC_MAX_FS_PACKET),
                HIBYTE(MSC_MAX_FS_PACKET),
                0x00,     /*Polling interval in milliseconds*/

            /********************** IFACE ASSOC *************************/
/*32*/      0x08, /* bLength */
            USB_DESC_TYPE_IFACE_ASSOCIATION, /* bDescriptorType */
            0x01, /* bFirstInterface */
            0x02, /* bInterfaceCount */
            0x02, /* bFunctionClass */  // #36
            0x02, /* bFunctionSubClass (ACM) */
            0x01, /* bFunctionProtocol (AT-COMMANDS) */
            0x05, /* iFunction: string descriptor */

            /********************** ACM interface **********************/
/*40*/      0x09,   /* bLength: Interface Descriptor size */
            USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: Interface */
            0x01,   /* bInterfaceNumber: Number of Interface */
            0x00,   /* bAlternateSetting: Alternate setting */
            0x01,   /* bNumEndpoints: One endpoints used */
            0x02,   /* bInterfaceClass: Communication Interface Class */  // #45
            0x02,   /* bInterfaceSubClass: Abstract Control Model */
            0x01,   /* bInterfaceProtocol: Common AT commands */
            0x05,   /* iInterface: string descriptor */

                /**************** Header Functional Descriptor ***************/
/*49*/          0x05,   /* bLength: Endpoint Descriptor size */
                0x24,   /* bDescriptorType: CS_INTERFACE */
                0x00,   /* bDescriptorSubtype: Header Func Desc */
                0x10,   /* bcdCDC: spec release number */
                0x01,

                /*********** Call Management Functional Descriptor **********/
/*54*/          0x05,   /* bFunctionLength */
                0x24,   /* bDescriptorType: CS_INTERFACE */
                0x01,   /* bDescriptorSubtype: Call Management Func Desc */
                0x00,   /* bmCapabilities: D0+D1 */
                0x02,   /* bDataInterface: 2 */

                /*************** ACM Functional Descriptor ***************/
/*59*/          0x04,   /* bFunctionLength */
                0x24,   /* bDescriptorType: CS_INTERFACE */
                0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
                0x06,   /* bmCapabilities XXX was:0x02? */

                /************* Union Functional Descriptor **************/
/*63*/          0x05,   /* bFunctionLength */
                0x24,   /* bDescriptorType: CS_INTERFACE */
                0x06,   /* bDescriptorSubtype: Union func desc */
                0x01,   /* bMasterInterface: Communication class interface */
                0x02,   /* bSlaveInterface0: Data Class Interface */

                /********************  CDC Endpoints ********************/

                /// Command endpoint
/*68*/          0x07,                           /* bLength: Endpoint Descriptor size */
                USB_DESC_TYPE_ENDPOINT,   /* bDescriptorType: Endpoint */
                CDC_CMD_EP,                     /* bEndpointAddress */
                0x03,                           /* bmAttributes: Interrupt */
                LOBYTE(CDC_CMD_PACKET_SIZE),     /* wMaxPacketSize: TODO: 2?*/
                HIBYTE(CDC_CMD_PACKET_SIZE),
                0xFF,                           /* bInterval: TODO was 0x10?*/

            /********** CDC Data Class Interface Descriptor ***********/
/*75*/      0x09,   /* bLength: Endpoint Descriptor size */
            USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: */
            0x02,   /* bInterfaceNumber: Number of Interface */
            0x00,   /* bAlternateSetting: Alternate setting */
            0x02,   /* bNumEndpoints: Two endpoints used */
            0x0A,   /* bInterfaceClass: CDC */    // #80
            0x00,   /* bInterfaceSubClass: */
            0x00,   /* bInterfaceProtocol: */
            0x06,   /* iInterface: TODO: string descriptor */

                /// Endpoint OUT Descriptor
/*84*/          0x07,   /* bLength: Endpoint Descriptor size */
                USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
                CDC_OUT_EP,                        /* bEndpointAddress */
                0x02,                              /* bmAttributes: Bulk */
                LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: TODO 8? */
                HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
                0x00,                              /* bInterval: ignore for Bulk transfer */

                /// Endpoint IN Descriptor
/*91*/          0x07,   /* bLength: Endpoint Descriptor size */
                USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
                CDC_IN_EP,                         /* bEndpointAddress */
                0x02,                              /* bmAttributes: Bulk */
                LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: TODO 16? */
                HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
                0x00                               /* bInterval: ignore for Bulk transfer */
    };


/* USB Standard Device Descriptor */
__ALIGN_BEGIN  uint8_t USBD_MSC_CDC_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC]  __ALIGN_END =
    {
        USB_LEN_DEV_QUALIFIER_DESC,
        USB_DESC_TYPE_DEVICE_QUALIFIER,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        MSC_MAX_FS_PACKET,
        0x01,
        0x00,
    };

static uint8_t USBD_MSC_CDC_Init(struct _USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    // this is not correct, but will work if neither returns BUSY (which they don't)
    uint8_t ret = 0;
    ret |= USBD_MSC_Init(pdev, cfgidx);
    ret |= USBD_CDC_Init(pdev, cfgidx);
    return ret;
}

static uint8_t USBD_MSC_CDC_DeInit(struct _USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    uint8_t ret = 0;
    ret |= USBD_MSC_DeInit(pdev, cfgidx);
    ret |= USBD_CDC_DeInit(pdev, cfgidx);
    return ret;
}

/* Control Endpoints*/
static uint8_t USBD_MSC_CDC_Setup(struct _USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    static uint8_t interface_bit;

    // handle common
    switch (req->bmRequest & USB_REQ_TYPE_MASK) {
        case USB_REQ_TYPE_STANDARD:
            switch (req->bRequest) {
                // those don't really do anything useful, but each class implements
                // them and it would send the data twice.
                case USB_REQ_GET_INTERFACE :
                    USBD_CtlSendData(pdev, &interface_bit, 1);
                    return USBD_OK;

                case USB_REQ_SET_INTERFACE :
                    interface_bit = (uint8_t) (req->wValue);
                    return USBD_OK;
            }
    }

    // class specific, or Interface -> Clear Feature
    USBD_MSC_Setup(pdev, req);
    USBD_CDC_Setup(pdev, req);
    return USBD_OK;
}

#if 0
static uint8_t USBD_MSC_CDC_EP0_TxSent(struct _USBD_HandleTypeDef *pdev)
{
    // not handled by either
    xTaskNotify(tskUsbEventHandle, USBEVT_FLAG_EP0_TX_SENT, eSetBits);
    return USBD_OK;
}
#endif

static uint8_t USBD_MSC_CDC_EP0_RxReady(struct _USBD_HandleTypeDef *pdev)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(tskMainHandle, USBEVT_FLAG_EP0_RX_RDY, eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    //USBD_CDC_EP0_RxReady(pdev);
    return USBD_OK;
}

/* Class Specific Endpoints*/
static uint8_t USBD_MSC_CDC_DataIn(struct _USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(tskMainHandle, USBEVT_FLAG_EPx_IN(epnum), eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
 //   if (epnum == MSC_EPIN_ADDR||epnum==MSC_EPOUT_ADDR) USBD_MSC_DataIn(pdev, epnum);
 //   else if (epnum == CDC_IN_EP||epnum == CDC_CMD_EP) USBD_CDC_DataIn(pdev, epnum);
    return USBD_OK;
}

static uint8_t USBD_MSC_CDC_DataOut(struct _USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(tskMainHandle, USBEVT_FLAG_EPx_OUT(epnum), eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
//    if (epnum == MSC_EPIN_ADDR||epnum == MSC_EPOUT_ADDR) USBD_MSC_DataOut(pdev, epnum);
//    else if (epnum == CDC_OUT_EP||epnum == CDC_CMD_EP) USBD_CDC_DataOut(pdev, epnum);
    return USBD_OK;
}

#if 0
static uint8_t USBD_MSC_CDC_SOF(struct _USBD_HandleTypeDef *pdev)
{
    // not handled by either
    return USBD_OK;
}

static uint8_t USBD_MSC_CDC_IsoINIncomplete(struct _USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    // not handled by either
    return USBD_OK;
}

static uint8_t USBD_MSC_CDC_IsoOUTIncomplete(struct _USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    // not handled by either
    return USBD_OK;
}

static uint8_t *USBD_MSC_CDC_GetUsrStrDescriptor(struct _USBD_HandleTypeDef *pdev, uint8_t index, uint16_t *length)
{
    // not handled by either
    return USBD_OK;
}
#endif

#if 0
uint8_t  *USBD_MSC_CDC_GetHSCfgDesc (uint16_t *length)
{
//    *length = sizeof (USBD_MSC_CDC_CfgHSDesc);
//    return USBD_MSC_CDC_CfgHSDesc;
    *length = sizeof (USBD_MSC_CDC_CfgFSDesc);
    return USBD_MSC_CDC_CfgFSDesc;
}
#endif

uint8_t  *USBD_MSC_CDC_GetFSCfgDesc (uint16_t *length)
{
    *length = sizeof (USBD_MSC_CDC_CfgFSDesc);

    // Optionally hide CDC-ACM by setting its class to Vendor Specific
    bool cdc_visible = SystemSettings.visible_vcom;
    USBD_MSC_CDC_CfgFSDesc[36] = (uint8_t) (cdc_visible ? 0x02 : 0xFF);
    USBD_MSC_CDC_CfgFSDesc[45] = (uint8_t) (cdc_visible ? 0x02 : 0xFF);
    USBD_MSC_CDC_CfgFSDesc[80] = (uint8_t) (cdc_visible ? 0x0A : 0xFF);

    // Optionally hide settings (if lock jumper is installed)
    bool msc_visible = SystemSettings.editable;
    USBD_MSC_CDC_CfgFSDesc[14] = (uint8_t) (msc_visible ? 0x08 : 0xFF);

    return USBD_MSC_CDC_CfgFSDesc;
}

#if 0
uint8_t  *USBD_MSC_CDC_GetOtherSpeedCfgDesc (uint16_t *length)
{
//    *length = sizeof (USBD_MSC_CDC_OtherSpeedCfgDesc);
//    return USBD_MSC_CDC_OtherSpeedCfgDesc;
    *length = sizeof (USBD_MSC_CDC_CfgFSDesc);
    return USBD_MSC_CDC_CfgFSDesc;
}
#endif

uint8_t  *USBD_MSC_CDC_GetDeviceQualifierDescriptor (uint16_t *length)
{
    *length = sizeof (USBD_MSC_CDC_DeviceQualifierDesc);
    return USBD_MSC_CDC_DeviceQualifierDesc;
}

uint8_t  *USBD_MSC_CDC_GetUsrStrDescriptor(struct _USBD_HandleTypeDef *pdev, uint8_t index, uint16_t *length)
{
    const char *str;
    switch (index) {
        case 0x04: str = "Settings Virtual Mass Storage"; break;
        case 0x05: str = "Virtual Comport ACM"; break;
        case 0x06: str = "Virtual Comport CDC"; break;
        default: str = "No Descriptor";
    }
    USBD_GetString ((uint8_t *) str, USBD_StrDesc, length);
    return USBD_StrDesc;
}

USBD_ClassTypeDef USBD_MSC_CDC =
    {
        USBD_MSC_CDC_Init,
        USBD_MSC_CDC_DeInit,
        USBD_MSC_CDC_Setup,

        NULL, // USBD_MSC_CDC_EP0_TxSent,
        USBD_MSC_CDC_EP0_RxReady,

        USBD_MSC_CDC_DataIn,
        USBD_MSC_CDC_DataOut,

        NULL, // USBD_MSC_CDC_SOF,
        NULL, // USBD_MSC_CDC_IsoINIncomplete,
        NULL, // USBD_MSC_CDC_IsoOUTIncomplete,

        // we return only the FS one, others are useless
        USBD_MSC_CDC_GetFSCfgDesc, //USBD_MSC_CDC_GetHSCfgDesc,
        USBD_MSC_CDC_GetFSCfgDesc,
        USBD_MSC_CDC_GetFSCfgDesc, //USBD_MSC_CDC_GetOtherSpeedCfgDesc,

        USBD_MSC_CDC_GetDeviceQualifierDescriptor,
        USBD_MSC_CDC_GetUsrStrDescriptor
    };
