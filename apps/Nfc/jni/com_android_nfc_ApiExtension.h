/** ----------------------------------------------------------------------
* File      : $RCSfile: com_android_nfc_ApiExtension.h,v $
* Revision  : $Revision: 1.7 $
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
#ifndef __COM_ANDROID_NFC_APIEXTENSION_H__
#define __COM_ANDROID_NFC_APIEXTENSION_H__

namespace android 
{
  
// Commands understood by the ApiExtension:
#define APIEX_CMD_SETAIDTABLE            1  
#define APIEX_CMD_GETAIDTABLE            2

#define APIEX_CMD_SETREADERGLOBALENABLE  3
#define APIEX_CMD_GETREADERGLOBALENABLE  4

#define APIEX_CMD_GET_SWHW_VERSION       5
    
// unofficial test-modes         
#define APIEX_CMD_CONTROL_IP1_TARGET     0x10006
#define APIEX_CMD_GET_IP1TARGET_CONTROL  0x10007

#define APIEX_CMD_CONTROL_WLFEAT         0x10008
#define APIEX_CMD_GET_WLFEAT_CONTROL     0x10009


/* ---------------------------------------------------------------------
 * General format for commands:
 * 
 * 
 * CMD      32 bit LE   -> Command, one of APIEX_CMD_XXXXXX
 * LEN      32 bit LE   -> payload following in bytes
 * PAYLOAD  n           -> actual payload, format depends on command 
 * 
 * --------------------------------------------------------------------- */
 
 
/* ---------------------------------------------------------------------
 * General format for answers:
 * 
 * 
 * STATUS   32 bit LE   -> status from libstmnfc.so (nfcStatusXXXX)
 * LEN      32 bit LE   -> length of payload to follow
 * PAYLOAD  n           -> actual payload, format depends on command 
 * 
 * --------------------------------------------------------------------- */


void Nfc_ExtensionApi_callback (void *context, UdsEvent event, void *data, size_t length);

void Nfc_ExtensionApi_Init (void);

}
#endif


