/** ----------------------------------------------------------------------
* File      : $RCSfile: com_android_nfc_NativeNfcManager.cpp,v $
* Revision  : $Revision: 1.97.2.14.4.6 $
* Date      : $Date: 2013/04/26 12:42:21 $
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
#include "com_android_nfc_ApiExtension.h"
#include "com_android_nfc_errorcodes.h"
#include "st21nfca_sefix.h"


#define EEDATA_SETTINGS_NUMBER        22

#define OS_SYNC_INFINITE              -1
#define OS_SYNC_RELEASED              0
#define OS_SYNC_TIMEOUT               1
#define OS_SYNC_FAILED                -1

// defines matching from framework NfcAdapter.java
#define SERVICE_STATE_UNDEF			0
#define SERVICE_STATE_OFF			1
#define SERVICE_STATE_TURNING_ON	2
#define SERVICE_STATE_ON			3
#define SERVICE_STATE_TURNING_OFF	4

const char *hwVersion =
  "JNI BUILD: STOLLMANN, ANDROID 4.0.3/4.x (R05-RC30), " __DATE__ ", " __TIME__;

PNfcService gpService = NULL;

#if defined (DEBUG) || defined (F_DEBUGOUT)
//int jniTraceMask = 0xFFFFFFFF;
//int traceMask    = 0xffffffff;
//int traceMaskL1  = 0xffffffff;
int jniTraceMask = 0xffffffff;
int traceMask    = 0xf37709f7;
int traceMaskL1  = 0x50005;
#else
int jniTraceMask = 0xffffffff;
int jniTraceMask = 0xffffffff;
int traceMask    = 0xf37709f7;
int traceMaskL1  = 0x50005;
#endif

namespace android {
  
static void loc_SetNfcEnableMode (PNfcService pService, BOOL enable);
  
static void com_android_nfc_NfcManager_doSetProperties (JNIEnv *e,
                                                        jobject o,
                                                        jint    param,
                                                        jint    value);

/*------------------------------------------------------------------*/
/**
 * @fn    nfcEventHandler
 *
 * @brief Global NFC stack event handler. Take argument from event
 *        ans schedule a message for later processing in our safe
 *        JAVA VM context.
 *
 * @param PVOID pvAppContext      - NFC service contetx pointer
 * @param NFCEVENT dwEvent        - NFC event. See nfcapi-spa doc.
 * @param PVOID pvListenerContext - NFC listener context if any
 * @param PNfcEventParam pEvent   - Special event param, depends on event
 *                                  See nfcapi-spa doc.
 *
 *
 * @return void NFCAPI
 */
/*------------------------------------------------------------------*/
void NFCAPI nfcEventHandler (PVOID          pvAppContext,
                             NFCEVENT       dwEvent,
                             PVOID          pvListenerContext,
                             PNfcEventParam pEvent)
{
  PNfcService pService = (PNfcService) pvAppContext;

  switch (dwEvent)
  {
    case nfcEventTagDetected:
      memcpy(&pService->tagInfo, &pEvent->tagEvent, sizeof(pService->tagInfo));

      LOGV("TARGET type %08x detected, listener %p",
           (unsigned int) pEvent->tagEvent.tagType,
           pvListenerContext);
           
      pService->previouslyDetectedTagType = pEvent->tagEvent.tagType;
      
      if (pService->pTagInfo->tagType & NFC_IND_TYPE_CE_HOST_MASK)
      {
      }
      else if (pService->pTagInfo->tagType & NFC_IND_TYPE_P2P_MASK)
      {
        nfcSignalNotification(pService, nfcNotificationP2PActivation, NULL, 0);
      }
      else if (pService->pTagInfo->tagType & NFC_IND_TYPE_CE_SECURE_CE)
      {
      }
      else
      {
        nfcSignalNotification(pService, nfcNotificationTagInsert, NULL, 0);
      }
      break;

    case nfcEventTagRemoved:
      nfcSignalNotification(pService, nfcNotificationTagRemove, NULL, 0);
      break;

    case nfcEventConnected:             /* P2P session established            */
      onLlcpSessionEstablished(pvAppContext, &pEvent->p2pConnect);
      break;

    case nfcEventConnectIndication:    /* P2P incoming call for listener       */
      onLlcpSessionIndication(pvListenerContext, &pEvent->p2pConnectInd);
      break;

    case nfcEventConnectProceeding:     /* P2P session is establishing        */
      break;

    case nfcEventConnectFailed:         /* P2P session failed        */
      onLlcpSessionFailed(pvAppContext, &pEvent->p2pDisconnect);
      break;

    case nfcEventDisconnected:          /* P2P session disconnect from remote */
      onLlcpSessionDisconnected(pvAppContext, &pEvent->p2pDisconnect);
      break;

    case nfcEventHangupTerminated:      /* P2P session disconnect terminated  */
      onLlcpSessionTerminated(pvAppContext, &pEvent->p2pDisconnect);
      break;

    case nfcEventDataTransmitted:       /* P2P Data transmit completed           */
      onLlcpDataTransmitted(pvAppContext, &pEvent->p2pData);
      break;

    case nfcEventDataIndication:        /* P2P New Data received                 */
      onLlcpDataIndication(pvAppContext, &pEvent->p2pData);
      break;

    case nfcEventReaderRequest:            /* Card emulation request PDU avail  */
      // Not supported yet
      break;

    case nfcEventTransaction:
      nfcSignalNotification(pService, nfcNotificationTransaction, &pEvent->transaction,
                            sizeof(pEvent->transaction));
      break;
      
    case nfcEventFieldActivity:
      if (pEvent->fieldState)  
      {
        nfcSignalNotification(pService, nfcNotificationFieldOn, NULL, 0);        
      }
      else
      {
        nfcSignalNotification(pService, nfcNotificationFieldOff, NULL, 0);        
      }
     break;
      
    default:
      LOGE("! nfcEventHandler(), unknown event %X", dwEvent);
      break;
  } // switch
}   // nfcEventHandler

/*------------------------------------------------------------------*/
static void com_android_uds_reset (PNfcService pService)
{
  LOG("Resetting Debug UDS connection");
  pService->debugUdsServer->dropConnection();
  pService->debugUdsServerState = UDS_STATE_DISCONNECTED;
  pService->debugUdsNumBytes    = 0;
}

/*------------------------------------------------------------------*/
static void com_android_debugUds_callback (void *context, UdsEvent event, void *data, size_t length)
{
  PNfcService pService = (PNfcService) context;

  switch (event)
  {
    case udsConnectIndication:
      if (pService->debugUdsServerState != UDS_STATE_DISCONNECTED)
      {
        LOGE("! UDS callback: udsConnectIndication in state %X", pService->debugUdsServerState);
        com_android_uds_reset(pService);
        break;
      
      }
      pService->debugUdsServerState = UDS_STATE_CONNECTED;
      break;

    case udsDataIndication:
      if (pService->debugUdsServerState != UDS_STATE_CONNECTED)
      {
        LOGE("! UDS callback: udsDataIndication in state %X", pService->debugUdsServerState);
        break;
      }
      if (pService->debugUdsNumBytes + length > UDS_EXPECTED_LENGTH)
      {
        LOGE("! UDS callback: Received too many bytes (%d)", pService->debugUdsNumBytes + length);
        com_android_uds_reset(pService);
        break;
      }
      memcpy(pService->debugUdsBuffer + pService->debugUdsNumBytes, data, length);
      pService->debugUdsNumBytes += length;
      if (pService->debugUdsNumBytes == UDS_EXPECTED_LENGTH)
      {
        /* Complete packet received, parse it */
        int first;
        int second;
        pService->debugUdsBuffer[pService->debugUdsNumBytes] = '\0';
        pService->debugUdsNumBytes = 0;
        if (sscanf((const char *) pService->debugUdsBuffer, "%x %x", &first, &second) != 2)
        {
          LOGE("! UDS callback: Error parsing received string %s", pService->debugUdsBuffer);
          com_android_uds_reset(pService);
          break;
        }
        LOGV("UDS callback: TraceMask=%#X TraceMask.L1=%#X", first, second);
        pService->traceMask   = first;
        pService->traceMaskL1 = second;

        pDebugSetFlagsEx(pService->traceMask, pService->traceMaskL1);
      }
      break;

    case udsDisconnectIndication:
      if (pService->debugUdsServerState != UDS_STATE_CONNECTED)
      {
        LOGE("! UDS callback: udsDisconnectIndication in state %X", pService->debugUdsServerState);
        break;
      }
      pService->debugUdsServerState = UDS_STATE_DISCONNECTED;
      break;

    default:
      LOGE("! udsCallback(), unknown event %#X", event);
      com_android_uds_reset(pService);
      break;
  } // switch
} /* com_android_debugUds_callback */



static void loc_SetNfcEnableMode (PNfcService pService, BOOL enable)
{
  WORD outLength = 0;

  pNfcDeviceIoControl(pService->hApplication, enable ? NFC_IO_ENABLE_NFC : NFC_IO_DISABLE_NFC,
                      NULL, 0, NULL, &outLength);

} /* st21SetNfcEnableMode */



static void loc_setAllSecureElementToMode (PNfcService pService, TSecureMode mode)
{
  if (pService->currentSEMode != mode)
  {
    for (int i=0; i<pService->seCount; i++)
    {
      TNfcInfoSE * se = &gpService->seInfo[i];
      
      if ((se->caps & seCapsSmartMX) && (pService->isPN544))
      {
        // this is secure element. Don't activate it on PN544 
        LOGV("workaround for bug: won't enable UICC/SE/SmartMX on PN544 to mode %d", mode);
        continue;
      }
      
      if (pService->isSt21 && !(se->caps & seCapsUICC))
      {
        // this is secure element is on the blacklist: don't do anything.
        //LOGV("UICC %d on blacklist. Ignored", i);
        LOGV("SE%d activation ignored (it's nota UICC, eSE probably)", i);
        continue;
      }
      
      if (pNfcSecureSetMode(pService->hApplication, se->hSecure, mode) != nfcStatusSuccess)
      {
        LOGV("! unable to set UICC/SE into mode %d", mode);
      }
    }
    pService->currentSEMode = mode;
  }
} /* loc_setAllSecureElementToMode */


/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_disableDiscovery
 *
 * @brief  Disable NFC discovery. We stop our NFC listener.
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object   JAVA reference to current JAVA manager object
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
static void com_android_nfc_NfcManager_disableDiscovery (JNIEnv *e, jobject o)
{
  PNfcService pService;
  NFCSTATUS   status = nfcStatusSuccess;

  LOG("NfcManager_disableDiscovery() enter");
  
  GLOBAL_LOCK();
  
  if (!globalIsInitialized)
  {
    LOG("!com_android_nfc_NfcManager_disableDiscovery() called while not initialized!");
    GLOBAL_UNLOCK();
    return;
  }

  CONCURRENCY_LOCK();

  pService = nfcGetServiceInstance(e, o);
  pService->discoveryIsEnabled = false;
  

  if (pService->hListener)
  {
   
    LOG("NfcManager_disableDiscovery: remove tag/llcp listener");
    status = pNfcUnregisterListener(pService->hListener);
    pService->hListener = 0;
  }
  
  if (pService->hTransactionListener)
  {
    LOG("NfcManager_disableDiscovery: remove transaction listener");
    status = pNfcUnregisterListener(pService->hTransactionListener);
    pService->hTransactionListener = 0;
  }
  
  // remove last referenced java object if exit..
  CleanupLastTag(pService, e);
  pService->pTagInfo->tagType = 0;

  if (pService->isSt21)
  {
    WORD outLength = 0;
    LOG("NfcManager_disableDiscovery: send HW Disconnect");
    pNfcDeviceIoControl(pService->hApplication,
                        NFC_IO_HW_DISCONNECT,
                        NULL,
                        0,
                        NULL,
                        &outLength);
  }

  CONCURRENCY_UNLOCK();

  LOG("NfcManager_disableDiscovery() exit");
  
  GLOBAL_UNLOCK();
} // com_android_nfc_NfcManager_disableDiscovery

/*------------------------------------------------------------------*/
/**
 * @fn    getSupportedListenerMask
 *
 * @brief get supported listener mask of attached NFC device
 *
 * @return bitmask
 */
/*------------------------------------------------------------------*/
DWORD getSupportedListenerMask (HANDLE hApp)
{
  /* get supported listener mask    */
  TNfcCapabilities caps;
  NFCSTATUS        status;

  status = pNfcGetCapabilities(hApp, &caps, sizeof(caps));

  if (status == nfcStatusSuccess)
  {
    return caps.controllerCaps[0]->dwSupportedListenMask;
  }
  else
  {
    return 0;
  }
} /* getListenerMask */




void nfcManagerStartStandardReader (PNfcService pService)
{
  // get listener support mask:
  NFCSTATUS status;
  
  DWORD support = getSupportedListenerMask(pService->hApplication);

  // these two should always be set because they aren't real tag-types but features
  support |= NFC_REQ_TYPE_NDEF;
  
  if (!nfcManagerGetIp1TargetEnable())
  {
    LOG("nfcManagerStartStandardReader: Disable IP1-Target mode (debug)");
    support &= ~NFC_REQ_TYPE_P2P_TARGET;
  }

  // filter out unsupported modes and report them
  if ((support & pService->listenMask) != pService->listenMask)
  {
    for (unsigned i = 0; i < 32; i++)
    {
      unsigned bit = 1 << i;

      if ((pService->listenMask & bit) && (!(support & bit)))
      {
        switch (bit)
        {
          case NFC_REQ_TYPE_TAG_14443A_106:
            LOG(
              "nfcManagerStartStandardReader: ISO14443A-106 requested but not supported by hardware. ignored");
            break;

          case NFC_REQ_TYPE_TAG_14443B_106:
            LOG(
              "nfcManagerStartStandardReader: ISO14443B-106 requested but not supported by hardware. ignored");
            break;

          case NFC_REQ_TYPE_TAG_15693:
            LOG(
              "nfcManagerStartStandardReader: ISO15693 requested but not supported by hardware. ignored");
            break;

          case NFC_REQ_TYPE_TAG_FELICA_212:
            LOG(
              "nfcManagerStartStandardReader: Felica 212 requested but not supported by hardware. ignored");
            break;

          case NFC_REQ_TYPE_TAG_FELICA_424:
            LOG(
              "nfcManagerStartStandardReader: Felica 424 requested but not supported by hardware. ignored");
            break;

          case NFC_REQ_TYPE_TAG_JEWEL_106:
            LOG(
              "nfcManagerStartStandardReader: Jewel 106 requested but not supported by hardware. ignored");
            break;

          case NFC_REQ_TYPE_P2P_INITIATOR:
            LOG(
              "nfcManagerStartStandardReader: P2P-Initiator requested but not supported by hardware. ignored");
            break;

          case NFC_REQ_TYPE_P2P_TARGET:
            LOG(
              "nfcManagerStartStandardReader: P2P-Target requested but not supported by hardware. ignored");
            break;
        } /* switch */
      }
    }
  }

  DWORD mask = (pService->listenMask & support);

  if (pService->hListener)
  {
    pNfcUnregisterListener(pService->hListener);
    pService->hListener = 0;
  }

  // remove last referenced java object if exit..
  CleanupLastTag(pService, AquireJniEnv(pService));
  pService->pTagInfo->tagType = 0;

  if (mask != 0)
  {
    status = pNfcRegisterListener(
      pService->hApplication,
      pService,
      mask | NFC_REQ_TYPE_NDEF,
      NULL, &pService->hListener);
  }
}


/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_enableDiscovery
 *
 * @brief Enable NFC discovery. We start our listen process
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object
 * @param jint mode - New discovery mode.
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
static void com_android_nfc_NfcManager_enableDiscovery (JNIEnv *e, jobject o, jint mode)
{
  NFCSTATUS   status;
  PNfcService pService;
  
  GLOBAL_LOCK();
  
  if (!globalIsInitialized)
  {
    LOG("!com_android_nfc_NfcManager_enableDiscovery() called while not initialized!");
    GLOBAL_UNLOCK();
    return;
  }

  
  CONCURRENCY_LOCK();

  pService = nfcGetServiceInstance(e, o);
  pService->discoveryIsEnabled = true;

  if (mode == DISCOVERY_MODE_TAG_READER)
  {
    
    LOGV("NfcManager_enableDiscovery in DISCOVERY_MODE_TAG_READER, hApplication=%p",
         pService->hApplication);
         
    if (!nfcManagerGetReaderEnable ())
    {
      LOG("NfcManager_enableDiscovery: listener for llcp/reader globally disabled (preference setting)");
    } 
    else 
    {
      nfcManagerStartStandardReader(pService);
      LOG("NfcManager_enableDiscovery: listener for llcp/reader globally enabled (preference setting)");
    }
  }

  if (pService->hTransactionListener)
  {
    pNfcUnregisterListener(pService->hTransactionListener);
    pService->hTransactionListener=0;
  }
 
  LOG("NfcManager_enableDiscovery: Register transaction listener");
  status = pNfcRegisterListener(
    pService->hApplication,
    pService,
    NFC_REQ_TYPE_CE_SECURE_CE,
    NULL,
    &pService->hTransactionListener);

  CONCURRENCY_UNLOCK();

  LOG("NfcManager_enableDiscovery() exit");
  
  GLOBAL_UNLOCK();
} // com_android_nfc_NfcManager_enableDiscovery

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_init_native_struc
 *
 * @brief  Initialize our NFC service object instance
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NfcManager_init_native_struc (JNIEnv *e, jobject o)
{
  jclass   cls;
  jfieldID f;

  LOG("NfcManager_init_native_struc");

  gpService = (PNfcService) calloc(sizeof(TNfcService), 1);
  if (gpService == NULL)
  {
    LOG("! Malloc of JNI server layer failed");
    return FALSE;
  }

  gpService->pTagInfo = &gpService->tagInfo;

  e->GetJavaVM(&(gpService->vm));
  gpService->env_version   = e->GetVersion();
  gpService->objectManager = e->NewGlobalRef(o);

  LOGV("New pService=%p, VM=%p env version %X", gpService, gpService->vm, gpService->env_version);

  //
  // Setup our reference into JAVA service instance
  //
  cls = e->GetObjectClass(o);
  f   = e->GetFieldID(cls, "mNative", "I");
  e->SetIntField(o, f, (jint) gpService);

  nfcRegisterSignalFunctions(e, cls);

  if (jniCacheObject(e, NATIVE_TAG_CLASS, &(gpService->objectNfcTag)) == -1)
  {
    free(gpService);
    return FALSE;
  }

  if (jniCacheObject(e, NATIVE_P2P_CLASS, &(gpService->objectP2pDevice)) == -1)
  {
    free(gpService);
    return FALSE;
  }

  return TRUE;
} // com_android_nfc_NfcManager_init_native_struc

void APIENTRY secureEnumCallBack (PVOID pvContext, PNfcInfoSE pInfoSE)
{
  PNfcService pService = (PNfcService) pvContext;

  if (pService->seCount < MAX_SE_COUNT)
  {
    memcpy(&pService->seInfo[pService->seCount], pInfoSE, sizeof(*pInfoSE));
    pService->seCount++;
  }
}


/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_initialize
 *
 * @brief Entry point for JNI layer, initialize the NFC subsystem
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NfcManager_initialize (JNIEnv *e, jobject o)
{
  PNfcService pService;
  jboolean    jResult = FALSE;
  NFCSTATUS   status;

  LOG("NfcManager_initialize() enter");
  GLOBAL_LOCK();
  
  if (globalIsInitialized)
  {
    LOG("!NfcManager_initialize() called while initialized!");
    GLOBAL_UNLOCK();
    return false;
  }
  
  /* Retrieve pNative structure address */
  pService = nfcGetServiceInstance(e, o);
  CONCURRENCY_LOCK();

  do
  {
    char szNfcParam[128];
    pService->ioBufferSize  = 65536;  // start with our guaranteed 64kb
    pService->pIoBuffer     = (PBYTE) malloc(pService->ioBufferSize);
    pService->traceMask     = traceMask;
    pService->traceMaskL1   = traceMaskL1;
    pService->seCount       = 0;
    pService->currentSEMode = seModeOff;
    pService->oemOptions    = 0;
    pService->discoveryIsEnabled = false;
    
    pService->pTagInfo = &pService->tagInfo;

    if (!pService->pIoBuffer)
    {
      LOGE("! Can't allocate IoBuffer (size %d)", pService->ioBufferSize);
      break;
    }

    sem_init(&pService->hSignalSemaphore, 0, 0);
    sem_init(&pService->hWaitSemaphore, 0, 0);
    pthread_mutex_init(&pService->hSendGuard, NULL);

    //
    // Create our worker notification thread
    //
    pthread_create(&(pService->hThread), NULL, nfcNotificationThread, pService);

    //
    // Loads the external NFC stack library
    //

    LOG("Load NFC stack library");
    status = nfcLoadLibrary();

    if (status != nfcStatusSuccess)
    {
      LOGE("! Cannot load lower NFC stack, status %X", status);
      break;
    }
    
    // get extra flags/hacks that may have been configured.
    if (jniGetSystemProperty("ro.stollmann.nfc.options", szNfcParam))
    {
      LOGV("got oem options: %s", szNfcParam);
      pService->oemOptions = atoi(szNfcParam);
      
      // remove obsolete oemOptions
      pService->oemOptions &= ~(32+64);
      LOGV("oem options after removing obsolete flags: %d", pService->oemOptions);
    }

    if (!jniGetSystemProperty("ro.stollmann.nfc.init", szNfcParam))
    {
      LOG("!can't access property ro.stollmann.nfc.init");
    }
    LOGV("NFC_INIT = <%s>", szNfcParam);

    //
    // Initialize the NFace+SPA layer
    //
    

    char szInit[256];
    sprintf(szInit,
            "%s TRACES.NFC=%08X TRACES.L1=%08X WLTESTMODE=%d",
            szNfcParam,
            pService->traceMask,
            pService->traceMaskL1,
            nfcManagerGetWlFeatEnable());
    
    // acitvate the next line to ignore szNfcParam string and get full debug
    // sprintf(szInit,"PORT=/dev/st21nfca IOTYPE=I2C TRACES.NFC=FFFFFFFF TRACES.L1=FFFFFFFF");

    LOGV("Call nfcInitialize <%s>", szInit);
    

    status = pNfcInitialize(szInit);
    if (status != nfcStatusSuccess)
    {
      LOGE("! nfcInitialize failed with %X", status);
      break;
    }

    //
    // Let us create our application first, we need this application always.
    //
    status = pNfcCreate(pService, nfcEventHandler, &pService->hApplication);
    if (status != nfcStatusSuccess)
    {
      if (status == nfcStatusLowLayerInitFailed)
      {
        if (pNfcDone)
        {
          pNfcDone();
        }

        nfcFreeLibrary();
      }
      break;
    }
    
    //
    // Start the listener for the Debug-Interface
    //
    
    pService->debugUdsServer = new UdsServer ("de.stollmann.nfc.debug",
                                              com_android_debugUds_callback,
                                              pService, FALSE);
                                              
    Nfc_ExtensionApi_Init();
    pService->extendUdsServer = new UdsServer("de.stollmann.nfc.customapi",
                                              Nfc_ExtensionApi_callback,
                                              pService, TRUE);

    pService->debugUdsServer->openServer();
    pService->extendUdsServer->openServer();

    //
    // We need knowledge about current stack caps.
    //

    LOGV("Get NFC capabilities for hApplication %p", pService->hApplication);

    PNfcCapabilities caps = (PNfcCapabilities) szNfcParam;
    status = pNfcGetCapabilities(pService->hApplication, caps, sizeof(szNfcParam));
    if (status)
    {
      LOGE("! nfcGetCapabilities failed with %X", status);
      break;
    }

    if (!caps->controllerCaps[0])
    {
      LOGE("! nfcGetCapabilities failed no controller caps, count %d", caps->controllerCount);
      break;
    }

    LOGV ("Vendor name = '%s'", caps->controllerCaps[0]->vendorName);
    
    gpService->isSt21  = false;
    gpService->isPN544 = false;
    if (strcmp((CHAR *) caps->controllerCaps[0]->vendorName, "STMICROELECTRONICS") == 0)
    {
      gpService->isSt21 = true;
    }
    if (strcmp((CHAR *) caps->controllerCaps[0]->vendorName, "NXP") == 0)
    {
      gpService->isPN544 = true;
    }

    if (pService->isSt21)
    {
      // enable 106kbit p2p in all cases:
      WORD outLength = 0;
      LOG("config: enable 106kbit LLCP (Mode-A)");
      pNfcDeviceIoControl(pService->hApplication, NFC_IO_106KBIT_P2P_ON, NULL, 0, NULL, &outLength);      

      if (pService->oemOptions & QUIRK_UICC_ENABLE_LOWBAT)
      {
        LOG("config: enable UICC in battery low mode.");
        pNfcDeviceIoControl(pService->hApplication, NFC_IO_SWP_MODE_ON, NULL, 0, NULL, &outLength);      
      }
      else
      {
        LOG("config: disable UICC in battery low mode.");
        pNfcDeviceIoControl(pService->hApplication, NFC_IO_SWP_MODE_OFF, NULL, 0, NULL, &outLength);      
      }
    }

    // set polling interval to 150ms
    WORD pollTime  = 150;
    WORD outLength = 0;
    
    LOGV("config: set tag presence polling to %dms", (int)pollTime);
    pNfcDeviceIoControl(pService->hApplication,
                        NFC_IO_TAG_POLL_TIME,
                        (unsigned char *)&pollTime,
                        sizeof (pollTime),
                        NULL,
                        &outLength);

    // turn on nfc mode 
    loc_SetNfcEnableMode(pService, TRUE);
  
    //
    // Register current supported listener mask
    //

    pService->supportedListenMask = caps->controllerCaps[0]->dwSupportedListenMask;

    LOGV("Found native supported listen mask %08x for Vendor <%s>",
         (unsigned int) pService->supportedListenMask,
         caps->controllerCaps[0]->vendorName);

    //
    // Now get the secure element environment. First Get the list of known secure elements
    //

    pNfcSecureGetList( pService->hApplication, pService, secureEnumCallBack);
    LOGV("found %d secure elements", pService->seCount);
    
  
    socketListInit(&gpService->sockets);

    if (pService->isSt21)
    {
      status = pNfcRegisterListener(
        pService->hApplication,
        pService,
        NFC_REQ_TYPE_RF_INDICATION,
        NULL,
        &pService->hFieldListener);
        
      if (status != nfcStatusSuccess)
      {
        LOGE("! register field listener failed with %X", status);
        break;
      }
    }
    jResult = TRUE;
  } while (FALSE);
    

  CONCURRENCY_UNLOCK();

  if (!jResult && pService)
  {
    if (pService->pIoBuffer)
    {
      free(pService->pIoBuffer);
    }
  }

  LOGV("NfcManager_initialize() exit %s", jResult ? "SUCCESS" : "! FAILED");

  /* android4 does not set these parameters anymore, do this for compatibility with
   * the defaults derived from android 2.3.4 */
   
   // TODO: Set these based on the flags, also maybe increase the MIU
  com_android_nfc_NfcManager_doSetProperties(e, o, PROPERTY_NFC_DISCOVERY_A, 1);
  com_android_nfc_NfcManager_doSetProperties(e, o, PROPERTY_NFC_DISCOVERY_B, 1);
  com_android_nfc_NfcManager_doSetProperties(e, o, PROPERTY_NFC_DISCOVERY_F, 1);
  com_android_nfc_NfcManager_doSetProperties(e, o, PROPERTY_NFC_DISCOVERY_15693, 1);
  com_android_nfc_NfcManager_doSetProperties(e, o, PROPERTY_NFC_DISCOVERY_NCFIP, 1);
  com_android_nfc_NfcManager_doSetProperties(e, o, PROPERTY_LLCP_LTO, 150);
  com_android_nfc_NfcManager_doSetProperties(e, o, PROPERTY_LLCP_MIU, 248);
  
  com_android_nfc_NfcManager_doSetProperties(e, o, PROPERTY_LLCP_WKS, 16);
  com_android_nfc_NfcManager_doSetProperties(e, o, PROPERTY_LLCP_OPT, 0);

  globalIsInitialized = jResult;

  GLOBAL_UNLOCK();

  return jResult;
} // com_android_nfc_NfcManager_initialize

/*------------------------------------------------------------------*/
/**
 * @fn    osWait
 *
 * @brief Wait for given semaphore signaling a specific time or ever
 *
 * @param sem_t * pSemaphore
 * @param DWORD timeout
 *
 *
 * @return BOOL
 */
/*------------------------------------------------------------------*/
static DWORD osWait (sem_t *pSemaphore, int timeout)
{
  DWORD result = OS_SYNC_RELEASED;

  if (timeout == OS_SYNC_INFINITE)
  {
    do
    {
      if (sem_wait(pSemaphore) == -1)
      {
        int  e = errno;
        char msg[200];

        if (e == EINTR)
        {
          LOG("! semaphore (infin) wait interrupted by system signal. re-enter wait");
          continue;
        }

        strerror_r(e, msg, sizeof(msg) - 1);
        LOGV("! semaphore (infin) wait failed. sem=0x%p, %s", pSemaphore, msg);

        result = OS_SYNC_FAILED;
      }
    } while (0);
  }
  else
  {
    struct timespec tm;
    long            oneSecInNs = (int) 1e9;

    clock_gettime(CLOCK_REALTIME, &tm);

    /* add timeout (can't overflow): */
    tm.tv_sec  += (timeout / 1000);
    tm.tv_nsec += ((timeout % 1000) * 1000000);

    /* make sure nanoseconds are below a million */
    if (tm.tv_nsec >= oneSecInNs)
    {
      tm.tv_sec++;
      tm.tv_nsec -= oneSecInNs;
    }

    do
    {
      if (sem_timedwait(pSemaphore, &tm) == -1)
      {
        int e = errno;

        if (e == EINTR)
        {
          LOG("! semaphore (timed) wait interrupted by system signal. re-enter wait");
          continue;
        }

        if (e == ETIMEDOUT)
        {
          result = OS_SYNC_TIMEOUT;
        }
        else
        {
          char msg[200];
          strerror_r(e, msg, sizeof(msg) - 1);
          LOGV(" ! semaphore (timed) wait failed. sem=0x%p, %s", pSemaphore, msg);
          result = OS_SYNC_FAILED;
        }
      }
    } while (0);
  }
  return result;
} // osWait

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_deinitialize
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NfcManager_deinitialize (JNIEnv *e, jobject o, int serviceState)
{
  PNfcService pService;
  
  GLOBAL_LOCK();
  
  if (!globalIsInitialized)
  {
    LOG("!com_android_nfc_NfcManager_deinitialize() while not initialized!");
    GLOBAL_UNLOCK();
    return false;
  }
  
  pService = nfcGetServiceInstance(e, o);

  LOG("NfcManager_deinitialize() enter");

  nfcSocketDiscardList(pService);
  socketListDestroy(&pService->sockets);
  
  if (pService->debugUdsServer)
  {
    delete pService->debugUdsServer;
    pService->debugUdsServer = 0;
  }

  if (pService->extendUdsServer)
  {
    delete pService->extendUdsServer;
    pService->extendUdsServer = 0;
  }

  CleanupLastTag(pService, e);

  if (pService->hApplication)
  {
    LOG("destroy NFC application handle and all active listeners:");
    pNfcDestroy(pService->hApplication);
    pService->hApplication         = NULL;
    pService->hFieldListener       = NULL;
    pService->hListener            = NULL;
    pService->hTransactionListener = NULL;
  }

  // we will not get any notifications anymore, so we can shut down the service thread:
  LOG("signal service thread termination.");
  pthread_detach(pService->hThread);
  nfcSignalNotification(pService, nfcNotificationQuit, NULL, 0);  

  // create a new app w/o any listeners, so we can turn off NFC power etc..
  pNfcCreate(pService, nfcEventHandler, &pService->hApplication);

  // Turn off NFC again, so we can turn off the chip:
  if ( ( serviceState == SERVICE_STATE_ON || serviceState == SERVICE_STATE_TURNING_ON)
      && pService->oemOptions & QUIRK_NFC_ON_AFTER_SHUTDOWN)
  {
    loc_SetNfcEnableMode(pService, TRUE);
  }
  else
  {
    loc_SetNfcEnableMode(pService, FALSE);
  }
  

  if (pService->isSt21)
  {
    // Disconnect from HOST:
    WORD outLength = 0;
    LOG("com_android_nfc_NfcManager_deinitialize: send HW Disconnect");
    pNfcDeviceIoControl(pService->hApplication,
                        NFC_IO_HW_DISCONNECT,
                        NULL,
                        0,
                        NULL,
                        &outLength);
  }
  
  // wait a moment so the last commands have a chance to reach the chip.
  usleep (100*1000);

  // final shutdown:
  pNfcDestroy(pService->hApplication);
  pService->hApplication = NULL;
  pNfcDone();


  // we give the thread two seconds to terminate.
  if (osWait(&pService->hWaitSemaphore, 2000) == OS_SYNC_TIMEOUT)
  {
    LOG("NfcManager_deinitialize: ! Service thread failed terminate");
  }
  else
  {
    LOG("Service thread terminated");
  }

  sem_destroy(&pService->hWaitSemaphore);
  sem_destroy(&pService->hSignalSemaphore);
  pthread_mutex_destroy(&pService->hSendGuard);

  nfcFreeLibrary();

  if (pService->pIoBuffer)
  {
    free(gpService->pIoBuffer);
    gpService->pIoBuffer = 0;
  }

  LOG("NfcManager_deinitialize() exit");
  
  globalIsInitialized = FALSE;
  
  GLOBAL_UNLOCK();
  return true;
} // com_android_nfc_NfcManager_deinitialize


/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_doSelectSecureElement
 *
 * @brief Switch on virtual access mode for secure element by given
 *        ID 'seID'
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void com_android_nfc_NfcManager_doSelectSecureElement (JNIEnv *e, jobject o)
{
  LOG("NfcManager_doSelectSecureElement");
  GLOBAL_LOCK();
  
  if (!globalIsInitialized)
  {
    LOG("!com_android_nfc_NfcManager_doSelectSecureElement called while not initialized!");
    GLOBAL_UNLOCK();
    return;
  }
  
  PNfcService service = nfcGetServiceInstance(e, o);
  
  CONCURRENCY_LOCK();
  loc_setAllSecureElementToMode (service, seModeVirtual);
  CONCURRENCY_UNLOCK();
  GLOBAL_UNLOCK();
} 

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_doDeselectSecureElement
 *
 * @brief Delesect secure element by given ID 'seID'
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void com_android_nfc_NfcManager_doDeselectSecureElement (JNIEnv *e, jobject o)
{
  LOG("NfcManager_doDeselectSecureElement");
  GLOBAL_LOCK();
  
  if (!globalIsInitialized)
  {
    LOG("!com_android_nfc_NfcManager_doDeselectSecureElement called while not initialized!");
    GLOBAL_UNLOCK();
    return;
  }
    
  PNfcService service = nfcGetServiceInstance(e, o);

  CONCURRENCY_LOCK();
  loc_setAllSecureElementToMode (service, seModeOff);
  CONCURRENCY_UNLOCK();
  
  GLOBAL_UNLOCK();
} 



/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_doCreateLlcpServiceSocket
 *
 * @brief Start a LLCP listen process. Remember  this call is running
 *        in a higher layer thread. So we can run until a incoming
 *        LLCP session is established.
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object
 * @param jint nSap
 * @param jstring sn
 * @param jint miu
 * @param jint rw
 * @param jint linearBufferLength
 *
 *
 * @return jobject
 */
/*------------------------------------------------------------------*/
static jobject com_android_nfc_NfcManager_doCreateLlcpServiceSocket (JNIEnv *e,
                                                                     jobject o,
                                                                     jint    nSap,
                                                                     jstring sn,
                                                                     jint    miu,
                                                                     jint    rw,
                                                                     jint    linearBufferLength)
{
  jobject     socketObject = NULL;
  PLlcpSocket pSocket;
  PNfcService pService;

  NFCSTATUS status;
  char      szServiceName[256];
  
  LOGV("NfcManager_doCreateLlcpServiceSocket() enter, nSap=0x%.02X", nSap);

  pService = nfcGetServiceInstance(e, o);

  socketListAquire(&pService->sockets);

  do
  {
    pSocket = nfcSocketCreate(e, NATIVE_SERVER_SOCKET_CLASS);
    if (!pSocket)
    {
      LOGE("Cannot alloc LLCP service socket for sap %d", nSap);
      break;
    }

    socketListAdd(&pService->sockets, &pSocket->listNode);

    if (!socketNodeAquire(&pSocket->listNode))
    {
      LOG("Cannot aquire ownership for socket");
      break;
    }

    PCHAR pszServiceName;

    if (sn == NULL)
    {
      pszServiceName = NULL;
    }
    else
    {
      size_t length = e->GetStringUTFLength(sn);
      if (length > sizeof(szServiceName))
      {
        length = sizeof(szServiceName);
      }

      memcpy(szServiceName, (char *) e->GetStringUTFChars(sn, NULL), length);
      szServiceName[length] = 0;
      pszServiceName        = szServiceName;

      // Ignore SAP value when we have a service name

      LOGV("Sap value 0x%X ignored, service name <%s> given", nSap, pszServiceName);
      nSap = 0;
    }
    
    CONCURRENCY_LOCK();

    status = pNfcP2PListenRequest(
      pService->hApplication,
      pSocket,
      nfcEventHandler,
      1,
      rw,
      (WORD) miu,
      (BYTE) nSap,
      pszServiceName,
      &pSocket->hServer
      );

    CONCURRENCY_UNLOCK();

    if (status != nfcStatusSuccess)
    {
      LOGE("LLCP setup listen failed with status %X", status);
      break;
    }

    // OK incoming connection detected, we exit here and use
    // doAceppt to trigger response

    /* copy data to java object */
    e->SetIntField(pSocket->object, NativeServiceSocketFields.mHandle, (jint) pSocket);
    e->SetIntField(pSocket->object,
                   NativeServiceSocketFields.mLocalLinearBufferLength,
                   (jint) linearBufferLength);
    e->SetIntField(pSocket->object, NativeServiceSocketFields.mLocalMiu, (jint) miu);
    e->SetIntField(pSocket->object, NativeServiceSocketFields.mLocalRW, (jint) rw);

    socketObject = pSocket->object;
  } while (FALSE);

  if (pSocket)
  {
    socketNodeRelease(&pSocket->listNode, &pService->sockets);

    if (!socketObject)
    {
      // late creation failed
      if (socketListRemove(&pService->sockets, &pSocket->listNode))
      {
        nfcSocketDestroy(pSocket);
      }
      pSocket = 0;
    }
  }

  LOGV("NfcManager_doCreateLlcpServiceSocket() exit, socket object %p", socketObject);

  socketListRelease(&pService->sockets);
  return socketObject;
} // com_android_nfc_NfcManager_doCreateLlcpServiceSocket

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_doCreateLlcpSocket
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object
 * @param jint nSap
 * @param jint miu
 * @param jint rw
 * @param jint linearBufferLength
 *
 *
 * @return jobject
 */
/*------------------------------------------------------------------*/
static jobject com_android_nfc_NfcManager_doCreateLlcpSocket (JNIEnv *e,
                                                              jobject o,
                                                              jint    nSap,
                                                              jint    miu,
                                                              jint    rw,
                                                              jint    linearBufferLength)
{
  jobject     socketObject = NULL;
  PLlcpSocket pSocket;
  PNfcService pService;

  (void) linearBufferLength; // suspress unused parameter warning

  LOGV("NfcManager_doCreateLlcpSocket() enter, nSap=0x%.02X. miu=%d rw=%d", nSap, miu, rw);

  pService = nfcGetServiceInstance(e, o);

  socketListAquire(&pService->sockets);

  pSocket = nfcSocketCreate(e, NATIVE_SOCKET_CLASS);

  if (pSocket) do 
  {
    pSocket->localRw  = rw;
    pSocket->localMiu = miu;

    // copy data into the java-object
    e->SetIntField(pSocket->object, NativeSocketFields.mHandle,   (jint) pSocket);
    e->SetIntField(pSocket->object, NativeSocketFields.mSap,      (jint) nSap);
    e->SetIntField(pSocket->object, NativeSocketFields.mLocalMiu, (jint) miu);
    e->SetIntField(pSocket->object, NativeSocketFields.mLocalRW,  (jint) rw);

    // Returns reference to new create object
    socketObject = pSocket->object;
    
    pSocket->rxBuffer = new RingBuffer();
    if (!pSocket->rxBuffer)
    {
      LOG("! ringbuffer allocation failed\n");
      nfcSocketDestroy(pSocket);
      pSocket      = 0;
      socketObject = 0;
      break;
    }
    
    // space estimate taken from open-source stack
    size_t bufferSpace = miu*(rw+1)+linearBufferLength;
    
    if (!pSocket->rxBuffer->allocate(bufferSpace))
    {
      LOG("! ringbuffer mem allocation failed\n");
      delete pSocket->rxBuffer;
      nfcSocketDestroy(pSocket);
      pSocket      = 0;
      socketObject = 0;
      break;
    }

    LOGV("RingBuffer for socket allocated with %d bytes\n", bufferSpace);

    socketListAdd(&pService->sockets, &pSocket->listNode);
  } while (0);

  socketListRelease(&pService->sockets);

  LOGV("NfcManager_doCreateLlcpSocket() exit, socket %p", socketObject);
  return socketObject;
} // com_android_nfc_NfcManager_doCreateLlcpSocket

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_doGetLastError
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object
 *
 *
 * @return jint
 */
/*------------------------------------------------------------------*/
static jint com_android_nfc_NfcManager_doGetLastError (JNIEnv *e, jobject o)
{
  PNfcService pService;

  pService = nfcGetServiceInstance(e, o);
  if(pService)
  {
	  LOGV("Last Error Status = 0x%02x", pService->lastError);

	  //  returns error codes already translated by mapNfcStatusToAndroid
	  return pService->lastError;
	}
	else return ERROR_INVALID_PARAM;
} // com_android_nfc_NfcManager_doGetLastError

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_doSetProperties
 *
 * @brief  Set global properties for NFC stack. Remember this funcions
 *         is called before initialialize.
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object
 * @param jint param
 * @param jint value
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
static void com_android_nfc_NfcManager_doSetProperties (JNIEnv *e,
                                                        jobject o,
                                                        jint    param,
                                                        jint    value)
{
  PNfcService pService;

  /* Retrieve pNative structure address */
  pService = nfcGetServiceInstance(e, o);

  switch (param)
  {
    case PROPERTY_LLCP_LTO:
      LOGV("> Set LLCP LTO to %d", value);
      pService->lto = value;
      break;

    case PROPERTY_LLCP_MIU:
      LOGV("> Set LLCP MIU to %d", value);
      pService->miu = value;
      break;

    case PROPERTY_LLCP_WKS:
      LOGV("> Set LLCP WKS to %d", value);
      pService->wks = value;
      break;

    case PROPERTY_LLCP_OPT:
      LOGV("> Set LLCP OPT to %d", value);
      pService->opt = value;
      break;

    case PROPERTY_NFC_DISCOVERY_A:
      LOGV("> Set NFC DISCOVERY A to %d", value);
      if (value)
      {
        pService->listenMask |= (NFC_REQ_TYPE_TAG_14443A_106 | NFC_REQ_TYPE_TAG_JEWEL_106);
      }
      else
      {
        pService->listenMask &= ~(NFC_REQ_TYPE_TAG_14443A_106 | NFC_REQ_TYPE_TAG_JEWEL_106);
      }
      break;

    case PROPERTY_NFC_DISCOVERY_B:
      LOGV("> Set NFC DISCOVERY B to %d", value);
      if (value)
      {
        pService->listenMask |= NFC_REQ_TYPE_TAG_14443B_106;
      }
      else
      {
        pService->listenMask &= ~NFC_REQ_TYPE_TAG_14443B_106;
      }
      break;

    case PROPERTY_NFC_DISCOVERY_F:
      LOGV("> Set NFC DISCOVERY F to %d", value);
      if (value)
      {
        pService->listenMask |= (NFC_REQ_TYPE_TAG_FELICA_212 | NFC_REQ_TYPE_TAG_FELICA_424);
      }
      else
      {
        pService->listenMask &= ~(NFC_REQ_TYPE_TAG_FELICA_212 | NFC_REQ_TYPE_TAG_FELICA_424);
      }
      break;

    case PROPERTY_NFC_DISCOVERY_15693:

      LOGV("> Set NFC DISCOVERY 15693 to %d", value);
      if (value)
      {
        pService->listenMask |= NFC_REQ_TYPE_TAG_15693;
      }
      else
      {
        pService->listenMask &= ~NFC_REQ_TYPE_TAG_15693;
      }
      break;

    case PROPERTY_NFC_DISCOVERY_NCFIP:
      LOGV("> Set NFC DISCOVERY NFCIP to %d", value);
      if (value)
      {
        pService->listenMask |= (NFC_REQ_TYPE_P2P_INITIATOR | NFC_REQ_TYPE_P2P_TARGET);
      }
      else
      {
        pService->listenMask &= ~(NFC_REQ_TYPE_P2P_INITIATOR | NFC_REQ_TYPE_P2P_TARGET);
      }
      break;

    default:
      LOGV("> ! Unknown Property %X", param);
      break;
  } // switch
}   // com_android_nfc_NfcManager_doSetProperties

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_doSetIsoDepTimeout
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object
 * @param jint timeout
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void com_android_nfc_NfcManager_doSetIsoDepTimeout (JNIEnv *e, jobject o, jint timeout)
{
  (void) o; // suspress unused parameter warning  
  (void) e; // suspress unused parameter warning  
  LOGE("NfcManager_doSetIsoDepTimeout %d not supported, DIRTY", timeout);
}

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NfcManager_doResetIsoDepTimeout
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o - JAVA reference to current JAVA manager object
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void com_android_nfc_NfcManager_doResetIsoDepTimeout (JNIEnv *e, jobject o)
{
  (void) o; // suspress unused parameter warning  
  (void) e; // suspress unused parameter warning  
  LOG("! NfcManager_doSetIsoDepTimeout  not supported");
}

/*
 *
 *
 *     Entrypoints for Android 4
 *
 *
 */

static void com_android_nfc_NfcManager_enableDiscovery_A4 (JNIEnv *e, jobject o)
{
  com_android_nfc_NfcManager_enableDiscovery(e, o, DISCOVERY_MODE_TAG_READER);
}

static bool com_android_nfc_NfcManager_doSetTimeoutA4 (JNIEnv *e,
                                                       jobject o,
                                                       jint    tech,
                                                       jint    timeout)
{
  (void) o; // suspress unused parameter warning  
  (void) e; // suspress unused parameter warning  
  LOGE("NfcManager_doSetTimeout %d %d called", tech, timeout);
  return true;
}

jint com_android_nfc_NfcManager_doGetTimeoutA4 (JNIEnv *e, jobject o, jint tech)
{
  (void) o; // suspress unused parameter warning  
  (void) e; // suspress unused parameter warning  
  LOGE("NfcManager_doGetTimeout %d called", tech);
  return -1;
}

static void com_android_nfc_NfcManager_doResetTimeoutsA4 (JNIEnv *e, jobject o)
{
  (void) o; // suspress unused parameter warning  
  (void) e; // suspress unused parameter warning  
  LOG("NfcManager_doResetTimeouts called");
}

static void com_android_nfc_NfcManager_doAbortA4 (JNIEnv *e, jobject o)
{
  (void) o; // suspress unused parameter warning  
  (void) e; // suspress unused parameter warning  
  LOG("! Emergency_recovery requested from upper layer: Enforce restart of NFC service");
  abort();  // force a noisy crash
}

static jstring com_android_nfc_NfcManager_doDumpA4 (JNIEnv *e, jobject o)
{
  (void) o; // suspress unused parameter warning  
  (void) e; // suspress unused parameter warning  
  return e->NewStringUTF("libnfc llc error_count=0");
}

/*
 * JNI registration.
 */

// *INDENT-OFF*
static JNINativeMethod gMethods[] =
{
 {"initializeNativeStructure", "()Z",                    (void *)com_android_nfc_NfcManager_init_native_struc},
 {"initialize", "()Z",                                   (void *)com_android_nfc_NfcManager_initialize},
 {"deinitialize", "(I)Z",                                 (void *)com_android_nfc_NfcManager_deinitialize},
 {"doGetLastError", "()I",                               (void *)com_android_nfc_NfcManager_doGetLastError},
 {"enableDiscovery", "()V",                              (void *)com_android_nfc_NfcManager_enableDiscovery_A4},
 {"disableDiscovery", "()V",                             (void *)com_android_nfc_NfcManager_disableDiscovery},
 {
    "doCreateLlcpServiceSocket",
    "(ILjava/lang/String;III)L"NATIVE_SERVER_SOCKET_CLASS";",
    (void *)com_android_nfc_NfcManager_doCreateLlcpServiceSocket
 },
 {
    "doCreateLlcpSocket",
    "(IIII)L"NATIVE_SOCKET_CLASS";",
    (void *)com_android_nfc_NfcManager_doCreateLlcpSocket
 },

 {"doSelectSecureElement", "()V",                        (void *)com_android_nfc_NfcManager_doSelectSecureElement},
 {"doDeselectSecureElement", "()V",                      (void *)com_android_nfc_NfcManager_doDeselectSecureElement},
 {"doSetTimeout", "(II)Z",                               (void *)com_android_nfc_NfcManager_doSetTimeoutA4},
 {"doGetTimeout", "(I)I",                                (void *)com_android_nfc_NfcManager_doGetTimeoutA4},
 {"doResetTimeouts", "()V",                              (void *)com_android_nfc_NfcManager_doResetTimeoutsA4},
 {"doAbort", "()V",                                      (void *)com_android_nfc_NfcManager_doAbortA4},
 {"doDump", "()Ljava/lang/String;",                      (void *)com_android_nfc_NfcManager_doDumpA4},
};
// *INDENT-ON*

bool register_com_android_nfc_NativeNfcManager (JNIEnv *e)
{
  return jniRegisterNativeMethods(e, NATIVE_MANAGER_CLASS, gMethods, NELEM(gMethods));
  

      
}
} /* namespace android */
