/** ----------------------------------------------------------------------
* File      : $RCSfile: com_android_nfc_NativeNfcSecureElement.cpp,v $
* Revision  : $Revision: 1.11 $
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
	
const  int SecureElementTech=TARGET_TYPE_ISO14443_4;
	
static jint com_android_nfc_NativeNfcSecureElement_doOpenSecureElementConnection (JNIEnv *e,
                                                                                  jobject o)
{
  NFCSTATUS status = nfcStatusFailed;
  
  // suspress unused parameter warning
  (void) e;
  (void) o;

  LOG("NativeNfcSecureElement_doOpenSecureElementConnection() enter");

  for (int i=0; i<gpService->seCount; i++)
  {
    TNfcInfoSE * se = &gpService->seInfo[i];
    
    if (se->caps & (seCapsSmartMX |  seCapsESE |  seCapsSD))
    {
     if (gpService->isPN544)
     {
       LOG("workaround for bug: don't open secure connection for smartMX on PN544");
       continue;
     }
      
     gpService->hSEConnection = 0;	  
     status = pNfcSecureOpen(gpService->hApplication, se->hSecure , &gpService->hSEConnection);
     LOGV("NativeNfcSecureElement_doOpenSecureElementConnection() result %p",gpService->hSEConnection);
     
     break;
    }
  }
  
  if (status != nfcStatusSuccess) 
  {
	  gpService->hSEConnection = 0;
  }

  return (jint)(gpService->hSEConnection);
}

static jboolean com_android_nfc_NativeNfcSecureElement_doDisconnect (JNIEnv *e,
                                                                     jobject o,
                                                                     jint    handle)
{
  jboolean result=FALSE;
  HANDLE h = (HANDLE) handle;
  
  // suspress unused parameter warning
  (void) e;
  (void) o;
  
  
  LOGV("NativeNfcSecureElement_doDisconnect(%08x) enter",handle);
  result =  (nfcStatusSuccess == pNfcSecureClose(h) );
  LOGV("NativeNfcSecureElement_doDisconnect() exit (%d)",result);
  
  return result;
}

static jbyteArray com_android_nfc_NativeNfcSecureElement_doTransceive (JNIEnv    *e,
                                                                       jobject    o,
                                                                       jint       handle,
                                                                       jbyteArray data)
{
	uint8_t *buf;
	uint32_t buflen;
	uint16_t bytesRead;
	static uint8_t readBuf[1024];
	jbyteArray result = NULL;
	HANDLE h = (HANDLE) handle;

  // suspress unused parameter warning
  (void) o;

  CONCURRENCY_LOCK();	
	LOG("NativeNfcSecureElement_doTransceive() enter");
	
	/* access java bytearray */
	buf    = (uint8_t *) e->GetByteArrayElements(data, NULL);
	buflen = (uint32_t ) e->GetArrayLength(data);
	
	bytesRead = 1024;
	pNfcSecureExchangePdu( h, buf, buflen, readBuf, &bytesRead );
	
	/* Copy results back to Java */
   result = e->NewByteArray(bytesRead);
   if(result != NULL)
   {
      e->SetByteArrayRegion(result, 0,(jint) bytesRead,(jbyte *)readBuf);
   }

  /* release java bytearray */
   e->ReleaseByteArrayElements(data,
      (jbyte *)buf, JNI_ABORT);
  
  CONCURRENCY_UNLOCK();

  LOGV("NativeNfcSecureElement_doTransceive returns with %d bytes", bytesRead);

  return result;
}

static jbyteArray com_android_nfc_NativeNfcSecureElement_doGetUid (JNIEnv *e,
                                                                   jobject o,
                                                                   jint    handle)
{
  LOG("NativeNfcSecureElement_doGetUid. Not supported. Return: NULL");
  
  // suspress unused parameter warning
  (void) e;
  (void) o;
  (void) handle;
   
  return NULL;
}

static jintArray com_android_nfc_NativeNfcSecureElement_doGetTechList (JNIEnv *e,
                                                                       jobject o,
                                                                       jint    handle)
{
   jintArray techList;
   LOG("Get Secure element Type function returns fixed ISO-14443_4 ");
   
   (void) o;  // suspress unused parameter warning
      
   if(handle != 0)
   {
      techList = e->NewIntArray(1);
      e->SetIntArrayRegion(techList, 0, 1, &SecureElementTech);
      return techList;
   }
   else
   {
      return NULL;
   }
   
}

/*
 * JNI registration.
 */

static JNINativeMethod gMethods[] = {
  {"doOpenSecureElementConnection", "()I",
   (void *) com_android_nfc_NativeNfcSecureElement_doOpenSecureElementConnection},
  {"doDisconnect",                  "(I)Z",
   (void *) com_android_nfc_NativeNfcSecureElement_doDisconnect                 },
  {"doTransceive",                  "(I[B)[B",
   (void *) com_android_nfc_NativeNfcSecureElement_doTransceive                 },
  {"doGetUid",                      "(I)[B",
   (void *) com_android_nfc_NativeNfcSecureElement_doGetUid                     },
  {"doGetTechList",                 "(I)[I",
   (void *) com_android_nfc_NativeNfcSecureElement_doGetTechList                },
};


bool register_com_android_nfc_NativeNfcSecureElement (JNIEnv *e)
{
  return jniRegisterNativeMethods(e, NATIVE_SE_CLASS, gMethods, NELEM(gMethods));
}
} // namespace android
