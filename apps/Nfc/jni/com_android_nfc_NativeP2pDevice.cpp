/** ----------------------------------------------------------------------
* File      : $RCSfile: com_android_nfc_NativeP2pDevice.cpp,v $
* Revision  : $Revision: 1.8 $
* Date      : $Date: 2012/03/30 14:13:53 $
* Author    : Mathias Ellinger
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
#include "nfcInit.h"

namespace android {
/*------------------------------------------------------------------*/
/**
 * @fn     com_android_nfc_NativeP2pDevice_doConnect
 *
 * @brief  dummy entry point
 *
 * @return
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NativeP2pDevice_doConnect (JNIEnv *e, jobject o)
{
  LOG("NativeP2pDevice_doConnect() not supported");
  (void) e; // suspress unused parameter warning  
  (void) o; // suspress unused parameter warning  

  return TRUE;
}

/*------------------------------------------------------------------*/
/**
 * @fn     com_android_nfc_NativeP2pDevice_doDisconnect
 *
 * @brief  dummy entry point
 *
 * @return
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NativeP2pDevice_doDisconnect (JNIEnv *e, jobject o)
{
  LOG("NativeP2pDevice_doDisconnect() ! not supported");
  (void) e; // suspress unused parameter warning  
  (void) o; // suspress unused parameter warning  
  return TRUE;
}

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeP2pDevice_doTransceive
 *
 * @brief dummy entry point
 *
 * @return
 */
/*------------------------------------------------------------------*/
static jbyteArray com_android_nfc_NativeP2pDevice_doTransceive (JNIEnv    *e,
                                                                jobject    o,
                                                                jbyteArray data)
{
  LOG("NativeP2pDevice_doTransceive() ! not supported");
  (void) e; // suspress unused parameter warning  
  (void) o; // suspress unused parameter warning  
  (void) data; // suspress unused parameter warning  

  return NULL;
}

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeP2pDevice_doReceive
 *
 * @brief dummy entry point
 *
 * @return
 */
/*------------------------------------------------------------------*/
static jbyteArray com_android_nfc_NativeP2pDevice_doReceive (JNIEnv *e, jobject o)
{
  LOG("NativeP2pDevice_doReceive() ! not supported");
  (void) e; // suspress unused parameter warning  
  (void) o; // suspress unused parameter warning  

  return NULL;
}

/*------------------------------------------------------------------*/
/**
 * @fn     com_android_nfc_NativeP2pDevice_doSend
 *
 * @brief dummy entry point
 *
 * @return
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NativeP2pDevice_doSend (JNIEnv *e, jobject o, jbyteArray buf)
{
  LOG("NativeP2pDevice_doSend() ! not supported");
  (void) e; // suspress unused parameter warning  
  (void) o; // suspress unused parameter warning  
  (void) buf; // suspress unused parameter warning  

  return FALSE;
}

/*
 * JNI registration.
 */
TNativeP2PDeviceFields NativeP2PDeviceFields;

static const JNINativeMethod gMethods[] = {
  {"doConnect",    "()Z",    (void *) com_android_nfc_NativeP2pDevice_doConnect   },
  {"doDisconnect", "()Z",    (void *) com_android_nfc_NativeP2pDevice_doDisconnect},
  {"doTransceive", "([B)[B", (void *) com_android_nfc_NativeP2pDevice_doTransceive},
  {"doReceive",    "()[B",   (void *) com_android_nfc_NativeP2pDevice_doReceive   },
  {"doSend",       "([B)Z",  (void *) com_android_nfc_NativeP2pDevice_doSend      },
};

static const JNIFields gFields[] = {
  {"mHandle",       "I",  &NativeP2PDeviceFields.mHandle      },
  {"mMode",         "I",  &NativeP2PDeviceFields.mMode        },
  {"mGeneralBytes", "[B", &NativeP2PDeviceFields.mGeneralBytes},
};

/*------------------------------------------------------------------*/
/**
 * @fn     register_com_android_nfc_NativeP2pDevice
 *
 * @brief  get method and field ids for NativeP2PDevice class
 *
 * @return
 */
/*------------------------------------------------------------------*/
bool register_com_android_nfc_NativeP2pDevice (JNIEnv *e)
{
  jclass cls = e->FindClass(NATIVE_P2P_CLASS);

  return JNIRegisterFields(e, cls, gFields, NELEM(gFields)) &&
         e->RegisterNatives(cls, gMethods, NELEM(gMethods)) == 0;
}
}
