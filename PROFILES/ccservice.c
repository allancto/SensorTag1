/*******************************************************************************
  Filename:       ccservice.c
  Revised:        $Date: 2010-08-06 08:56:11 -0700 (Fri, 06 Aug 2010) $
  Revision:       $Revision: 23333 $
  
  Description:    This file contains the proprietary Connection Control Service
                  for use with the sensor tag application to enable functionality
                  limited by iOS Core Bluetooth and Android BLE.
  
  Copyright 2010 - 2015 Texas Instruments Incorporated. All rights reserved.
  
  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.
  
  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
  
  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
*******************************************************************************/

#ifdef FEATURE_OAD

/*********************************************************************
* INCLUDES
*/
#include "bcomdef.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"
#include "string.h"

#include "ccservice.h"
#include "st_util.h"
/*********************************************************************
* MACROS
*/

/*********************************************************************
* CONSTANTS
*/

/*********************************************************************
* TYPEDEFS
*/

/*********************************************************************
* GLOBAL VARIABLES
*/
// Connection Control Profile Service UUID: 0xCCC0
CONST uint8_t ccServiceServUUID[TI_UUID_SIZE] =
{
  TI_UUID(CCSERVICE_SERV_UUID),
};

// Characteristic 1 UUID: 0xCCC1
CONST uint8_t ccServicechar1UUID[TI_UUID_SIZE] =
{
  TI_UUID(CCSERVICE_CHAR1_UUID),
};

// Characteristic 2 UUID: 0xCCC2
CONST uint8_t ccServicechar2UUID[TI_UUID_SIZE] =
{
  TI_UUID(CCSERVICE_CHAR2_UUID),
};

// Characteristic 3 UUID: 0xCCC3
CONST uint8_t ccServicechar3UUID[TI_UUID_SIZE] =
{
  TI_UUID(CCSERVICE_CHAR3_UUID),
};

/*********************************************************************
* EXTERNAL VARIABLES
*/

/*********************************************************************
* EXTERNAL FUNCTIONS
*/

/*********************************************************************
* LOCAL VARIABLES
*/

static ccCBs_t *ccService_AppCBs = NULL;

/*********************************************************************
* Profile Attributes - variables
*/

// Connect Control Service attribute
static CONST gattAttrType_t ccServiceService = { TI_UUID_SIZE, ccServiceServUUID };

// Connect Control Service Characteristic 1 Properties
static uint8_t ccServiceChar1Props = GATT_PROP_READ | GATT_PROP_NOTIFY;

// Connect Control Service Characteristic 1 Value
static uint8_t ccServiceChar1[CCSERVICE_CHAR1_LEN] = {0, 0, 0, 0, 0, 0};

// Accelerometer Characteristic Configuration
static gattCharCfg_t *ccDataConfig;

#ifdef USER_DESCRIPTION
// Connect Control Service Characteristic 1 User Description
static uint8_t ccServiceChar1UserDesp[] = "Conn. Params";
#endif

// Connect Control Service Characteristic 2 Properties
static uint8_t ccServiceChar2Props = GATT_PROP_WRITE;

// Connect Control Service Characteristic 2 Value
static uint8_t ccServiceChar2[CCSERVICE_CHAR2_LEN] = { 0, 0, 0, 0, 0, 0, 0, 0 };

#ifdef USER_DESCRIPTION
// Connect Control Service Characteristic 2 User Description
static uint8_t ccServiceChar2UserDesp[] = "Conn. Params Req";
#endif

// Connect Control Service Characteristic 3 Properties
static uint8_t ccServiceChar3Props = GATT_PROP_WRITE;

// Connect Control Service Characteristic 3 Value
static uint8_t ccServiceChar3 = 0;

#ifdef USER_DESCRIPTION
// Connect Control Service Characteristic 3 User Description
static uint8_t ccServiceChar3UserDesp[] = "Disconnect Req";
#endif

/*********************************************************************
* Profile Attributes - Table
*/

static gattAttribute_t ccServiceAttrTbl[] =
{
  // Barometer Profile Service
  {
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8_t *)&ccServiceService                /* pValue */
  },

  // Characteristic 1 Declaration
  {
    { ATT_BT_UUID_SIZE, characterUUID },
    GATT_PERMIT_READ,
    0,
    &ccServiceChar1Props
  },

  // Characteristic Value 1
  {
    { TI_UUID_SIZE, ccServicechar1UUID },
    GATT_PERMIT_READ,
    0,
    ccServiceChar1
  },

  // Characteristic 1 configuration
  {
    { ATT_BT_UUID_SIZE, clientCharCfgUUID },
    GATT_PERMIT_READ | GATT_PERMIT_WRITE,
    0,
    (uint8_t *)&ccDataConfig
  },
#ifdef USER_DESCRIPTION
  // Characteristic 1 User Description
  {
    { ATT_BT_UUID_SIZE, charUserDescUUID },
    GATT_PERMIT_READ,
    0,
    ccServiceChar1UserDesp
  },
#endif
  // Characteristic 2 Declaration
  {
    { ATT_BT_UUID_SIZE, characterUUID },
    GATT_PERMIT_READ,
    0,
    &ccServiceChar2Props
  },

  // Characteristic Value 2
  {
    { TI_UUID_SIZE, ccServicechar2UUID },
    GATT_PERMIT_WRITE,
    0,
    ccServiceChar2
  },
#ifdef USER_DESCRIPTION
  // Characteristic 2 User Description
  {
    { ATT_BT_UUID_SIZE, charUserDescUUID },
    GATT_PERMIT_READ,
    0,
    ccServiceChar2UserDesp
  },
#endif
  // Characteristic 3 Declaration
  {
    { ATT_BT_UUID_SIZE, characterUUID },
    GATT_PERMIT_READ,
    0,
    &ccServiceChar3Props
  },

  // Characteristic Value 3
  {
    { TI_UUID_SIZE, ccServicechar3UUID },
    GATT_PERMIT_WRITE,
    0,
    &ccServiceChar3
  },
#ifdef USER_DESCRIPTION
  // Characteristic 3 User Description
  {
    { ATT_BT_UUID_SIZE, charUserDescUUID },
    GATT_PERMIT_READ,
    0,
    ccServiceChar3UserDesp
  },
#endif
};


/*********************************************************************
* LOCAL FUNCTIONS
*/
static bStatus_t ccService_ReadAttrCB(uint16_t connHandle, 
                                      gattAttribute_t *pAttr,
                                      uint8_t *pValue, uint16_t *pLen, 
                                      uint16_t offset, uint16_t maxLen, 
                                      uint8_t method);
static bStatus_t ccService_WriteAttrCB(uint16_t connHandle, 
                                       gattAttribute_t *pAttr,
                                       uint8_t *pValue, uint16_t len, 
                                       uint16_t offset, uint8_t method);

/*********************************************************************
* PROFILE CALLBACKS
*/
// Service Callbacks
CONST gattServiceCBs_t ccServiceCBs =
{
  ccService_ReadAttrCB,  // Read callback function pointer
  ccService_WriteAttrCB, // Write callback function pointer
  NULL                   // Authorization callback function pointer
};

/*********************************************************************
* PUBLIC FUNCTIONS
*/

/*********************************************************************
* @fn      CcService_addService
*
* @brief   Initializes the service by registering
*          GATT attributes with the GATT server.
*
* @return  Success or Failure
*/
bStatus_t CcService_addService(void)
{
  // Allocate Client Characteristic Configuration table
  ccDataConfig = (gattCharCfg_t *)ICall_malloc(sizeof(gattCharCfg_t) *
                                                linkDBNumConns);
  if (ccDataConfig == NULL)
  {
    return (bleMemAllocError);
  }
  
  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg(INVALID_CONNHANDLE, ccDataConfig);
  
  // Register GATT attribute list and CBs with GATT Server App
  return GATTServApp_RegisterService(ccServiceAttrTbl,
                                     GATT_NUM_ATTRS(ccServiceAttrTbl),
                                     GATT_MAX_ENCRYPT_KEY_SIZE,
                                     &ccServiceCBs);
}


/*********************************************************************
* @fn      CcService_registerAppCBs
*
* @brief   Registers the application callback function. Only call
*          this function once.
*
* @param   callbacks - pointer to application callbacks.
*
* @return  SUCCESS or bleAlreadyInRequestedMode
*/
bStatus_t CcService_registerAppCBs(ccCBs_t *appCallbacks)
{
  if (appCallbacks)
  {
    ccService_AppCBs = appCallbacks;

    return (SUCCESS);
  }
  else
  {
    return (bleAlreadyInRequestedMode);
  }
}


/*********************************************************************
* @fn      CcService_setParameter
*
* @brief   Set a Connection Control Profile parameter.
*
* @param   param - Profile parameter ID
* @param   len - length of data to write
* @param   value - pointer to data to write.  This is dependent on
*          the parameter ID and WILL be cast to the appropriate
*          data type (example: data type of uint16_t will be cast to
*          uint16_t pointer).
*
* @return  bStatus_t
*/
bStatus_t CcService_setParameter(uint8_t param, uint8_t len, void *value)
{
  bStatus_t ret = SUCCESS;

  switch (param)
  {
  case CCSERVICE_CHAR1:
    if (len == CCSERVICE_CHAR1_LEN )
    {
      memcpy(ccServiceChar1, value, CCSERVICE_CHAR1_LEN);

      // See if Notification has been enabled
      ret = GATTServApp_ProcessCharCfg( ccDataConfig, ccServiceChar1, FALSE,
                                 ccServiceAttrTbl, 
                                 GATT_NUM_ATTRS( ccServiceAttrTbl ),
                                 INVALID_TASK_ID, ccService_ReadAttrCB);
    }
    else
    {
      ret = bleInvalidRange;
    }
    break;

  case CCSERVICE_CHAR2:
    if (len == CCSERVICE_CHAR2_LEN )
    {
      // Should not write to this value other than startup
      memcpy(ccServiceChar2, value, CCSERVICE_CHAR2_LEN);
    }
    else
    {
      ret = bleInvalidRange;
    }
    break;

  case CCSERVICE_CHAR3:
    if (len == sizeof(uint8_t))
    {
      // Should not write to this value other than startup
      ccServiceChar3 = *((uint8_t*)value);
    }
    else
    {
      ret = bleInvalidRange;
    }
    break;

  default:
    ret = INVALIDPARAMETER;
    break;
  }

  return (ret);
}

/*********************************************************************
* @fn      CcService_getParameter
*
* @brief   Get a CC Profile parameter.
*
* @param   param - Profile parameter ID
* @param   value - pointer to data to put.  This is dependent on
*          the parameter ID and WILL be cast to the appropriate
*          data type (example: data type of uint16_t will be cast to
*          uint16_t pointer).
*
* @return  bStatus_t
*/
bStatus_t CcService_getParameter(uint8_t param, void *value)
{
  bStatus_t ret = SUCCESS;
  
  switch (param)
  {
  case CCSERVICE_CHAR1:
    memcpy(value, ccServiceChar1, CCSERVICE_CHAR1_LEN);
    break;

  case CCSERVICE_CHAR2:
    memcpy(value, ccServiceChar2, CCSERVICE_CHAR2_LEN);
    break;

  case CCSERVICE_CHAR3:
    *((uint8_t*)value) = ccServiceChar3;
    break;

  default:
    ret = INVALIDPARAMETER;
    break;
  }

  return (ret);
}

/*********************************************************************
 * @fn          ccService_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 * @param       method - type of read message 
 *
 * @return      SUCCESS, blePending or Failure
 */
static bStatus_t ccService_ReadAttrCB(uint16_t connHandle, 
                                      gattAttribute_t *pAttr,
                                      uint8_t *pValue, uint16_t *pLen, 
                                      uint16_t offset, uint16_t maxLen,
                                      uint8_t method)
{
  bStatus_t status = SUCCESS;
  uint16_t uuid;

  // If attribute permissions require authorization to read, return error
  if (gattPermitAuthorRead(pAttr->permissions))
  {
    // Insufficient authorization
    return (ATT_ERR_INSUFFICIENT_AUTHOR);
  }

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if (offset > 0)
  {
    return (ATT_ERR_ATTR_NOT_LONG);
  }

  if (utilExtractUuid16(pAttr,&uuid) == FAILURE)
  {
    // Invalid handle
    *pLen = 0;
    return ATT_ERR_INVALID_HANDLE;
  }

  switch (uuid)
  {
  case CCSERVICE_CHAR1_UUID:
    *pLen = CCSERVICE_CHAR1_LEN;
    memcpy(pValue, pAttr->pValue, CCSERVICE_CHAR1_LEN);
    break;
    
  default:
    *pLen = 0;
    status = ATT_ERR_ATTR_NOT_FOUND;
    break;
  }

  return (status);
}

/*********************************************************************
 * @fn      ccService_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   method - type of write message 
 *
 * @return  SUCCESS, blePending or Failure
*/
static bStatus_t ccService_WriteAttrCB(uint16_t connHandle, 
                                       gattAttribute_t *pAttr,
                                       uint8_t *pValue, uint16_t len, 
                                       uint16_t offset, uint8_t method)
{
  uint16_t uuid;
  bStatus_t status = SUCCESS;
  uint8_t notifyApp = 0xFF;

  // If attribute permissions require authorization to write, return error
  if (gattPermitAuthorWrite(pAttr->permissions))
  {
    // Insufficient authorization
    return (ATT_ERR_INSUFFICIENT_AUTHOR);
  }

  if (utilExtractUuid16(pAttr,&uuid) == FAILURE)
  {
    // Invalid handle
    return ATT_ERR_INVALID_HANDLE;
  }

  switch (uuid)
  {
  case CCSERVICE_CHAR2_UUID:

    // Validate the value
    // Make sure it's not a blob oper
    if (offset == 0)
    {
      if (len != CCSERVICE_CHAR2_LEN )
      {
        status = ATT_ERR_INVALID_VALUE_SIZE;
      }
    }
    else
    {
      status = ATT_ERR_ATTR_NOT_LONG;
    }

    // Write the value
    if (status == SUCCESS)
    {
      memcpy(ccServiceChar2, pValue, CCSERVICE_CHAR2_LEN);
      notifyApp = CCSERVICE_CHAR2;
    }

    break;
  case CCSERVICE_CHAR3_UUID:

    if (offset == 0)
    {
      if (len != 1) 
      {
        status = ATT_ERR_INVALID_VALUE_SIZE;
      }
    }
    else
    {
      status = ATT_ERR_ATTR_NOT_LONG;
    }

    // Write the value
    if (status == SUCCESS)
    {
      uint8_t *pCurValue = (uint8_t *)pAttr->pValue;
      *pCurValue = pValue[0];
      notifyApp = CCSERVICE_CHAR3;
    }

    break;

  case GATT_CLIENT_CHAR_CFG_UUID:
      status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                              offset, GATT_CLIENT_CFG_NOTIFY);
      break;

  default:
    status = ATT_ERR_ATTR_NOT_FOUND;
    break;
  }


  // If a characteristic value changed then callback function to notify application of change
  if ((notifyApp != 0xFF ) && ccService_AppCBs && ccService_AppCBs->pfnCcChange)
  {
    ccService_AppCBs->pfnCcChange(notifyApp);
  }

  return (status);
}

/*********************************************************************
*********************************************************************/
#endif
