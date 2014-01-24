/** ----------------------------------------------------------------------
 * File      : $RCSfile: com_android_nfc.cpp,v $
 * Revision  : $Revision: 1.43.2.3 $
 * Date      : $Date: 2012/09/12 16:46:02 $
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
#include "com_android_nfc_errorcodes.h"
#include <sys/system_properties.h>

pthread_mutex_t debugOutputSem   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t concurrencyMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t globalInitMutex  = PTHREAD_MUTEX_INITIALIZER;

BOOL            globalIsInitialized = FALSE;

/*------------------------------------------------------------------*/
/**
 * @fn     mapNfcStatusToAndroid
 *
 * @brief  maps NFCStatus to Android nfc_errorcodes 
 *
 * @return mapped State
 */
/*------------------------------------------------------------------*/
int mapNfcStatusToAndroid (NFCSTATUS p)
{
	int status = ERROR_IO;
	switch(p)
  	{ // TODO map more codes
    
    case nfcStatusSuccess:
     status = ERROR_SUCCESS;
     break;
     
		case nfcStatusFailed:
		 status = ERROR_IO;
		 break;
		default:
		 status = ERROR_IO;
		 break;
	}
	return status;
}

void PrintAndroidVersion (JNIEnv *env)
{
  jclass   clsVersion;
  jfieldID id;
  bool     GotData = false;

  do
  {
    clsVersion = env->FindClass("android/os/Build$VERSION");
    if (!clsVersion)
      break;

    id = env->GetStaticFieldID(clsVersion, "RELEASE", "Ljava/lang/String;");
    if (!id)
      break;

    jstring obj = (jstring) env->GetStaticObjectField(clsVersion, id);
    if (!obj)
      break;

    const char *string = env->GetStringUTFChars(obj, 0);
    LOGV("Android version is %s\n", string);
    env->ReleaseStringUTFChars(obj, string);

    GotData = true;
  } while (0);

  if (!GotData)
  {
    LOG("! Access android version failed\n");
  }
} /* PrintAndroidVersion */

/*
 * JNI Initialization
 */

jint JNI_OnLoad (JavaVM *jvm, void *reserved)
{
  JNIEnv *e;
  (void) reserved;

  LOG("NFC Service : loading JNI\n");

  // Check JNI version
  if (jvm->GetEnv((void **) &e, JNI_VERSION_1_6))
    return JNI_ERR;

  PrintAndroidVersion(e);

  if (!android::register_com_android_nfc_NativeNfcManager(e))
  {
    LOG("! Register NativeNfcManger failed");
    return JNI_ERR;
  }

  if (!android::register_com_android_nfc_NativeNfcTag(e))
  {
    LOG("! Register NativeNfcTag failed");
    return JNI_ERR;
  }

  if (!android::register_com_android_nfc_NativeP2pDevice(e))
  {
    LOG("! Register NativeP2PDevice failed");
    return JNI_ERR;
  }

  if (!android::register_com_android_nfc_NativeLlcpSocket(e))
  {
    LOG("! Register NativeLlcpSocket failed");
    return JNI_ERR;
  }

  if (!android::register_com_android_nfc_NativeLlcpServiceSocket(e))
  {
    LOG("! Register NativeLlcpServiceSocket failed");
    return JNI_ERR;
  }

  if (!android::register_com_android_nfc_NativeNfcSecureElement(e))
  {
    LOG("! Register NativeNfcSecureElement failed");
    return JNI_ERR;
  }

  return JNI_VERSION_1_6;
} // JNI_OnLoad

namespace android {
extern PNfcService exported_pService;

/* thread save output functions */

void threadsaveOutputProc (const char *string)
{
  pthread_mutex_lock(&::debugOutputSem);
  __android_log_print(ANDROID_LOG_DEBUG, "nfc-stm", string);
  pthread_mutex_unlock(&::debugOutputSem);
}

static int addTechnology (int *techList,
                          int *handleList,
                          int *typeList,
                          int  listSize,
                          int  maxListSize,
                          int  techToAdd,
                          int  handleToAdd,
                          int  typeToAdd)
{
  bool found = false;

  for (int i = 0; i < listSize; i++)
  {
    if (techList[i] == techToAdd)
    {
      found = true;
      break;
    }
  }

  if (!found && listSize < maxListSize)
  {
    techList[listSize]   = techToAdd;
    handleList[listSize] = handleToAdd;
    typeList[listSize]   = typeToAdd;
    return listSize + 1;
  }

  return listSize;
} // addTechnology

int nfcGetTechnologyTree (PNfcService pService,
                          int         size,
                          int        *pTechList,
                          int        *pHandleList,
                          int        *pTypeList)
{
  int index  = 0;
  int handle = 1; // We have no specific support for handles here

  DWORD type = pService->pTagInfo->tagType & NFC_IND_TYPE_TYPE_MASK;

  // TODO later, support for more tags

  switch (type)
  {
    case NFC_IND_TYPE_TAG_ISO14443A:
    case NFC_IND_TYPE_TAG_DESFIRE_TYPE4:
      index = addTechnology(pTechList,
                            pHandleList,
                            pTypeList,
                            index,
                            size,
                            TARGET_TYPE_ISO14443_4,
                            handle,
                            type);
      index = addTechnology(pTechList,
                            pHandleList,
                            pTypeList,
                            index,
                            size,
                            TARGET_TYPE_ISO14443_3A,
                            handle + 1,
                            type);
      break;

    case NFC_IND_TYPE_TAG_14443B:
      index = addTechnology(pTechList,
                            pHandleList,
                            pTypeList,
                            index,
                            size,
                            TARGET_TYPE_ISO14443_4,
                            handle,
                            type);
      index = addTechnology(pTechList,
                            pHandleList,
                            pTypeList,
                            index,
                            size,
                            TARGET_TYPE_ISO14443_3B,
                            handle + 1,
                            type);
      // dumpIso14443B(pService->pTagInfo->tagType);
      break;

    case NFC_IND_TYPE_TAG_15693:
      index = addTechnology(pTechList,
                            pHandleList,
                            pTypeList,
                            index,
                            size,
                            TARGET_TYPE_ISO15693,
                            handle,
                            type);
      break;

    case NFC_IND_TYPE_TAG_MIFARE_STD_1K:
    case NFC_IND_TYPE_TAG_MIFARE_STD_4K:
      index = addTechnology(pTechList,
                            pHandleList,
                            pTypeList,
                            index,
                            size,
                            TARGET_TYPE_MIFARE_CLASSIC,
                            handle,
                            type);
      index = addTechnology(pTechList,
                            pHandleList,
                            pTypeList,
                            index,
                            size,
                            TARGET_TYPE_ISO14443_3A,
                            handle,
                            type);
      break;

    case NFC_IND_TYPE_TAG_ULTRALIGHT_TYPE2:
      index = addTechnology(pTechList,
                            pHandleList,
                            pTypeList,
                            index,
                            size,
                            TARGET_TYPE_MIFARE_UL,
                            handle,
                            type);
      index = addTechnology(pTechList,
                            pHandleList,
                            pTypeList,
                            index,
                            size,
                            TARGET_TYPE_ISO14443_3A,
                            handle,
                            type);
      break;

    case NFC_IND_TYPE_TAG_FELICA_TYPE3:
      index = addTechnology(pTechList,
                            pHandleList,
                            pTypeList,
                            index,
                            size,
                            TARGET_TYPE_FELICA,
                            handle,
                            type);
      break;

    case NFC_IND_TYPE_TAG_JEWEL_TYPE1:
      index = addTechnology(pTechList,
                            pHandleList,
                            pTypeList,
                            index,
                            size,
                            TARGET_TYPE_ISO14443_3A,
                            handle,
                            type);
      break;

    default:
      index = addTechnology(pTechList,
                            pHandleList,
                            pTypeList,
                            index,
                            size,
                            TARGET_TYPE_UNKNOWN,
                            handle,
                            type);
      break;
  } // switch

  return index;
} // nfcGetTechnologyTree


PLlcpSocket nfcSocketCreate (JNIEnv *e, const char *pszClassName)
{
  PLlcpSocket pSocket = NULL;

  do
  {
    pSocket = (PLlcpSocket) calloc(sizeof(*pSocket), 1);
    if (!pSocket)
    {
      LOGE("Cannot alloc LLCP service socket for class <%s>", pszClassName);
      break;
    }

    LOGV("New native socket %p for class <%s> created", pSocket, pszClassName);

    socketNodeInit(&pSocket->listNode);

    /* Create new NativeLlcpServiceSocket object */
    pSocket->object = jniCreateObject(e, pszClassName);

    if (!pSocket->object)
    {
      LOG("Llcp Socket object creation error");
      free(pSocket);
      pSocket = NULL;
      break;
    }
  } while (FALSE);

  return pSocket;
} // nfcSocketCreate

void nfcSocketDestroy (PLlcpSocket pSocket)
{
  LOG("enter nfcSocketDestroy()");

  if (pSocket->object)
  {
    // free socket java object (if still exists)
    JNIEnv *env = AquireJniEnv(gpService);

    // set java object mHandle to zero. mHandle is checked in all
    // native member functions to be != zero before any further
    // checks are done. This protects us from calls into our code
    // by leaking or late java-objects.

    jclass   c = env->GetObjectClass(pSocket->object);
    jfieldID f = env->GetFieldID(c, "mHandle", "I");
    env->SetIntField(pSocket->object, f, 0);
    env->DeleteLocalRef(c);

    env->DeleteGlobalRef(pSocket->object);

    pSocket->object = 0;
  }
  
  // free ring-buffer; 
  if (pSocket->rxBuffer)
  {
    delete pSocket->rxBuffer;
    pSocket->rxBuffer = 0;
  }

  free(pSocket);

  LOG("exit nfcSocketDestroy()");
} // nfcSocketDestroy

/*------------------------------------------------------------------*/
/**
 * @fn    nfcSocketDiscardList
 *
 * @brief Remove all objects from socket list and free allocated
 *        memory
 *
 * @param PNfcService pService
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void nfcSocketDiscardList (PNfcService pService)
{
  LOG("nfcSocketDiscardList enter");
  socketListAquire(&pService->sockets);

  int numSockets = 0;

  // for all sockets that exist:
  while (pService->sockets.root)
  {
    PLlcpSocket sock = (PLlcpSocket) pService->sockets.root;

    // unblock all waiting threads:
    socketListInterrupt(&pService->sockets, &sock->listNode);

    // remove from list:
    if (socketListRemove(&pService->sockets, &sock->listNode))
    {
      // free resources
      nfcSocketDestroy(sock);
    }
    numSockets++;
  }

  socketListRelease(&pService->sockets);
  LOGV("nfcSocketDiscardList exit, %d sockets removed", numSockets);
} // nfcSocketDiscardList

/*
 * JNI Utils
 */
int jniCacheObject (JNIEnv *e, const char *pszClassName, jobject *pCachedObject)
{
  jclass    cls;
  jobject   obj;
  jmethodID ctor;

  cls = e->FindClass(pszClassName);
  if (cls == NULL)
  {
    LOGE("! Find class <%s> error", pszClassName);
    return -1;
  }

  ctor = e->GetMethodID(cls, "<init>", "()V");
  obj  = e->NewObject(cls, ctor);
  if (obj == NULL)
  {
    LOGV("! Create object for class <%s> failed", pszClassName);
    return -1;
  }

  *pCachedObject = e->NewGlobalRef(obj);
  if (*pCachedObject == NULL)
  {
    LOGE("Global ref for class <%s> failed", pszClassName);
    e->DeleteLocalRef(obj);
    return -1;
  }

  e->DeleteLocalRef(obj);

  return 0;
} // jniCacheObject

jobject jniCreateObjectReference (JNIEnv *env, jobject pClassTemplate)
{
  LOGV("jniCreateObjectReference for env=%p and template %p", env, pClassTemplate);

  jobject obj;

  jclass objClass = env->GetObjectClass(pClassTemplate);

  if (env->ExceptionCheck())
  {
    LOG("! Get Object Class Error");
    // kill_client(pService);
    env->DeleteLocalRef(objClass);
    return 0;
  }

  //
  // Create new target instance
  //
  obj = env->NewObject(objClass, env->GetMethodID(objClass, "<init>", "()V"));

  // smells, but...
  jobject objRef = env->NewGlobalRef(obj);
  env->DeleteLocalRef(obj);
  env->DeleteLocalRef(objClass);

  return objRef;
} // jniCreateObjectReference

jobject jniCreateObject (JNIEnv *e, const char *pszClassName)
{
  jobject obj;

  jclass objClass = e->FindClass(pszClassName);

  if (!objClass)
  {
    LOGE("Find class failed for <%s>", pszClassName);
    return 0;
  }

  obj = e->NewObject(objClass, e->GetMethodID(objClass, "<init>", "()V"));
  if (!obj)
  {
    LOGE("Create object <%s> failed", pszClassName);
    e->DeleteLocalRef(objClass);
    return 0;
  }

  jobject refObj = e->NewGlobalRef(obj);
  LOGV("new JAVA object %p created, cached reference %p", obj, refObj);
  e->DeleteLocalRef(obj);
  e->DeleteLocalRef(objClass);

  return refObj;
} // jniCreateObject

void jniSetByteArray (JNIEnv *env, jobject obj, jfieldID id, PBYTE pSource, int length)
{
  jbyteArray jArray = env->NewByteArray(length);

  if (length)
  {
    env->SetByteArrayRegion(jArray, 0, length, (jbyte *) pSource);
  }
  else
  {
    LOGV("Unable to create Byte-Array of size %d\n", length);
  }

  env->SetObjectField(obj, id, jArray);
  env->DeleteLocalRef(jArray);
} // jniSetByteArray

/*------------------------------------------------------------------*/
/**
 * @fn    jniSetIntArray
 *
 * @brief
 *
 * @param PJniObject pObject
 * @param jfieldID field
 * @param int * pSource
 * @param int length
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void jniSetIntArray (JNIEnv *env, jobject obj, jfieldID field, int *pSource, int length)
{
  jintArray arrayList;
  jint     *jItem;

  arrayList = env->NewIntArray(length);
  if (arrayList)
  {
    jItem = env->GetIntArrayElements(arrayList, NULL);

    for (int i = 0; i < length; i++)
    {
      jItem[i] = pSource[i];
    }

    env->SetObjectField(obj, field, arrayList);
    env->ReleaseIntArrayElements(arrayList, jItem, 0);
    env->DeleteLocalRef(arrayList);
  }
  else
  {
    LOGV("Unable to create Int-Array of size %d\n", length);
  }
} // jniSetIntArray

/*------------------------------------------------------------------*/
/**
 * @fn    nfcSocketGetReference
 *
 * @brief Get own socket object for given JAVA reference. We assumes
 *        this JAVA object contains field with name 'mHandle'
 *
 * @param JNIEnv * e
 * @param jobject o
 *
 *
 * @return PLlcpSocket
 */
/*------------------------------------------------------------------*/
PLlcpSocket nfcSocketGetReference (JNIEnv *e, jobject o)
{
  jclass      c       = e->GetObjectClass(o);
  jfieldID    f       = e->GetFieldID(c, "mHandle", "I");
  PLlcpSocket pSocket = (PLlcpSocket) e->GetIntField(o, f);

  // Validate socket. This socket must exits in our list
  if (!socketListExists(&gpService->sockets, &pSocket->listNode))
  {
    LOGE("! Socket object %p not in socket list", pSocket);
    pSocket = NULL;
  }

  if (pSocket)
  {
    // aquire ownership
    if (!socketNodeAquire(&pSocket->listNode))
    {
      // failed - someone already interrupting on that socket.
      pSocket = NULL;
    }
  }

  return pSocket;
} // nfcSocketGetReference

PNfcService nfcGetServiceInstance (JNIEnv *e, jobject o)
{
  jclass   c;
  jfieldID f;

  /* Retrieve pNative structure address */
  c = e->GetObjectClass(o);
  f = e->GetFieldID(c, "mNative", "I");
  e->DeleteLocalRef(c);
  return (PNfcService) e->GetIntField(o, f);
}

/*------------------------------------------------------------------*/
/**
 * @fn    JNIRegisterFields
 *
 * @brief register multiple java fields. this is what JNIEnv::RegisterNatives
 *        does, but for members instead of functions.
 *
 * @param
 *
 * @return false if anything fails
 */
/*------------------------------------------------------------------*/
bool JNIRegisterFields (JNIEnv *e, jclass cls, const JNIFields *fields, size_t numFields)
{
  for (size_t i = 0; i < numFields; i++)
  {
    jfieldID id = e->GetFieldID(cls, fields[i].fieldName, fields[i].fieldType);
    //LOGV("register field %s, status = %d", fields[i].fieldName, id != 0);
    
    *fields[i].field = id;
    if (!*fields[i].field)
    {
      return false;
    }
  }
  return true;
} // JNIRegisterFields

/*------------------------------------------------------------------*/
/**
 * @fn    jniGetSystemProperty
 *
 * @brief get system propperty from Android. Since cutils/properties.h
 *        is not part of the NDK environment we load the library directly
 *        and call it. this will work as long as the prototype of property_get does
 *        not change.
 *
 *
 * @param PNFCMSG pMessage Message descriptor that contains all data for processing.
 *
 * @return void
 */
/*------------------------------------------------------------------*/

BOOL jniGetSystemProperty (const char *key, char *value)
{
  typedef int (*func)(const char *key,
                      const char *value,
                      const char *defaultvalue);

  BOOL  result = false;
  void *lib    = dlopen("libcutils.so", RTLD_NOW);
  *value = 0;

  if (lib)
  {
    func getprop = (func) dlsym(lib, "property_get");
    if (getprop)
    {
      getprop(key, value, "unknown");
      result = true;
    }
    else
    {
      LOG("!!! unable to get entry-point property_get in libcutils.so");
    }
    dlclose(lib);
  }
  else
  {
    LOG("!!! unable top open library libcutils.so");
  }

  return result;
} // jniGetSystemProperty

/*------------------------------------------------------------------*/
/**
 * @fn    aquireIoBuffer
 *
 * @brief aquire exclusive access to the global io-buffer managed by the
 *        service-layer. Note: The buffer has a guaranteed size of 64kb,
 *        so for most NFC calls we can assume that allocation never fails.
 *
 * @param size_t  size  minimal size of the buffer.
 *
 * @return ptr to memory or NULL.
 */
/*------------------------------------------------------------------*/
PBYTE aquireIoBuffer (size_t size)
{
  if (gpService->ioBufferSize < size)
  {
    // reallocate
    PBYTE temp = (PBYTE) malloc(size);
    if (temp)
    {
      LOGV("reallocated shared IO-buffer to %d bytes", size);
      free(gpService->pIoBuffer);
      gpService->pIoBuffer    = temp;
      gpService->ioBufferSize = size;
    }
    else
    {
      LOGE("reallocation of shared IO-buffer for %d bytes failed", size);
      return NULL;
    }
  }

  return gpService->pIoBuffer;
} // aquireIoBuffer

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/

bool jniRegisterNativeMethods (JNIEnv                *env,
                               const char            *className,
                               const JNINativeMethod *gMethods,
                               int                    numMethods)
{
  bool succ = true;

  for (int i = 0; i < numMethods; i++)
  {
    //LOGV("Register method %s, cls=%s", gMethods[i].name, className);
    succ = (env->RegisterNatives(env->FindClass(className), &gMethods[i], 1) == 0);
    if (!succ)
      break;
  }
  return succ;
} /* jniRegisterNativeMethods */


/*------------------------------------------------------------------*/
/**
 * @fn    CleanupLastTag
 *
 * @brief Removes the instance stored at pService->lastSignaledTag.
 *
 *
 * @param pService      nfcService instance
 * @param env           current java environment
 *
 * @return -
 */
/*------------------------------------------------------------------*/
void CleanupLastTag (PNfcService pService, JNIEnv *env)
{
  if (pService->lastSignaledTag)
  {
    env->SetIntField(pService->lastSignaledTag, NativeTagFields.mConnectedHandle, -1);
    env->SetIntField(pService->lastSignaledTag, NativeTagFields.mIsPresent,        0);
    env->DeleteGlobalRef(pService->lastSignaledTag);
    pService->lastSignaledTag = 0;
    pService->pTagInfo->tagType = 0;
  }
  if (pService->lastSignaledP2PDevice)
  {
    env->SetIntField(pService->lastSignaledP2PDevice, NativeP2PDeviceFields.mHandle, -1);
    env->DeleteGlobalRef(pService->lastSignaledP2PDevice);
    pService->lastSignaledP2PDevice = 0;
  }
} /* CleanupLastTag */

/*------------------------------------------------------------------*/
/**
 * @fn    nfcIsSocketAlive
 *
 * @brief Checks if the object o is alive. This function must only
 *        be called on NativeLlcpServiceSocket and NativeLlcpSocket
 *        objects (won't be checked).
 *
 *        The test itself is done by reading out the mHandle member.
 *
 * @param env           current java environment
 * @param o             object to test-
 *
 * @return -
 */
/*------------------------------------------------------------------*/
BOOL nfcIsSocketAlive (JNIEnv *env, jobject o)
{
  int      handle = 0;
  jclass   c      = env->GetObjectClass(o);
  jfieldID f      = env->GetFieldID(c, "mHandle", "I");

  if (f != NULL)
  {
    handle = env->GetIntField(o, f);
  }
  else
  {
    LOGV("Warning: java-object %p does not has a member mHandle", o);
  }

  env->DeleteLocalRef(c);
  return (handle != 0);
} /* nfcIsSocketAlive */
} // namespace android
