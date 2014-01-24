/** ----------------------------------------------------------------------
* File      : $RCSfile: com_android_nfc.h,v $
* Revision  : $Revision: 1.47.2.5 $
* Date      : $Date: 2012/09/12 16:46:31 $
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
* $log$
*
----------------------------------------------------------------------*/

#ifndef __COM_ANDROID_NFC_JNI_H__
#define __COM_ANDROID_NFC_JNI_H__

#include <jni.h>

// IN NDK build environment this file is not avail
// #include <JNIHelp.h>

#ifndef NELEM
  #define NELEM(x)  ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <android/log.h>

// #include <cutils/properties.h> // for property_get

#include <nfcapi.h>
#include "udsServer.h"
#include "nfcSocketList.h"
#include "ringbuffer.h"

extern int             jniTraceMask;
extern pthread_mutex_t debugOutputSem;


// *INDENT-OFF*
//#define LOG(s)             if (!jniTraceMask ) {} else __android_log_print(ANDROID_LOG_DEBUG, "nfc-jni",s)
//#define LOGV(s,...)        if (!jniTraceMask ) {} else __android_log_print(ANDROID_LOG_DEBUG, "nfc-jni",s,__VA_ARGS__)
//#define LOGE(s,...)         __android_log_print(ANDROID_LOG_ERROR, "nfc-jni",s,__VA_ARGS__)
#define LOG(s)             if (!jniTraceMask ) {} else { pthread_mutex_lock (&debugOutputSem); __android_log_print(ANDROID_LOG_DEBUG, "nfc-jni",s); pthread_mutex_unlock (&debugOutputSem);}
#define LOGV(s,...)        if (!jniTraceMask ) {} else { pthread_mutex_lock (&debugOutputSem); __android_log_print(ANDROID_LOG_DEBUG, "nfc-jni",s,__VA_ARGS__); pthread_mutex_unlock (&debugOutputSem); }
#define LOGE(s,...)        { pthread_mutex_lock (&debugOutputSem); __android_log_print(ANDROID_LOG_ERROR, "nfc-jni",s,__VA_ARGS__); pthread_mutex_unlock (&debugOutputSem);}
// *INDENT-ON*


#define NATIVE_SOCKET_CLASS         "com/android/nfc/stollmann/NativeLlcpSocket"
#define NATIVE_SERVER_SOCKET_CLASS  "com/android/nfc/stollmann/NativeLlcpServiceSocket"
#define NATIVE_TAG_CLASS            "com/android/nfc/stollmann/NativeNfcTag"
#define NATIVE_P2P_CLASS            "com/android/nfc/stollmann/NativeP2pDevice"
#define NATIVE_SE_CLASS             "com/android/nfc/stollmann/NativeNfcSecureElement"
#define NATIVE_MANAGER_CLASS        "com/android/nfc/stollmann/NativeNfcManager"
#define NATIVE_CL_SOCKET_CLASS      "com/android/nfc/stollmann/NativeLlcpConnectionlessSocket"


/* Discovery modes -- keep in sync with NFCManager.DISCOVERY_MODE_* */
#define DISCOVERY_MODE_TAG_READER      0
#define DISCOVERY_MODE_NFCIP1          1
#define DISCOVERY_MODE_CARD_EMULATION  2

#define DISCOVERY_MODE_TABLE_SIZE      3

#define DISCOVERY_MODE_DISABLED        0
#define DISCOVERY_MODE_ENABLED         1

#define MODE_P2P_TARGET                0
#define MODE_P2P_INITIATOR             1

/* Properties values */
#define PROPERTY_LLCP_LTO             0
#define PROPERTY_LLCP_MIU             1
#define PROPERTY_LLCP_WKS             2
#define PROPERTY_LLCP_OPT             3
#define PROPERTY_NFC_DISCOVERY_A      4
#define PROPERTY_NFC_DISCOVERY_B      5
#define PROPERTY_NFC_DISCOVERY_F      6
#define PROPERTY_NFC_DISCOVERY_15693  7
#define PROPERTY_NFC_DISCOVERY_NCFIP  8

/* Name strings for target types. These *must* match the values in TagTechnology.java */
#define TARGET_TYPE_UNKNOWN          -1
#define TARGET_TYPE_ISO14443_3A      1
#define TARGET_TYPE_ISO14443_3B      2
#define TARGET_TYPE_ISO14443_4       3
#define TARGET_TYPE_FELICA           4
#define TARGET_TYPE_ISO15693         5
#define TARGET_TYPE_NDEF             6
#define TARGET_TYPE_NDEF_FORMATABLE  7
#define TARGET_TYPE_MIFARE_CLASSIC   8
#define TARGET_TYPE_MIFARE_UL        9

/* Pre-defined card read/write state values. These must match the values in
* Ndef.java in the framework.
*/

#define NDEF_UNKNOWN_TYPE        -1
#define NDEF_TYPE1_TAG           1
#define NDEF_TYPE2_TAG           2
#define NDEF_TYPE3_TAG           3
#define NDEF_TYPE4_TAG           4
#define NDEF_MIFARE_CLASSIC_TAG  101

/* Pre-defined tag type values. These must match the values in
* Ndef.java in the framework.
*/

#define NDEF_MODE_READ_ONLY   1
#define NDEF_MODE_READ_WRITE  2
#define NDEF_MODE_UNKNOWN     3

#define MAX_SE_COUNT                    2
#define UDS_EXPECTED_LENGTH             17


#define QUIRK_SE_ALWAYS_VIRTUAL       1     // Not used any more.
#define QUIRK_NFC_ON_AFTER_SHUTDOWN   2     // 
#define QUIRK_UICC_ENABLE_LOWBAT      4     // whether CE is active in PowerOff(Bat good) state.
#define QUIRK_UICC_ENABLE_SCREENOFF   8     // whether CE is active in ScreenLocked and ScreenOff state 
#define QUIRK_NFC_EXTRAS_ROUTING      16    // whether NfcExtras.SetRouting is active. 
#define QUIRK_NFC_DISABLE_106K_P2P    32    // disable p2p 106 kbit mode
#define QUIRK_ST21_BLACKLIST_FIX      64    // do blacklist workaround for st21nfca 3.8b4 firmware

typedef enum tagNfcNotification
{
  nfcNotificationQuit          = 0xabc0,
  nfcNotificationTagInsert     = 0xabc1,
  nfcNotificationTagRemove     = 0xabc2,
  nfcNotificationTransaction   = 0xabc3,
  nfcNotificationP2PActivation = 0xabc4,
  nfcNotificationFieldOn       = 0xabc5,
  nfcNotificationFieldOff      = 0xabc6
} TNfcNotification;



typedef struct tagLlcpServiceSocket
{
  TSocketListNode listNode;
  jobject         object;
  HANDLE          hConnection;                // data-connection handle (or null if this is a dummy socket)
  HANDLE          hServer;                    // Indicates, this is a listen server socket
  
  // local values
  int             localRw;
  int             localMiu;
  int             localSap;
  
  // remote values
  int             rw;
  int             miu;
  int             sap;

  // queue for incomming data:
  RingBuffer     *rxBuffer;
} TLlcpSocket, *PLlcpSocket;

typedef struct tagNfcService
{
  pthread_t       hThread;               // message thread handle
  sem_t           hSignalSemaphore;      // Signals new notification for processing
  sem_t           hWaitSemaphore;        // Signals processing finished
  pthread_mutex_t hSendGuard;            // Mutex protecting send-msg function

  DWORD           oemOptions;            // quirks/flags to control the behaviour of the stack:
  bool            discoveryIsEnabled;

  // data that is shared bewtween the message-thread
  // and any other nfc-threads lives in this structure:
  struct
  {
    TNfcNotification event;             // event to be sent to message-thread
    void            *param;             // optional parameter (llcp data etc)
    int              size;              // optional parameter size argument.
  }               threadShared;

  JavaVM         *vm;                    // Our VM reference
  int             env_version;           // java environment version

  /* LLCP params */
  int             lto,miu,wks,opt;

  jobject         tag;                    // Detected tag object
  int             lastError;              // last Lib status

  jobject         objectManager;          // Reference to the NFCManager instance
  jobject         objectNfcTag;           // Cached object to NFC tag
  jobject         objectP2pDevice;        // Cached object to P2P device

  jobject         lastSignaledTag;
  jobject         lastSignaledP2PDevice;
  int             transactionCount;
  int             tagDetectedCount;

  /* NFCAPI interface */

  DWORD           supportedListenMask;    // Supported NFCAPI listener mask
  DWORD           listenMask;             // Active NFCAPI listen mask

  int             traceMask;
  int             traceMaskL1;

  PBYTE           pIoBuffer;                  // ptr to data
  size_t          ioBufferSize;               // allocated size of above ptr.


  // *************************************
  //
  //    Secure Element related stuff 
  //
  // *************************************
  
  TNfcInfoSE      seInfo[MAX_SE_COUNT];
  int             seCount;
  TSecureMode     currentSEMode;
  HANDLE          hSEConnection;         // handle to open SE connection (nfcExtras, seek-api ..)

  DWORD           tagType;
  DWORD           previouslyDetectedTagType; 
  HANDLE          hConnectedTag;
  PNfcTagInfo     pTagInfo;
  HANDLE          hListener;
  HANDLE          hTransactionListener;
  HANDLE          hApplication;
  HANDLE          hFieldListener;
  TNfcTagInfo     tagInfo;

  UdsServer      *debugUdsServer;                  // de.stollmann.nfc.debug UDS Interface
  UdsState        debugUdsServerState;             // current connection state.
  UdsServer      *extendUdsServer;                 // server for API extension.
  BYTE            debugUdsBuffer[UDS_EXPECTED_LENGTH + 1];
  int             debugUdsNumBytes;

  TSocketList     sockets;

  bool            isSt21;                    // for st21 specific workarounds.
  bool            isPN544;                   // for pn544s specific workarounds.
} TNfcService, *PNfcService;

extern PNfcService gpService;
extern pthread_mutex_t concurrencyMutex;       // Mutex protecting library against concurrency
extern pthread_mutex_t globalInitMutex;        // mutex around enable/disable 


#define CONCURRENCY_LOCK()    pthread_mutex_lock(&concurrencyMutex);
#define CONCURRENCY_UNLOCK()  pthread_mutex_unlock(&concurrencyMutex);
#define GLOBAL_LOCK()         pthread_mutex_lock(&globalInitMutex);
#define GLOBAL_UNLOCK()       pthread_mutex_unlock(&globalInitMutex);

extern BOOL globalIsInitialized;

namespace android {
bool jniRegisterNativeMethods (JNIEnv                *e,
                               const char            *className,
                               const JNINativeMethod *gMethods,
                               int                    numMethods);

PNfcService nfcGetServiceInstance (JNIEnv *e, jobject o);

int nfc_jni_cache_object (JNIEnv *e, const char *clsname, jobject *cached_obj);

JNIEnv *AquireJniEnv (PNfcService pService);

void *nfcNotificationThread (void *ptr);

NFCSTATUS   nfcLoadLibrary (void);
void        nfcFreeLibrary (void);
void NFCAPI nfcEventHandler (PVOID          pvAppContext,
                             NFCEVENT       dwEvent,
                             PVOID          pvListenerContext,
                             PNfcEventParam pEvent);

bool register_com_android_nfc_NativeNfcManager (JNIEnv *e);
bool register_com_android_nfc_NativeNfcTag (JNIEnv *e);
bool register_com_android_nfc_NativeP2pDevice (JNIEnv *e);
bool register_com_android_nfc_NativeLlcpServiceSocket (JNIEnv *e);
bool register_com_android_nfc_NativeLlcpSocket (JNIEnv *e);
bool register_com_android_nfc_NativeNfcSecureElement (JNIEnv *e);

//
// Signal handler to upper JAVA service instance
//

void nfcRegisterSignalFunctions (JNIEnv *e, jclass cls);
void nfcSignalP2pActivation (PNfcService pService);
void nfcSignalTagDetected (PNfcService pService);
void nfcSignalTargetRemoved (PNfcService pService);
void nfcSignalTransaction (PNfcService pService, PNfcTransactionInd pParam);

void nfcSignalNotification (PNfcService pService, TNfcNotification event, PVOID pvParam, int size);

void nfcSetTargetPollBytes (JNIEnv     *e,
                            jobject     tagObject,
                            PNfcService pService,
                            PNfcTagInfo pTagInfo);
void nfcSetTargetActivationBytes (JNIEnv     *e,
                                  jobject     tagObject,
                                  PNfcService pService,
                                  PNfcTagInfo pTagInfo);

int nfcGetTechnologyTree (PNfcService pService,
                          int         size,
                          int        *pTechList,
                          int        *pHandleList,
                          int        *pTypeList);

int jniCacheObject (JNIEnv *e, const char *pszClassName, jobject *pCachedObject);

jobject jniCreateObjectReference (JNIEnv *env, jobject pClassTemplate);
jobject jniCreateObject (JNIEnv *e, const char *pszClassName);

void jniSetByteArray (JNIEnv *env, jobject obj, jfieldID id, PBYTE pSource, int length);
void jniSetIntArray (JNIEnv *env, jobject obj, jfieldID field, int *pSource, int length);

PBYTE aquireIoBuffer (size_t size);

BOOL jniGetSystemProperty (const char *key, char *value);

PLlcpSocket nfcSocketCreate (JNIEnv *e, const char *pszClassName);
void        nfcSocketDestroy (PLlcpSocket pSocket);
PLlcpSocket nfcSocketGetReference (JNIEnv *e, jobject o);
void        nfcSocketDiscardList (PNfcService pService);

BOOL nfcIsSocketAlive (JNIEnv *env, jobject o);

void onLlcpSessionEstablished (HANDLE handle, PP2PConnect pInfo);
void onLlcpSessionIndication (HANDLE handle, PP2PConnectInd pInfo);
void onLlcpSessionFailed (HANDLE handle, PP2PDisconnect pInfo);
void onLlcpSessionTerminated (HANDLE handle, PP2PDisconnect pInfo);
void onLlcpSessionDisconnected (HANDLE handle, PP2PDisconnect pInfo);
void onLlcpDataTransmitted (HANDLE handle, PP2PData pInfo);
void onLlcpDataIndication (HANDLE handle, PP2PData pInfo);


bool nfcManagerGetReaderEnable (void);
void nfcManagerSetReaderEnable (bool mode);

/* these control temporary test-modes, not to be used in production */
bool nfcManagerGetIp1TargetEnable (void);
void nfcManagerSetIp1TargetEnable (bool mode);
bool nfcManagerGetWlFeatEnable    (void);
void nfcManagerSetWlFeatEnable    (bool mode);


void nfcManagerStartStandardReader (PNfcService pService);


/* clean up the object stored at pService->lastSignaledTag and lastSignaledP2PDevice */
void CleanupLastTag (PNfcService pService, JNIEnv *env);

/* thread save output function for stack output redirection */
void threadsaveOutputProc (const char *string);

/* helper structure to import java-fields */
struct JNIFields
{
  const char *fieldName;
  const char *fieldType;
  jfieldID   *field;
};

/* register an array of field-ids.. */
bool JNIRegisterFields (JNIEnv *e, jclass cls, const JNIFields *fields, size_t numFields);

/* cached fieldID values for NativeLlcpSocket */
struct TNativeSocketFields
{
  jfieldID mHandle;
  jfieldID mSap;
  jfieldID mLocalMiu;
  jfieldID mLocalRW;
};

/* cached fieldID values for NativeLlcpServiceSocket */
struct TNativeServiceSocketFields
{
  jfieldID mHandle;
  jfieldID mLocalLinearBufferLength;
  jfieldID mLocalMiu;
  jfieldID mLocalRW;
};

struct TNativeTagFields
{
  jfieldID mTechList;             /* [I  */
  jfieldID mTechHandles;          /* [I  */
  jfieldID mTechLibNfcTypes;      /* [I  */
  jfieldID mTechPollBytes;        /* [[B */
  jfieldID mTechActBytes;         /* [[B */
  jfieldID mUid;                  /* [B  */
  jfieldID mConnectedHandle;      /* I   */
  jfieldID mIsPresent;            /* B   */
};

struct TNativeP2PDeviceFields
{
  jfieldID mHandle;               /* I */
  jfieldID mMode;                 /* I */
  jfieldID mGeneralBytes;         /* [B */
};

/* cached field-ids */
extern TNativeSocketFields        NativeSocketFields;
extern TNativeServiceSocketFields NativeServiceSocketFields;
extern TNativeTagFields           NativeTagFields;
extern TNativeP2PDeviceFields     NativeP2PDeviceFields;
}      // namespace android


int mapNfcStatusToAndroid(NFCSTATUS status);



#endif // ifndef __COM_ANDROID_NFC_JNI_H__
