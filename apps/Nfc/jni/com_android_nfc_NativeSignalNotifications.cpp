/** ----------------------------------------------------------------------
* File      : $RCSfile: com_android_nfc_NativeSignalNotifications.cpp,v $
* Revision  : $Revision: 1.39.2.1 $
* Date      : $Date: 2012/07/12 14:33:19 $
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
static jmethodID nfcManagerNotifyTagDetected;
static jmethodID nfcManagerNotifyTransaction;
static jmethodID nfcManagerNotifyLlcpLinkActivation;
static jmethodID nfcManagerNotifyLlcpLinkDeactivated;
static jmethodID nfcManagerNotifyTargetRemoved;
static jmethodID nfcManagerGetReaderEnablePersistant;
static jmethodID nfcManagerSetReaderEnablePersistant;

static jmethodID nfcManagerGetIp1TargetEnablePersistant;
static jmethodID nfcManagerSetIp1TargetEnablePersistant;

static jmethodID nfcManagerGetWlFeatEnablePersistant;
static jmethodID nfcManagerSetWlFeatEnablePersistant;

static jmethodID nfcManagerNotifyFieldOn;
static jmethodID nfcManagerNotifyFieldOff;


/*
 * A word about how the syncronization of this notification thread works:
 *
 * Messages are sent to the thread via the nfcSignalNotification function.
 *
 * - This function is protected by the mutex hSendGuard to make sure only one thread
 *   can enter nfcSignalNotification at a time.
 *
 * - Data is gets written into shared memory.
 *
 * - The thread gets released via hSignalSemaphore
 *
 * - The send-function waits until the thread has finished work and signalled hWaitSemaphore
 *
 * This makes sure that all events are executed in order. Sending threads are blocked until
 * all processing has been done, It is okay to pass stack-pointers to the notification
 * thread.
 */

void *nfcNotificationThread (void *ptr)
{
  PNfcService      pService = (PNfcService) ptr;
  JavaVMAttachArgs args;
  JNIEnv          *env;
  BOOL             exitSignalled = false;

  memset(&args, 0, sizeof(args));

  args.name    = "NFC Message Loop";
  args.version = pService->env_version;
  args.group   = NULL;

  pService->vm->AttachCurrentThread(&env, &args);

  LOG("[Enter native service message loop]");

  while (!exitSignalled)
  {
    // Wait until signaled
    sem_wait(&pService->hSignalSemaphore);

    LOGV("Notification Service loop, dispatch event %X", pService->threadShared.event);

    switch (pService->threadShared.event)
    {
      case nfcNotificationTagInsert:
        LOG("[worker thread: nfcSignalTagDetected]");
        nfcSignalTagDetected(pService);
        break;

      case nfcNotificationTagRemove:
        LOG("[worker thread: nfcSignalTargetRemoved]");
        nfcSignalTargetRemoved(pService);
        break;

      case nfcNotificationTransaction:
        LOG("[worker thread: nfcSignalTransaction]");
        nfcSignalTransaction(pService, (PNfcTransactionInd) pService->threadShared.param);
        break;

      case nfcNotificationP2PActivation:
        LOG("[worker thread: nfcNotificationP2PActivation]");
        nfcSignalP2pActivation(pService);
        break;

      case nfcNotificationQuit:
        LOG("[Native service loop break]");
        exitSignalled = TRUE;
        break;
        
      case nfcNotificationFieldOn:
        LOG("[worker thread: nfcNotificationFieldOn]");
        env->CallVoidMethod(pService->objectManager, nfcManagerNotifyFieldOn);
        break;
        
      case nfcNotificationFieldOff:
        LOG("[worker thread: nfcNotificationFieldOff]");
        env->CallVoidMethod(pService->objectManager, nfcManagerNotifyFieldOff);
        break;
        

      default:
        LOGV("[native service notification illegal command: %d]\n",
             (int) pService->threadShared.event);
        break;
    } // switch

    // unblock sender function:
    sem_post(&pService->hWaitSemaphore);
  }

  LOG("[worker thread detaches from VM]");

  pService->vm->DetachCurrentThread();

  LOG("[exit native service notification loop]");

  sem_post(&pService->hWaitSemaphore);
  return 0;
} // nfcNotificationThread

/*------------------------------------------------------------------*/
/**
 * @fn     nfcSignalNotification
 *
 * @brief
 *
 * @return
 */
/*------------------------------------------------------------------*/
void nfcSignalNotification (PNfcService pService, TNfcNotification event, PVOID pvParam, int size)
{
  if (pthread_mutex_lock(&pService->hSendGuard) != 0)
  {
    LOGE("pthead_mutex lock failed %d", errno);
  }

  // copy event to shared memory:
  pService->threadShared.event = event;
  pService->threadShared.param = pvParam;
  pService->threadShared.size  = size;

  // Signal worker thread for processing
  if (sem_post(&pService->hSignalSemaphore) != 0)
  {
    LOGE("sem_post failed %d", errno);
  }

  // wait until worker-thread has finished processing.
  if (sem_wait(&pService->hWaitSemaphore) != 0)
  {
    LOGE("sem_wait failed %d", errno);
  }

  if (pthread_mutex_unlock(&pService->hSendGuard) != 0)
  {
    LOGE("pthead_mutex unlock failed %d", errno);
  }
} // nfcSignalNotification

/*------------------------------------------------------------------*/
/**
 * @fn     AquireJniEnv
 *
 * @brief  Get Java environment pointer from service..
 *
 * @return
 */
/*------------------------------------------------------------------*/
JNIEnv *AquireJniEnv (PNfcService pService)
{
  JNIEnv *env = 0;

  if (pService->vm)
  {
    pService->vm->AttachCurrentThread(&env, 0);
  }

  pService->vm->GetEnv((void **) &env, pService->env_version);

  if (env == 0)
  {
    LOG("FATAL: Unable to get a valid JNI-Environment!");
  }

  return env;
} // AquireJniEnv

/*------------------------------------------------------------------*/
/**
 * @fn    nfcSignalTagRemoved
 *
 * @brief Signal TAG removed to upper JAVA NFC service layer
 *
 * @param PNfcService pService - Pointer to our JNI service instance
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void nfcSignalTargetRemoved (PNfcService pService)
{
  JNIEnv *env = AquireJniEnv(pService);

  if (pService->pTagInfo->tagType & (NFC_IND_TYPE_P2P_MASK))
  {
    LOG("Signal LLCP Link is deactivated");
    env->CallVoidMethod(pService->objectManager,
                        nfcManagerNotifyLlcpLinkDeactivated,
                        pService->objectP2pDevice);
  }
  else
  {
    LOG("nfcSignalTagRemoved");
    
    if (pService->lastSignaledTag)
    {
      // make sure the Java world knows that the device has been removed.
      env->SetIntField(pService->lastSignaledTag, NativeTagFields.mIsPresent, 0);
      env->SetIntField(pService->lastSignaledTag, NativeTagFields.mConnectedHandle, -1);
    }

    env->CallVoidMethod(pService->objectManager, nfcManagerNotifyTargetRemoved);
  }

  pService->pTagInfo->tagType = 0;

  if (env->ExceptionCheck())
  {
    LOG("Exception occured");
  }
} // nfcSignalTargetRemoved

/*------------------------------------------------------------------*/
/**
 * @fn    nfcSignalP2pActivation
 *
 * @brief Signal P2P instance activation/deactivation. We do this
 *        every time when a P2P link is detected or removed.
 *
 * @param PNfcService pService - Pointer to our JNI service instance
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void nfcSignalP2pActivation (PNfcService pService)
{
  JNIEnv *env = AquireJniEnv(pService);
  jobject p2pDevice;

  LOGV("nfcSignalP2pActivation() type %.08X, handle=%p",
       (unsigned int)pService->pTagInfo->tagType,
       pService->hListener);

  p2pDevice = jniCreateObjectReference(env, pService->objectP2pDevice);

  if (!p2pDevice)
  {
    LOG("! Unable to create p2pDevice");
    return;
  }

  if (pService->pTagInfo->tagType == NFC_IND_TYPE_P2P_INITIATOR)
  {
    LOGV("Discovered P2P Initiator, general bytes length %d",
         pService->pTagInfo->p.p2p.historicalBytesLength);

    env->SetIntField(p2pDevice, NativeP2PDeviceFields.mMode, (jint) MODE_P2P_INITIATOR);

    //
    // Set General Bytes if any
    //

    // TODO
    #if 0
    jbyteArray generalBytes = NULL;
    jniSetByteArray(env,
                    p2pDevice,
                    NativeP2PDeviceFields.mGeneralBytes
                    pService->pTagInfo->p.p2p.historicalBytes,
                    pService->pTagInfo->p.p2p.historicalBytesLength
                    );
    #endif
  }
  else
  {
    LOG("Discovered P2P Target");
    env->SetIntField(p2pDevice, NativeP2PDeviceFields.mMode, (jint) MODE_P2P_TARGET);
  }

  env->SetIntField(p2pDevice, NativeP2PDeviceFields.mHandle, (jint) pService->hListener);

  CleanupLastTag(pService, env);
  pService->lastSignaledP2PDevice = p2pDevice;

  //
  // Notify the service that new a P2P device was found
  //

  LOGV("Notify NFC Service LLCP link activation %p", nfcManagerNotifyLlcpLinkActivation);

  env->CallVoidMethod(pService->objectManager, nfcManagerNotifyLlcpLinkActivation, p2pDevice);

  if (env->ExceptionCheck())
  {
    LOG("! Exception occured");
  }
} // nfcSignalP2pActivation

/*------------------------------------------------------------------*/
/**
 * @fn    nfcSignalTagDetected
 *
 * @brief Signal TAG detected to NFC JAVA service layer
 *
 * @param PNfcService pService - Pointer to our JNI service instance
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void nfcSignalTagDetected (PNfcService pService)
{
  JNIEnv *env = AquireJniEnv(pService);
  jobject tag;

#define MAX_NUM_TECHNOLOGIES  8
  int technologies[MAX_NUM_TECHNOLOGIES];
  int handles[MAX_NUM_TECHNOLOGIES];
  int libnfctypes[MAX_NUM_TECHNOLOGIES];
  int count;

  pService->tagDetectedCount++;

  LOGV("nfcSignalTagDetected #%d, pService=%p, vm=%p",
       pService->tagDetectedCount,
       pService,
       pService->vm);

  tag = jniCreateObjectReference(env, pService->objectNfcTag);

  if (!tag)
  {
    return;
  }

  if ( NativeTagFields.mConnectedHandle != NULL)
  {
    // initialize class connection:
    env->SetIntField(tag, NativeTagFields.mConnectedHandle, -1);    
  }

  //
  // !!! Target and Tag specific Hacks and Workarounds for compatibility with LibNFC !!!
  //

  if ((pService->pTagInfo->tagType & NFC_IND_TYPE_TYPE_MASK) == NFC_IND_TYPE_TAG_JEWEL_TYPE1)
  {
    // Set tag UID. For Type 1 fake the UID length (4) to NPX Google stack value. Our
    // value in 'legacy mode' is wrong (too short). The normal length is 7.
    LOG("setting UID length of TYPE1 to 4 (hack for compatibility with libnfc-nxp)\n");
    pService->pTagInfo->uidLength   = 4;
    LOG("setting SAK of TYPE1 to zero (hack for compatibility with libnfc-nxp)\n");
    pService->pTagInfo->p.typeA.sak = 0;
  }

  jniSetByteArray(env,
                  tag,
                  NativeTagFields.mUid,
                  pService->pTagInfo->uid,
                  pService->pTagInfo->uidLength);

  count = nfcGetTechnologyTree(
    pService,
    MAX_NUM_TECHNOLOGIES,
    technologies,
    handles,
    libnfctypes
    );

  //
  // Push the technology list into the java object
  //

  for (int i = 0; i < count; i++)
  {
    LOGV("Signal #%d technology=%X handle=%08x, libnfctype=%.08X",
         i,
         technologies[i],
         handles[i],
         libnfctypes[i]);
  }

  jniSetIntArray(env, tag, NativeTagFields.mTechList, technologies, count);
  jniSetIntArray(env, tag, NativeTagFields.mTechHandles, handles, count);
  jniSetIntArray(env, tag, NativeTagFields.mTechLibNfcTypes, libnfctypes, count);
  
  jfieldID id = NativeTagFields.mConnectedHandle;
  
  env->SetIntField(tag, id, -1);

  CleanupLastTag(pService, env);

  pService->lastSignaledTag    = tag;
  pService->transactionCount   = 0;

  // Notify manager that new a tag was found
  env->CallVoidMethod(pService->objectManager, nfcManagerNotifyTagDetected, tag);

  if (env->ExceptionCheck())
  {
    LOG("Exception occured");
  }
} // nfcSignalTagDetected

/*------------------------------------------------------------------*/
/**
 * @fn    nfcSignalTransaction
 *
 * @brief Signal transaction event from UICC to upper JAVA service layer
 *
 * @param PNfcService pService  - Pointer to our JNI service instance
 * @param PNfcEventParam pEvent - Pointer to transaction parameter
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void nfcSignalTransaction (PNfcService pService, PNfcTransactionInd pParam)
{
  JNIEnv *env = AquireJniEnv(pService);
  jobject aid = NULL;

  aid = env->NewByteArray(pParam->aidLength);
  if (!aid)
  {
    LOGE("! cannot alloc AID data, length %d", pParam->aidLength);
    env->ExceptionClear();
  }
  else
  {
    env->SetByteArrayRegion((jbyteArray) aid, 0, pParam->aidLength, (jbyte *) pParam->aid);
    LOG("Notify Nfc Service with transaction AID");

    //
    // Notify manager that a new event occurred on a SE, remember additional param data are lost
    // with the API
    //
    env->CallVoidMethod(pService->objectManager, nfcManagerNotifyTransaction, aid);

    env->DeleteLocalRef(aid);
  }
} // nfcSignalTransaction

/*------------------------------------------------------------------*/
/**
 * @fn    nfcSetTargetPollBytes
 *
 * @brief Set some low level (technology depended information) in upper
 *        layer JAVA object TAG instance
 *
 * @param JNIEnv * e  - Current JAVA VM environment
 * @param jobject tagObject    - Native pointer to current TAG instance
 * @param PNfcService pService - Pointer to our JNI service instance
 * @param PNfcTagInfo pTagInfo - Pointer to current TAG information
 *
 *
 */
/*------------------------------------------------------------------*/
void nfcSetTargetPollBytes (JNIEnv     *e,
                            jobject     tagObject,
                            PNfcService pService,
                            PNfcTagInfo pTagInfo)
{
  jclass       classz;
  jfieldID     id;
  jobjectArray existingPollBytes;
  
  (void) pService; // suspress unused parameter warning

  classz = e->GetObjectClass(tagObject);
  id     = e->GetFieldID(classz, "mTechPollBytes", "[[B");

  //
  // Set this bytes only once
  //

  existingPollBytes = (jobjectArray) e->GetObjectField(tagObject, id);

  if (existingPollBytes != NULL)
  {
    LOGE("No field 'mTechPollBytes' found for ID %X", (unsigned int)id);
    return;
  }

  jfieldID  techListField    = e->GetFieldID(classz, "mTechList", "[I");
  jintArray techList         = (jintArray) e->GetObjectField(tagObject, techListField);
  jint     *techId           = e->GetIntArrayElements(techList, 0);
  int       techListLength   = e->GetArrayLength(techList);

  jbyteArray   pollBytes     = e->NewByteArray(0);
  jobjectArray techPollBytes = e->NewObjectArray(techListLength, e->GetObjectClass(pollBytes), 0);

  for (int tech = 0; tech < techListLength; tech++)
  {
    LOGV("nfcSetTargetPollBytes, tech ID %.08X", techId[tech]);
    switch (techId[tech])
    {
      /* ISO14443-3A: ATQA/SENS_RES */
      case TARGET_TYPE_ISO14443_3A:
      case TARGET_TYPE_MIFARE_CLASSIC:
      case TARGET_TYPE_MIFARE_UL:
      case TARGET_TYPE_ISO14443_4:
        LOGV("SET ATQA %X", pTagInfo->p.typeA.atqa);
        pollBytes = e->NewByteArray(2);
        e->SetByteArrayRegion(pollBytes, 0, 2, (jbyte *) &pTagInfo->p.typeA.atqa);
        break;

      /* ISO14443-3B: Application data (4 bytes) and Protocol Info (3 bytes) from ATQB/SENSB_RES */
      case TARGET_TYPE_ISO14443_3B:
        pollBytes = e->NewByteArray(7);
        e->SetByteArrayRegion(pollBytes, 0, 4, (jbyte *) &pTagInfo->p.typeB.applicationData);
        if (pTagInfo->p.typeB.higherLayerResponseLength > 3)
        {
          // TODO is this OK ?
          e->SetByteArrayRegion(pollBytes, 4, 3, (jbyte *) &pTagInfo->p.typeB.higherLayerResponse);
        }
        break;

      /* JIS_X_6319_4: PAD0 (2 byte), PAD1 (2 byte), MRTI(2 byte), PAD2 (1 byte), RC (2 byte) */
      case TARGET_TYPE_FELICA:
      {
        BYTE systemCode[2];

        systemCode[1] = (BYTE) (pTagInfo->p.typeF.systemCode & 0xFF);
        systemCode[0] = (BYTE) (pTagInfo->p.typeF.systemCode >> 8);
        pollBytes     = e->NewByteArray(8 + 2);
        e->SetByteArrayRegion(pollBytes, 0, 8, (jbyte *) pTagInfo->p.typeF.pmm);
        e->SetByteArrayRegion(pollBytes, 8, 2, (jbyte *) systemCode);
      }
      break;

      /* ISO15693: response flags (1 byte), DSFID (1 byte) */
      case TARGET_TYPE_ISO15693:
        pollBytes = e->NewByteArray(2);
        e->SetByteArrayRegion(pollBytes, 0, 1, (jbyte *) &pTagInfo->p.typeV.flags);
        e->SetByteArrayRegion(pollBytes, 1, 1, (jbyte *) &pTagInfo->p.typeV.dsfId);
        break;

      default:
        pollBytes = e->NewByteArray(0);
        break;
    } // switch

    e->SetObjectArrayElement(techPollBytes, tech, pollBytes);
  }

  e->SetObjectField(tagObject, id, techPollBytes);

  e->ReleaseIntArrayElements(techList, techId, 0);
} // nfcSetTargetPollBytes

/*------------------------------------------------------------------*/
/**
 * @fn    nfcSetTargetActivationBytes
 *
 * @brief Utility to recover activation bytes from target infos
 *
 * @param JNIEnv * e           - Current JAVA VM environment
 * @param jobject tagObject    - Native pointer to current TAG instance
 * @param PNfcService pService - Pointer to our JNI service instance
 * @param PNfcTagInfo pTagInfo - Pointer to current TAG information
 *
 */
/*------------------------------------------------------------------*/
void nfcSetTargetActivationBytes (JNIEnv     *e,
                                  jobject     tagObject,
                                  PNfcService pService,
                                  PNfcTagInfo pTagInfo)
{
  jclass     classz;
  jfieldID   id;

  (void) pService; // suspress unused parameter warning

  classz = e->GetObjectClass(tagObject);
  id     = e->GetFieldID(classz, "mTechActBytes", "[[B");

  //
  // Set this bytes only once
  //
  if (e->GetObjectField(tagObject, id) != NULL)
  {
    return;
  }

  jfieldID  techListField   = e->GetFieldID(classz, "mTechList", "[I");
  jintArray techList        = (jintArray) e->GetObjectField(tagObject, techListField);
  int       techListLength  = e->GetArrayLength(techList);
  jint     *techId          = e->GetIntArrayElements(techList, 0);

  jbyteArray   actBytes     = e->NewByteArray(0);
  jobjectArray techActBytes = e->NewObjectArray(techListLength, e->GetObjectClass(actBytes), 0);

  for (int tech = 0; tech < techListLength; tech++)
  {
    switch (techId[tech])
    {
      /* ISO14443-3A: SAK/SEL_RES */
      case TARGET_TYPE_ISO14443_3A:
      case TARGET_TYPE_MIFARE_CLASSIC:
      case TARGET_TYPE_MIFARE_UL:
        actBytes = e->NewByteArray(1);
        e->SetByteArrayRegion(actBytes, 0, 1, (jbyte *) &pTagInfo->p.typeA.sak);
        LOGV("SET SAK %X", pTagInfo->p.typeA.sak);
        break;

      /* ISO14443-3A & ISO14443-4: SAK/SEL_RES, historical bytes from ATS */
      /* ISO14443-3B & ISO14443-4: HiLayerResp                           */
      case TARGET_TYPE_ISO14443_4:
        // Determine whether -A or -B
        switch (pTagInfo->tagType & NFC_IND_TYPE_TYPE_MASK)
        {
          case NFC_IND_TYPE_TAG_ISO14443A:
            actBytes = e->NewByteArray(pTagInfo->p.typeA.historicalBytesLength);
            if (pTagInfo->p.typeA.historicalBytesLength)
            {
              e->SetByteArrayRegion(actBytes,
                                    0,
                                    pTagInfo->p.typeA.historicalBytesLength,
                                    (jbyte *) pTagInfo->p.typeA.historicalBytes);
            }
            break;

          case NFC_IND_TYPE_TAG_14443B:
            actBytes = e->NewByteArray(pTagInfo->p.typeB.higherLayerResponseLength);
            if (pTagInfo->p.typeB.higherLayerResponseLength)
            {
              e->SetByteArrayRegion(actBytes, 0, pTagInfo->p.typeB.higherLayerResponseLength,
                                    (jbyte *) pTagInfo->p.typeB.higherLayerResponse);
            }
            break;

          default:
            actBytes = e->NewByteArray(0);
            break;
        } // switch
        break;

      /* ISO15693: response flags (1 byte), DSFID (1 byte) */
      case TARGET_TYPE_ISO15693:
        actBytes = e->NewByteArray(2);
        e->SetByteArrayRegion(actBytes, 0, 1, (jbyte *) &pTagInfo->p.typeV.flags);
        e->SetByteArrayRegion(actBytes, 1, 1, (jbyte *) &pTagInfo->p.typeV.dsfId);
        break;

      default:
        actBytes = e->NewByteArray(0);
        break;
    } // switch

    e->SetObjectArrayElement(techActBytes, tech, actBytes);
  }

  e->SetObjectField(tagObject, id, techActBytes);

  e->ReleaseIntArrayElements(techList, techId, 0);
} // nfcSetTargetActivationBytes


bool nfcManagerGetReaderEnable (void)
{
  bool result;
  JNIEnv *env = AquireJniEnv(gpService);
  result = env->CallBooleanMethod(gpService->objectManager, nfcManagerGetReaderEnablePersistant);
  
  LOGV("nfcManagerGetReaderEnable returned %d", (int)result);
  return result;
}

void nfcManagerSetReaderEnable (bool mode)
{
  JNIEnv *env = AquireJniEnv(gpService);
  LOGV("nfcManagerSetReaderEnable called with %d", (int)mode);
  env->CallVoidMethod(gpService->objectManager, nfcManagerSetReaderEnablePersistant, mode);
}

bool nfcManagerGetIp1TargetEnable (void)
{
  bool result;
  JNIEnv *env = AquireJniEnv(gpService);
  result = env->CallBooleanMethod(gpService->objectManager, nfcManagerGetIp1TargetEnablePersistant);
  
  LOGV("nfcManagerGetIp1TargetEnable returned %d", (int)result);
  return result;
}

void nfcManagerSetIp1TargetEnable (bool mode)
{
  JNIEnv *env = AquireJniEnv(gpService);
  LOGV("nfcManagerSetIp1TargetEnable called with %d", (int)mode);
  env->CallVoidMethod(gpService->objectManager, nfcManagerSetIp1TargetEnablePersistant, mode);
}

bool nfcManagerGetWlFeatEnable (void)
{
  bool result;
  JNIEnv *env = AquireJniEnv(gpService);
  result = env->CallBooleanMethod(gpService->objectManager, nfcManagerGetWlFeatEnablePersistant);
  
  LOGV("nfcManagerGetWlFeatEnable returned %d", (int)result);
  return result;
}

void nfcManagerSetWlFeatEnable (bool mode)
{
  JNIEnv *env = AquireJniEnv(gpService);
  LOGV("nfcManagerSetWlFeatEnable called with %d", (int)mode);
  env->CallVoidMethod(gpService->objectManager, nfcManagerSetWlFeatEnablePersistant, mode);
}




/*------------------------------------------------------------------*/
/**
 * @fn    nfcRegisterSignalFunctions
 *
 * @brief Initialize cache notification entry points from upper
 *        NFC JAVA layer
 *
 * @param JNIEnv * e  JAVA reference to current VM
 * @param jclass cls Service manager class reference
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void nfcRegisterSignalFunctions (JNIEnv *e, jclass cls)
{
// *INDENT-OFF*
  nfcManagerNotifyTagDetected         = e->GetMethodID(cls, "notifyNdefMessageListeners", "(L"NATIVE_TAG_CLASS";)V");
  nfcManagerNotifyTransaction         = e->GetMethodID(cls, "notifyTransactionListeners", "([B)V");
  nfcManagerNotifyLlcpLinkActivation  = e->GetMethodID(cls, "notifyLlcpLinkActivation",   "(L"NATIVE_P2P_CLASS";)V");
  nfcManagerNotifyLlcpLinkDeactivated = e->GetMethodID(cls, "notifyLlcpLinkDeactivated",  "(L"NATIVE_P2P_CLASS";)V");
  nfcManagerNotifyTargetRemoved       = e->GetMethodID(cls, "notifyTargetDeselected",     "()V");
  nfcManagerNotifyFieldOn             = e->GetMethodID(cls, "notifySeFieldActivated",     "()V");
  nfcManagerNotifyFieldOff            = e->GetMethodID(cls, "notifySeFieldDeactivated",   "()V");
  
  /* these aren't really notifications but hooks into our NativeNfcManager.java  to get and set preferences */
  nfcManagerGetReaderEnablePersistant = e->GetMethodID(cls, "getReaderEnablePersistant", "()Z");
  nfcManagerSetReaderEnablePersistant = e->GetMethodID(cls, "setReaderEnablePersistant", "(Z)V");

  /* entry-points to two test-modes  */
  nfcManagerGetIp1TargetEnablePersistant = e->GetMethodID(cls, "getIp1TargetEnablePersistant", "()Z");
  nfcManagerSetIp1TargetEnablePersistant = e->GetMethodID(cls, "setIp1TargetEnablePersistant", "(Z)V");
  nfcManagerGetWlFeatEnablePersistant    = e->GetMethodID(cls, "getWlFeatEnablePersistant", "()Z");
  nfcManagerSetWlFeatEnablePersistant    = e->GetMethodID(cls, "setWlFeaetEnablePersistant", "(Z)V");


// *INDENT-ON*
} // nfcRegisterSignalFunctions
}
