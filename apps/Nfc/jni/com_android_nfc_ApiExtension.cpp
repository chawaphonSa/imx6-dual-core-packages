/** ----------------------------------------------------------------------
* File      : $RCSfile: com_android_nfc_ApiExtension.cpp,v $
* Revision  : $Revision: 1.10 $
* Date      : $Date: 2012/06/26 12:22:52 $
* Author    : Nils Pipenbrinck
* Copyright (c)2011 Stollmann E+V GmbH
*              Mendelssohnstr. 15
*              22761 Hamburg
*              Phone: +49 40 89088-0
*
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* $log:$
*
----------------------------------------------------------------------*/
#include "com_android_nfc.h"
#include "com_android_nfc_ApiExtension.h"
#include "nfcInit.h"

namespace android
{
/* we work with static data until it works and later put the stuff into a context-structure */

#define MAX_AID_LENGTH  (19 * 255) /* maximum AID table length */

static UdsState mState;

/* buffer to receive incomming data */
static BYTE   inputBuffer[MAX_AID_LENGTH + 20];
static size_t inputBufferPos = 0;


/* ----------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------- */

static size_t loc_satAddUnsigned32 (size_t x, size_t y)
// helper to calculate the saturated sum of two size_t values:
{
  size_t bits = sizeof (size_t) * CHAR_BIT;
  
  uint32_t signmask = (~0)<<(bits-1);
  uint32_t t0 = (y ^ x) & signmask;
  uint32_t t1 = (y & x) & signmask;
  x &= ~signmask;
  y &= ~signmask;
  x += y;
  t1 |= t0 & x;
  t1 = (t1 << 1) - (t1 >> (bits-1));
  return (x ^ t0) | t1;
}



/* ----------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------- */

void Nfc_ExtensionApi_Init (void)
{
  mState = UDS_STATE_DISCONNECTED;
  inputBufferPos = 0;
}

/* ----------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------- */

static void loc_killClient (PNfcService pService)
{
  /* disconnect whatever is currently connected */
  pService->extendUdsServer->dropConnection();
  inputBufferPos = 0;
  mState = UDS_STATE_DISCONNECTED;
}

/* ----------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------- */

static void loc_sendAnswer (PNfcService pService, NFCSTATUS status, DWORD length, const BYTE *data)
{
  DWORD answerHeader[2];

  // ignore any payload if we go any errors:
  if (status != nfcStatusSuccess) length = 0;
  answerHeader[0] = status;
  answerHeader[1] = length;
  pService->extendUdsServer->sendToClient((const BYTE*)answerHeader, sizeof (answerHeader));

  if (length)
  {
    // send out data:
    pService->extendUdsServer->sendToClient(data, length);
  }
}

/* ----------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------- */

static void loc_onCommand (PNfcService pService, DWORD command, DWORD version, DWORD length, const BYTE *data)
{
  if (version != ((1<<16)|(0)))
  {
    LOG("ApiExtension version mismatch");
    loc_sendAnswer(pService, nfcStatusInternalError, 0, NULL);
    return;
  }
  
  switch (command)
  {
    case APIEX_CMD_SETAIDTABLE:
    {
      NFCSTATUS status;
      LOG("ApiExtension SET AID TABLE");

      CONCURRENCY_LOCK();
      
      status = pNfcSecureSetAidTable(pService->hApplication, (BYTE *) data, length);
      loc_sendAnswer(pService, status, 0, NULL);
      CONCURRENCY_UNLOCK();
    }
    break;
    
    case APIEX_CMD_GETAIDTABLE:
    {
      NFCSTATUS status;
      WORD aidBufferSize = MAX_AID_LENGTH;
      BYTE aidBuffer[MAX_AID_LENGTH];  /* roughly 5k, this can reside on stack */

      LOG("ApiExtension GET AID TABLE");
      
      CONCURRENCY_LOCK();
      
      if (length == 0)
      {
        status = pNfcSecureGetAidTable(pService->hApplication, aidBuffer, &aidBufferSize);
      }
      else
      {
        LOGE ("ApiExtension GET AID TABLE failed: got excess data %d\n", (int)length);
        status = nfcStatusInvalidParameter;
      }

      if (status != nfcStatusSuccess)
      {
        // if it failed don't send any data.
        aidBufferSize = 0;
      }

      loc_sendAnswer(pService, status, aidBufferSize, aidBuffer);
      CONCURRENCY_UNLOCK();
    }
    break;
    
    case APIEX_CMD_SETREADERGLOBALENABLE:
    {
      LOG("ApiExtension SET READER GLOBAL ENABLE");
      CONCURRENCY_LOCK();
      
      bool isEnabled = ((length ==1) && (data[0] != 0));
      
      nfcManagerSetReaderEnable(isEnabled);
      
      if (isEnabled)
      {
         if ((!pService->hListener) && (pService->discoveryIsEnabled))
         {
          LOG ("nfc reading is currently off, but discovery enabled: turn it on");
          nfcManagerStartStandardReader(pService);
         }
      }
      else
      {
        if (pService->hListener)
        {
          LOG ("nfc reading is currently on, turn it off");
          HANDLE listener     = pService->hListener;
          pService->hListener = 0;
          pNfcUnregisterListener(listener);
          CleanupLastTag(pService, AquireJniEnv(pService));                    
        }
      }
      
      loc_sendAnswer(pService, nfcStatusSuccess, 0, NULL);
      
      LOG("UdsServer - detach from vm\n");
      gpService->vm->DetachCurrentThread();
      
      CONCURRENCY_UNLOCK();
    } break;
    
    case APIEX_CMD_GETREADERGLOBALENABLE:
    {
      LOG("ApiExtension GET READER GLOBAL ENABLE");
      CONCURRENCY_LOCK();
      
      uint8_t isEnabled = nfcManagerGetReaderEnable();

      loc_sendAnswer(pService, nfcStatusSuccess, 1, &isEnabled);
      
      LOG("UdsServer - detach from vm\n");
      gpService->vm->DetachCurrentThread();
      
      CONCURRENCY_UNLOCK();
    } break;
    
    
    case APIEX_CMD_CONTROL_IP1_TARGET:
    {
      LOG("ApiExtension APIEX_CMD_DISABLE_IP1_TARGET");
      CONCURRENCY_LOCK();
      
      bool isEnabled = ((length ==1) && (data[0] != 0));
      
      nfcManagerSetIp1TargetEnable (isEnabled);
     
      if (pService->hListener)
      {
        HANDLE listener     = pService->hListener;
        pService->hListener = 0;
        pNfcUnregisterListener(listener);
        CleanupLastTag(pService, AquireJniEnv(pService));                    
        nfcManagerStartStandardReader(pService);
      }

      loc_sendAnswer(pService, nfcStatusSuccess, 0, NULL);
      
      LOG("UdsServer - detach from vm\n");
      gpService->vm->DetachCurrentThread();

      CONCURRENCY_UNLOCK();
    } break;

    case APIEX_CMD_CONTROL_WLFEAT:
    {
      LOG("ApiExtension APIEX_CMD_CONTROL_WLFEAT");
      CONCURRENCY_LOCK();
      
      bool isEnabled = ((length ==1) && (data[0] != 0));
      
      nfcManagerSetWlFeatEnable (isEnabled);
     
      loc_sendAnswer(pService, nfcStatusSuccess, 0, NULL);
      
      LOG("UdsServer - detach from vm\n");
      gpService->vm->DetachCurrentThread();

      CONCURRENCY_UNLOCK();
      
      LOG("enforcing re-start of NFC-stack for WLFEAT test-mode\n");
      
      abort();
    } break;


    case APIEX_CMD_GET_IP1TARGET_CONTROL:
    {
      LOG("ApiExtension APIEX_CMD_GET_IP1TARGET_CONTROL");
      CONCURRENCY_LOCK();
      
      uint8_t isEnabled = nfcManagerGetIp1TargetEnable();

      loc_sendAnswer(pService, nfcStatusSuccess, 1, &isEnabled);
      
      LOG("UdsServer - detach from vm\n");
      gpService->vm->DetachCurrentThread();
      
      CONCURRENCY_UNLOCK();
    } break;

    case APIEX_CMD_GET_WLFEAT_CONTROL:
    {
      LOG("ApiExtension APIEX_CMD_GET_WLFEAT_CONTROL");
      CONCURRENCY_LOCK();
      
      uint8_t isEnabled = nfcManagerGetWlFeatEnable();

      loc_sendAnswer(pService, nfcStatusSuccess, 1, &isEnabled);
      
      LOG("UdsServer - detach from vm\n");
      gpService->vm->DetachCurrentThread();
      
      CONCURRENCY_UNLOCK();
    } break;
    
    case APIEX_CMD_GET_SWHW_VERSION:
    {
      LOG("ApiExtension APIEX_CMD_GET_SWHW_VERSION");
      CONCURRENCY_LOCK();
      
      BYTE payload[8];
      TNfcCapabilities caps;
      NFCSTATUS        status;

      status = pNfcGetCapabilities(gpService->hApplication, &caps, sizeof(caps));

      if (status == nfcStatusSuccess)
      {
        memcpy (payload+0, caps.controllerCaps[0]->swVersion, 4);
        memcpy (payload+4, caps.controllerCaps[0]->hwVersion, 4);
        loc_sendAnswer(pService, nfcStatusSuccess, 8, payload);
      }
      else
      {
        loc_sendAnswer(pService, status, 0, NULL);
      }
      
      LOG("UdsServer - detach from vm\n");
      gpService->vm->DetachCurrentThread();
      
      CONCURRENCY_UNLOCK();
    } break;
    
    default:
      LOGE("ApiExtension command %d is not known", (int)command);
      loc_killClient(pService);
      break;
  } /* switch */
}   /* loc_onCommand */

/* ----------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------- */


void Nfc_ExtensionApi_callback (void *context, UdsEvent event, void *data, size_t length)
{
  PNfcService pService = (PNfcService) context;

  switch (event)
  {
    case udsConnectIndication:
    {
      inputBufferPos = 0;
      memset (inputBuffer, 0xaa, sizeof (inputBuffer));
      
      if (pService->debugUdsServerState != UDS_STATE_DISCONNECTED)
      {
        loc_killClient(pService);
        break;
      }
      pService->debugUdsServerState = UDS_STATE_CONNECTED;
    }
    break;

    case udsDataIndication:
    {
      if (pService->debugUdsServerState == UDS_STATE_CONNECTED)
      {
        /* check absence of integer-overflow */
        if ((inputBufferPos + length) != loc_satAddUnsigned32(inputBufferPos, length))
        {
          /* integer overflow occured */
          LOG("! ApiExtension buffer overflow detected. kill client");
          loc_killClient(pService);
          
          /* done here */
          break;
        }
        
        memcpy (&inputBuffer[inputBufferPos], data, length);
        inputBufferPos += length;
        
        /* check if we got all the data */
        if (inputBufferPos >= 3*sizeof(DWORD))
        {
          DWORD command, version, dataLength;
          memcpy (&dataLength, inputBuffer + 2*sizeof(DWORD), sizeof(DWORD));
          
          if (dataLength == (inputBufferPos-(3*sizeof(DWORD))))
          {
            /* got all the data for the command */
            memcpy (&command, inputBuffer + 0*sizeof(DWORD), sizeof(DWORD));
            memcpy (&version, inputBuffer + 1*sizeof(DWORD), sizeof(DWORD));
            loc_onCommand(pService, command, version, dataLength, inputBuffer+3*sizeof(DWORD));
            inputBufferPos = 0;
          }
        }
      }
    } break;

    case udsDisconnectIndication:
    {
      pService->debugUdsServerState = UDS_STATE_DISCONNECTED;
      inputBufferPos = 0;
      break;
    }
  } /* switch */
}   /* Nfc_ExtensionApi_callback */
}   /* namespace */
