/** ----------------------------------------------------------------------
* File      : $RCSfile: com_android_nfc_NativeNfcTag.cpp,v $
* Revision  : $Revision: 1.38.4.3 $
* Date      : $Date: 2012/10/09 10:31:42 $
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

static const jint STATUS_OK    = 0;
static const jint STATUS_ERROR = -1;
static const jint STATUS_REJECTED = 0x93;
static const jint STATUS_CODE_TARGET_LOST = 146;

/*
#define NFCSTATUS_SHUTDOWN                  (0x0091)
#define NFCSTATUS_TARGET_LOST               (0x0092)
#define NFCSTATUS_REJECTED                  (0x0093)
#define NFCSTATUS_TARGET_NOT_CONNECTED      (0x0094) 
#define NFCSTATUS_INVALID_HANDLE            (0x0095)
#define NFCSTATUS_ABORTED                   (0x0096)
#define NFCSTATUS_COMMAND_NOT_SUPPORTED     (0x0097)
#define NFCSTATUS_NON_NDEF_COMPLIANT        (0x0098)
#define NFCSTATUS_OK                        (0x0000)
*/
  
// external declaration:
DWORD getSupportedListenerMask (HANDLE hApp);
  
/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeNfcTag_doRead
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o
 *
 *
 * @return jbyteArray
 */
/*------------------------------------------------------------------*/
static jbyteArray com_android_nfc_NativeNfcTag_doRead (JNIEnv *e, jobject o)
{
  NFCSTATUS  status = nfcStatusFailed;
  jbyteArray jBuffer = NULL;
  PBYTE      pBuffer;
  WORD       length;

  CONCURRENCY_LOCK();

  jfieldID id = NativeTagFields.mConnectedHandle;

  if (e->GetIntField(o, id) != -1)
  {
    LOGV("NativeNfcTag_doRead() enter, (known NDEF size %d)",
         (int)gpService->pTagInfo->ndef.currentSize);
         

    // TODO later get from TAG INFO for ndef
    length  = 0xFFFF;
    pBuffer = aquireIoBuffer(length);

    if (pBuffer)
    {
      status = pNfcReadNdefMessage(gpService->hListener, pBuffer, &length);

      if (NFC_STATUS_SUCCESS(status))
      {
        jBuffer = e->NewByteArray(length);
        e->SetByteArrayRegion(jBuffer, 0, length, (jbyte *) pBuffer);
      }
      else
      {
        length  = 0;
        jBuffer = e->NewByteArray(0);
      }
    }

    LOGV("NativeNfcTag_doRead() exit, read length=%d", length);
  }
  else
  {
    LOG("NativeNfcTag_doRead() enter, handle=0. Call ignored");
  }

  gpService->lastError = mapNfcStatusToAndroid(status);

  CONCURRENCY_UNLOCK();
  return jBuffer;
} // com_android_nfc_NativeNfcTag_doRead

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_pServiceNfcTag_doWrite
 *
 * @brief Write NDEF message to given TAG
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o
 * @param jbyteArray buf
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NativeNfcTag_doWrite (JNIEnv *e, jobject o, jbyteArray buf)
{
  NFCSTATUS status;
  WORD      length;
  PBYTE     pvBuffer;
  
  (void) o; // suspress unused parameter warning

  // HANDLE  handle = nfc_jni_get_nfc_tag_handle(e, o);

  length   = (WORD) e->GetArrayLength(buf);
  pvBuffer = (PBYTE) e->GetByteArrayElements(buf, NULL);

  LOGV("NativeNfcTag_doWrite() enter, %d bytes", length);

  CONCURRENCY_LOCK();

  status = pNfcWriteNdefMessage(gpService->hListener, pvBuffer, length);

  CONCURRENCY_UNLOCK();

  e->ReleaseByteArrayElements(buf, (jbyte *) pvBuffer, JNI_ABORT);

  LOGV("NativeNfcTag_doWrite() exit, status %X", status);

  if (!NFC_STATUS_SUCCESS(status))
  {
    return FALSE;
  }

  return TRUE;
} // com_android_nfc_NativeNfcTag_doWrite


static void loc_dumpIntArray (const char * name, JNIEnv * e, jobject o, jfieldID id)
{
  jintArray array = (jintArray) e->GetObjectField (o, id);
  
  jsize n = e->GetArrayLength (array);
  
  jint * data = e->GetIntArrayElements(array, NULL);

  LOGV ("%s: Array size is %d", name, n);
  for (jsize i=0; i<n; i++)
  {
    LOGV ("\tn[%d] = %d\n",i, data[i]);
  }
  e->ReleaseIntArrayElements(array, data, 0);
  e->DeleteLocalRef(array);
}

/*------------------------------------------------------------------*/
/**
 * @fn    loc_dumpTag
 *
 * @brief output the parameters of a NativeNfc-Tag object.
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static void loc_dumpTag (JNIEnv * e, jobject o)
{
  loc_dumpIntArray ("mTechList",        e, o, NativeTagFields.mTechList);
  loc_dumpIntArray ("mTechHandles",     e, o, NativeTagFields.mTechHandles);
  loc_dumpIntArray ("mTechLibNfcTypes", e, o, NativeTagFields.mTechLibNfcTypes);
}

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_pServiceNfcTag_doConnect
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jint com_android_nfc_NativeNfcTag_doConnect (JNIEnv *e, jobject o, unsigned long handle)
{
  NFCSTATUS status;

  CONCURRENCY_LOCK();

  LOGV("NativeNfcTag_doConnect() handle %08x, tagInfo %p", (unsigned int)handle, gpService->pTagInfo);

  // TODO later, is handle always reported tagInfo pointer or can we switch here between different detected tags ???

  status = pNfcConnectTag(gpService->hListener, 0);

  if (status == nfcStatusSuccess)
  {
    // Success, set poll & act bytes
    nfcSetTargetPollBytes      (e, o, gpService, gpService->pTagInfo);
    nfcSetTargetActivationBytes(e, o, gpService, gpService->pTagInfo);
  } 
  else 
  {
    LOGE("NativeNfcTag_doConnect() failed with status %d", status);
  }
  
  loc_dumpTag (e,o);

  CONCURRENCY_UNLOCK();

  return (NFC_STATUS_SUCCESS(status)) ? STATUS_OK : STATUS_ERROR;
  
} // com_android_nfc_NativeNfcTag_doConnect

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_pServiceNfcTag_doDisconnect
 *
 * @brief Is here a call to restart the discovery.
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NativeNfcTag_doDisconnect (JNIEnv *e, jobject o)
{
  CONCURRENCY_LOCK();

  jfieldID id = NativeTagFields.mConnectedHandle;

  if (e->GetIntField(o, id) != -1)
  {
    NFCSTATUS status;
    
    LOGV("NativeNfcTag_doDisconnect() tagType %08x", (unsigned int)gpService->pTagInfo->tagType);

    if (gpService->pTagInfo->tagType)
    {
      gpService->lastError = mapNfcStatusToAndroid (pNfcDisconnectTag(gpService->hListener));
    }
    else
    {
      // Tag seems removed, so we restart polling loop. In our current implementation
      // we must stop the listener first and restart again.
      if (gpService->hListener != NULL)
      {
        pNfcUnregisterListener(gpService->hListener);
        gpService->hListener = 0;
      }
      
      // get listener support mask:
      DWORD support     = getSupportedListenerMask (gpService->hApplication);
      
      DWORD featureMask = (gpService->listenMask & support);
      
      if (featureMask != 0)
      {
        status = pNfcRegisterListener(gpService->hApplication, gpService,
                                      featureMask | NFC_REQ_TYPE_NDEF,
                                      NULL, &gpService->hListener);
      }
    }

    LOG("NativeNfcTag_doDisconnect() exit");
    CONCURRENCY_UNLOCK();
  } 
  else
  {
    LOG("NativeNfcTag_doDisconnect() handle 0, Call ignored");
    CONCURRENCY_UNLOCK();
  }
  return TRUE;
} // com_android_nfc_NativeNfcTag_doDisconnect

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_pServiceNfcTag_doTransceive
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o
 * @param jbyteArray jData
 *
 *
 * @return jbyteArray
 */
/*------------------------------------------------------------------*/
static jbyteArray com_android_nfc_NativeNfcTag_doTransceive (JNIEnv *e, jobject o, jbyteArray jData)
{
  WORD       length;
  PBYTE      pBuffer;
  WORD       returnedLength;
  PBYTE      pRxBuffer;
  jbyteArray jPduResponse = NULL;

  length = e->GetArrayLength(jData);

  (void) o; // suspress unused parameter warning  

  LOGV("NativeNfcTag_doTransceive() size %d", length);

  pBuffer = aquireIoBuffer(length);

  if (!pBuffer)
  {
    return NULL;
  }

  CONCURRENCY_LOCK();

  pRxBuffer = pBuffer + length + 16;

  e->GetByteArrayRegion(jData, 0, length, (jbyte *) pBuffer);
  
  // Fixup for MIFARE classic Authenticate PDUs
  if (gpService->pTagInfo->tagType & ( NFC_IND_TYPE_TAG_MIFARE_STD_1K |
                                       NFC_IND_TYPE_TAG_MIFARE_STD_4K))
  {
    // check if the PDU is a mifare authenticate:
    if ((length == 12) && ((pBuffer[0] == 0x60) || (pBuffer[0] == 0x61)))
    {
      // also check the UID:
      if (memcmp (gpService->pTagInfo->uid, pBuffer+2, 4) == 0)
      {
        LOG ("reformat mifare authentication for compatibility with libnfc");        
        // This is a mifare standard authenticate command. The stack 
        // uses a different order for these than the NXP stack. Fixup!:
        BYTE key[6], uid[4];
        memcpy (key, pBuffer+6, 6);
        memcpy (uid, pBuffer+2, 4);
        memcpy (pBuffer+2, key, 6);
        memcpy (pBuffer+8, uid, 4);
      } 
    } 
  } 
  
  returnedLength   = gpService->ioBufferSize - length - 16;
  NFCSTATUS status = pNfcExchangePdu(gpService->hListener,
                                     pBuffer,
                                     length,
                                     pRxBuffer,
                                     &returnedLength);

  if (NFC_STATUS_SUCCESS(status))
  {
    jPduResponse = e->NewByteArray(returnedLength);
    e->SetByteArrayRegion(jPduResponse, 0, returnedLength, (jbyte *) pRxBuffer);
    gpService->transactionCount++;
  }

  CONCURRENCY_UNLOCK();

  LOGV("NativeNfcTag_doTransceive() returns with %d bytes", returnedLength);

  return jPduResponse;
} // com_android_nfc_NativeNfcTag_doTransceive



/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_pServiceNfcTag_doCheckNdef
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/

static jint com_android_nfc_NativeNfcTag_doCheckNdef (JNIEnv *e, jobject o, jintArray ndefinfo)
{
  jboolean result = FALSE;

  (void) o; // suspress unused parameter warning  

  if (gpService->pTagInfo->tagType == 0)
  {
    /* This is very special!
     * 
     * The NFC service may call checkNdef even if the tag has already been lost. :-(
     * The framework code expects us to return the last known ndef value to be returned, not
     * an actual error-code.
     * 
     * That's what we're doing here.. (ISIS checks this!)
     */
    result = (gpService->previouslyDetectedTagType & NFC_REQ_TYPE_NDEF) ? STATUS_OK : STATUS_ERROR;
    LOGV("NativeNfcTag_doCheckNdef() tag lost - return data from last detected tag instead: %d", result);
    return result;
  }

  
  NFCSTATUS status = nfcStatusSuccess;

  if ((gpService->pTagInfo->tagType & NFC_REQ_TYPE_NDEF) == 0)
  {
    // Not an ndef-tag at detection time. This might have changed.
    // request a new ndef state now.
    
    CONCURRENCY_LOCK();
    if (gpService->transactionCount > 0)
    {
      status = pNfcGetTagInfo(gpService->hListener, NFC_REQ_TYPE_NDEF, &gpService->tagInfo);
    }
    CONCURRENCY_UNLOCK();
  }
  
  if ((status == nfcStatusSuccess) && (gpService->pTagInfo->tagType & NFC_REQ_TYPE_NDEF))
  {
    if (e->GetArrayLength(ndefinfo) != 2)
    {
      LOG("NativeNfcTag_doCheckNdef() return error (illegal array passed)");
      return STATUS_ERROR;
    }

    jint *ndef         = e->GetIntArrayElements(ndefinfo, 0);
    int   apiCardState = NDEF_MODE_UNKNOWN;

    if(ndef != NULL)
    {
      ndef[0] = gpService->pTagInfo->ndef.maxSize;

      if (gpService->pTagInfo->ndef.flags.isWritable)
      {
        apiCardState = NDEF_MODE_READ_WRITE;
        LOG("NDEF TAG is writable");
      }
      else if (gpService->pTagInfo->ndef.flags.isReadable)
      {
        apiCardState = NDEF_MODE_READ_ONLY;
        LOG("NDEF TAG is read only");
      }
      ndef[1] = apiCardState;
      e->ReleaseIntArrayElements(ndefinfo, ndef, 0);
      result  = TRUE;
    }
    else
      result = FALSE;
  }
 
  LOGV("NativeNfcTag_doCheckNdef() tag type %08x returns %s",
       (unsigned int)gpService->pTagInfo->tagType,
       result ? "TRUE" : "FALSE");

  return result ? STATUS_OK : STATUS_ERROR;
} // com_android_nfc_NativeNfcTag_doCheckNdef



/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_pServiceNfcTag_doPresenceCheck
 *
 * @brief Check TAG presence in the field
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NativeNfcTag_doPresenceCheck (JNIEnv *e, jobject o)
{
  NFCSTATUS status;

  jfieldID id = NativeTagFields.mConnectedHandle;

  if (e->GetIntField(o, id) != -1)
  {
    CONCURRENCY_LOCK();

    LOG("NativeNfcTag_doPresenceCheck()");

    // TODO for later, suppress all logging information (use cache feature) and
    // show only when presence check is failed. We don't need redundant informations here

    status = pNfcCheckTag(gpService->hListener);

    CONCURRENCY_UNLOCK();

    if (NFC_STATUS_SUCCESS(status))
    {
      LOG("NativeNfcTag_doPresenceCheck() TAG present");
      return TRUE;
    }

    // Don't signal TAG removed. Negative result is enough
    // TODO: Is this even true, elli?
    gpService->pTagInfo->tagType = 0;
    LOG("NativeNfcTag_doPresenceCheck() TAG removed");
    return FALSE;
  }
  else
  {
    LOG("NativeNfcTag_doPresenceCheck() handle=0, Call ignored");
    return FALSE;
  }
} // com_android_nfc_NativeNfcTag_doPresenceCheck



/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeNfcTag_doGetNdefType
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o
 * @param jint libnfcType
 * @param jint javaType
 *
 *
 * @return jint
 */
/*------------------------------------------------------------------*/
static jint com_android_nfc_NativeNfcTag_doGetNdefType (JNIEnv *e,
                                                        jobject o,
                                                        jint    libnfcType,
                                                        jint    javaType)
{
  jint ndefType = NDEF_UNKNOWN_TYPE;

  (void) o; // suspress unused parameter warning  
  (void) e; // suspress unused parameter warning  

  LOGV("NativeNfcTag_doGetNdefType 0x%X 0x%X", libnfcType, javaType);

  switch (libnfcType & NFC_IND_TYPE_TYPE_MASK)
  {
    case NFC_IND_TYPE_TAG_JEWEL_TYPE1:
      ndefType = NDEF_TYPE1_TAG;
      break;

    case NFC_IND_TYPE_TAG_ULTRALIGHT_TYPE2:
      ndefType = NDEF_TYPE2_TAG;
      break;

    case NFC_IND_TYPE_TAG_FELICA_TYPE3:
      ndefType = NDEF_TYPE3_TAG;
      break;

    case NFC_IND_TYPE_TAG_ISO14443A:
    case NFC_IND_TYPE_TAG_14443B:
      ndefType = NDEF_TYPE4_TAG;
      break;

    case NFC_IND_TYPE_TAG_MIFARE_STD_4K:
    case NFC_IND_TYPE_TAG_MIFARE_STD_1K:
      ndefType = NDEF_MIFARE_CLASSIC_TAG;
      break;

    default:
      break;
  } // switch

  LOGV("NativeNfcTag_doGetNdefType (libNfcType=0x%X javaType=0x%X) -> returns %X",
       libnfcType,
       javaType,
       ndefType);

  return ndefType;
} // com_android_nfc_NativeNfcTag_doGetNdefType

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeNfcTag_doNdefFormat
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o
 * @param jbyteArray key
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NativeNfcTag_doNdefFormat (JNIEnv *e, jobject o, jbyteArray key)
{
  NFCSTATUS status;
  jboolean  jResult = FALSE;

  (void) o; // suspress unused parameter warning  
  (void) e; // suspress unused parameter warning  
  (void) key; // suspress unused parameter warning  

  LOG("NativeNfcTag_doNdefFormat");

  CONCURRENCY_LOCK();

  status = pNfcFormatNdef(gpService->hListener, 0);
  if (status == nfcStatusSuccess)
  {
    pNfcGetTagInfo(gpService->hListener, 0, &gpService->tagInfo);
    jResult = TRUE;
  }

  CONCURRENCY_UNLOCK();

  LOGV("NativeNfcTag_doNdefFormat %s", jResult ? "SUCCESS" : "! FAILED");

  return jResult;
} // com_android_nfc_NativeNfcTag_doNdefFormat

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeNfcTag_doMakeReadonly
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NativeNfcTag_doMakeReadonly (JNIEnv *e, jobject o)
{
  (void) e; // suspress unused parameter warning  
  (void) o; // suspress unused parameter warning  
  
  NFCSTATUS status;
  LOG("NativeNfcTag_doMakeReadOnly");
  CONCURRENCY_LOCK();
  status = pNfcMakeReadOnly(gpService->hListener);
  CONCURRENCY_UNLOCK();
  return (status == nfcStatusSuccess) ? FALSE : TRUE;
} 

/*
 * JNI registration.
 */


static jint com_android_nfc_NativeNfcTag_doHandleReconnect(JNIEnv *e, jobject o, unsigned long handle)
{
  (void) e; // suspress unused parameter warning  
  (void) o; // suspress unused parameter warning  
  (void) handle; // suspress unused parameter warning  
  return STATUS_OK;
}

static jint com_android_nfc_NativeNfcTag_doReconnect (JNIEnv *e, jobject o)
{
  (void) e; // suspress unused parameter warning  
  (void) o; // suspress unused parameter warning  
  return STATUS_OK;
}
  
static const JNINativeMethod gMethods[] = {
  {"doConnect",          "(I)I",      (void *) com_android_nfc_NativeNfcTag_doConnect      },
  {"doDisconnect",       "()Z",       (void *) com_android_nfc_NativeNfcTag_doDisconnect   },
  {"doReconnect",        "()I",       (void *) com_android_nfc_NativeNfcTag_doReconnect    },
  {"doHandleReconnect",  "(I)I",      (void *) com_android_nfc_NativeNfcTag_doHandleReconnect},
  {"doTransceive",       "([BZ[I)[B", (void *) com_android_nfc_NativeNfcTag_doTransceive   },
  {"doGetNdefType",      "(II)I",     (void *) com_android_nfc_NativeNfcTag_doGetNdefType },
  {"doCheckNdef",        "([I)I",     (void *) com_android_nfc_NativeNfcTag_doCheckNdef    },
  {"doRead",             "()[B",      (void *) com_android_nfc_NativeNfcTag_doRead         },
  {"doWrite",            "([B)Z",     (void *) com_android_nfc_NativeNfcTag_doWrite        },
  {"doPresenceCheck",    "()Z",       (void *) com_android_nfc_NativeNfcTag_doPresenceCheck}, 
  {"doNdefFormat",       "([B)Z",     (void *) com_android_nfc_NativeNfcTag_doNdefFormat   },
  {"doMakeReadonly",     "()Z",       (void *) com_android_nfc_NativeNfcTag_doMakeReadonly },
};

static const JNIFields gFields[] = {
  {"mTechList",            "[I",  &NativeTagFields.mTechList           },
  {"mTechHandles",         "[I",  &NativeTagFields.mTechHandles        },
  {"mTechLibNfcTypes",     "[I",  &NativeTagFields.mTechLibNfcTypes    },
  {"mTechPollBytes",       "[[B", &NativeTagFields.mTechPollBytes      },
  {"mTechActBytes",        "[[B", &NativeTagFields.mTechActBytes       },
  {"mConnectedHandle",     "I",   &NativeTagFields.mConnectedHandle    },
  {"mIsPresent",           "I",   &NativeTagFields.mIsPresent          },
  {"mUid",                 "[B",  &NativeTagFields.mUid                },
};

// *INDENT-ON*

TNativeTagFields NativeTagFields;

/*------------------------------------------------------------------*/
/**
 * @fn     register_com_android_nfc_NativeNfcTag
 *
 * @brief  get method and field ids for NativeNFcTag class
 *
 * @return
 */
/*------------------------------------------------------------------*/
bool register_com_android_nfc_NativeNfcTag (JNIEnv *e)
{
  e->ExceptionClear();
  jclass cls = e->FindClass(NATIVE_TAG_CLASS);
  
  memset (&NativeTagFields, 0, sizeof (NativeTagFields));

  return JNIRegisterFields(e, cls, gFields, NELEM(gFields)) &&
     e->RegisterNatives(cls, gMethods, NELEM(gMethods)) == 0;
}

} // namespace android
