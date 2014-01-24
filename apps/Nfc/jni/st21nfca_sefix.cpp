/** ----------------------------------------------------------------------
* File      : $RCSfile: st21nfca_sefix.cpp,v $
* Revision  : $Revision: 1.4.2.1 $
* Date      : $Date: 2012/09/07 16:40:16 $
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

#include <stdint.h>
#include "com_android_nfc.h"
#include "nfcInit.h"

namespace android {


// structure to hold SE information 
struct seInfo
{
  int        numSE;
  TNfcInfoSE se[MAX_SE_COUNT];
};


// dummy callback handler, does nothing..
static void NFCAPI nfcDummyHandler ( PVOID pvAppContext, 
                                     NFCEVENT dwEvent, 
                                     PVOID pvListenerContext, 
                                     PNfcEventParam pEvent)
{
  (void) pvAppContext;
  (void) dwEvent;
  (void) pvListenerContext;
  (void) pEvent;
}

// dummy SE callback handler, just copies data int seInfo structure
static void APIENTRY seCountCb (PVOID pvContext, PNfcInfoSE pInfoSE)
{
  seInfo & info = *(seInfo *) pvContext;
  memcpy (&info.se[info.numSE], pInfoSE, sizeof (TNfcInfoSE));
  info.numSE++;
}

static uint32_t blacklist = 0;


BOOL fix_IsUiccOnBlacklist (int id)
{
  return (blacklist & (1<<id)) != 0;
}

/*------------------------------------------------------------------*/
/**
 * @fn    loc_GetUiccActivationFlags
 *
 * @brief try to activate all UICC and remember the success-code in global
 *        variables.
 *
 */
/*------------------------------------------------------------------*/
BOOL fix_GetUiccActivationFlags(char * initString, DWORD oemOptions)
{
  LOG("oneshot stack activation to get UICC enable flags");
  BOOL result = FALSE;
  HANDLE hApplication;
  BOOL uiccActivationFailed = FALSE;
  
  // all are blacklisted by default:
  blacklist = 0xffffffff;
  
  do 
  {
    NFCSTATUS status;
    
    status = pNfcInitialize(initString);
    if (status != nfcStatusSuccess)
    {
      LOGE("! nfcInitialize failed with %X", status);
      break;
    }

    status = pNfcCreate(NULL, nfcDummyHandler, &hApplication);
    if (status != nfcStatusSuccess)
    {
      LOGE("! nfcCreate failed with %X", status);
      pNfcDone();
      break;
    }
    
    TNfcCapabilities caps;
    status = pNfcGetCapabilities(hApplication, &caps, sizeof(caps));
    if (status != nfcStatusSuccess)
    {
      // oops.. 
      LOGE("! nfcGetCapabilities failed with %X", status);
      pNfcDone();
      break;
    }
    
    BOOL isSt21 = (strcmp((CHAR *) caps.controllerCaps[0]->vendorName, "STMICROELECTRONICS") == 0);

    if ((isSt21) && (oemOptions & QUIRK_ST21_BLACKLIST_FIX))
    {
      seInfo info;
      
      memset (&info, 0, sizeof (info));
      
      status = pNfcSecureGetList(hApplication, &info, seCountCb);
      if (status == nfcStatusSuccess)
      {
        /* assume all SE are working */
        blacklist = 0;
        
        LOGV("found %d secure elements", info.numSE);
        for (int i=0; i<info.numSE; i++)
        {
          if (pNfcSecureSetMode(hApplication, info.se[i].hSecure, seModeVirtual) != nfcStatusSuccess)
          {
            blacklist |= (1<<i);
            LOGV ("SE%d activation failed, blacklisted for further use\n", i);
            
            /* remember that at least one UICC activation has failed */
            if (info.se[i].caps & seCapsUICC)
            {
              uiccActivationFailed = TRUE;
            }
          }
          pNfcSecureSetMode(hApplication, info.se[i].hSecure, seModeOff);
        }
      }
      else
      {
        LOG("! getting secure list failed. Assume no SE\n");
        blacklist = 0xffffffff;
      }
    }
    else 
    {
      blacklist = 0;
    }
    
    // control UICC activation in battery low mode:
    if (uiccActivationFailed)
    {
      WORD outLength = 0;
      LOG("Disable UICC in battery low mode.");
      pNfcDeviceIoControl(hApplication, NFC_IO_SWP_MODE_OFF, NULL, 0, NULL, &outLength);      
    } 
    else 
    {
      WORD outLength = 0;
      
      if (oemOptions & QUIRK_UICC_ENABLE_LOWBAT)
      {
        LOG("Enable UICC in battery low mode.");
        pNfcDeviceIoControl(hApplication, NFC_IO_SWP_MODE_ON, NULL, 0, NULL, &outLength);      
      }
      else
      {
        LOG("Disable UICC in battery low mode.");
        pNfcDeviceIoControl(hApplication, NFC_IO_SWP_MODE_OFF, NULL, 0, NULL, &outLength);      
      }
    }

    status = pNfcDestroy(hApplication);
    if (status != nfcStatusSuccess)
    {
      LOGE("! nfcDestroy failed with %X", status);
      break;
    }
    
    status = pNfcDone();
    if (status != nfcStatusSuccess)
    {
      LOGE("! nfcDone failed with %X", status);
      break;
    }
    
    result = TRUE;
  } while (0);

  LOGV("oneshot stack activation returned with status %d, blacklist is %08x", result, blacklist);
  return result;
}

}

