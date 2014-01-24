/** ----------------------------------------------------------------------
* File      : $RCSfile: com_android_nfc_NativeLlcpServiceSocket.cpp,v $
* Revision  : $Revision: 1.21 $
* Date      : $Date: 2012/04/05 15:51:31 $
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

#include "com_android_nfc.h"
#include "nfcInit.h"

namespace android {
  

/*------------------------------------------------------------------*/
/**
 * @fn    com_NativeLlcpServiceSocket_doAccept
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM
 * @param jobject o
 * @param jint miu
 * @param jint rw
 * @param jint linearBufferLength
 *
 *
 * @return jobject
 */
/*------------------------------------------------------------------*/
static jobject com_NativeLlcpServiceSocket_doAccept (JNIEnv *e,
                                                     jobject o,
                                                     jint    miu,
                                                     jint    rw,
                                                     jint    linearBufferLength)
{
  PLlcpSocket pServerSocket;
  jobject     clientSocket = NULL;
  
  if (!nfcIsSocketAlive(e, o))
  {
    LOG("NativeLlcpServiceSocket_doAccept() Object already destroyed\n");
    return 0;
  }

  //
  // Retrieve server socket reference
  //
  socketListAquire(&gpService->sockets);

  pServerSocket = nfcSocketGetReference(e, o);

  LOGV("NativeLlcpServiceSocket_doAccept() pSocket %p, miu %d, rw %d, bufferLength %d",
       pServerSocket,
       miu,
       rw,
       linearBufferLength);

  do
  {
    PLlcpSocket pSocket        = NULL;
    NFCSTATUS   status;
    HANDLE      dataConnection = NULL;

    if (!pServerSocket)
    {
      break;
    }

    LOGV("Wait for LLCP connect indication, new server handle %p ...", pServerSocket->hServer);

    int stat = socketListSuspend(&gpService->sockets,
                                 &pServerSocket->listNode,
                                 WAITEVENT_CONNECT | WAITEVENT_CLOSE);

    if (stat == -1)
    {
      LOG("LLCP listen failed - socket interrupt in progress");
    }
    else
    {
      if (pServerSocket->listNode.event & WAITEVENT_CLOSE)
      {
        LOG("LLCP accept got close event. Generate a dummy connection..");

        // note: don't remove WAITEVENT_CLOSE, other threads may
        // be waiting on it as well.
        // break;
      }
      if (pServerSocket->listNode.event & WAITEVENT_CONNECT)
      {
        LOG("LLCP accept got connect event. ");
        pServerSocket->listNode.event &= ~WAITEVENT_CONNECT;
        dataConnection = pServerSocket->hConnection;
      }
    }

    if (!pServerSocket->hConnection)
    {
      LOG("due to previous error doAccept will generate a dummy socket");
    }

    //
    // Create new client socket.
    //

    pSocket = nfcSocketCreate(e, NATIVE_SOCKET_CLASS);
    if (!pSocket)
    {
      break;
    }

    pSocket->rxBuffer = new RingBuffer();
    if (!pSocket->rxBuffer)
    {
      LOG("! RingBuffer allocation failed");
      break;
    }
    
    // space estimate taken from open-source stack
    size_t bufferSpace = miu*(rw+1)+linearBufferLength;
    
    if (!pSocket->rxBuffer->allocate(bufferSpace))
    {
      LOG("! Ringbuffer allocation failed");
      break;
    }
    LOGV("RingBuffer for socket allocated with %d bytes\n", bufferSpace);
    
    //
    // Register last session parameter
    //

    pSocket->miu               = pServerSocket->miu;
    pSocket->hConnection       = dataConnection;
    pSocket->rw                = pServerSocket->rw;
    pSocket->sap               = pServerSocket->sap;
    pSocket->localMiu          = miu;
    pSocket->localRw           = rw;
    pSocket->localSap          = pServerSocket->localSap;

    pServerSocket->hConnection = NULL;

    //
    // Register event handler and new context for new connection
    //

    if (pSocket->hConnection)
    {
      status = pNfcP2PConnectResponse(pSocket->hConnection, pSocket, nfcEventHandler);

      if (status != nfcStatusSuccess)
      {
        break;
      }
    }

    /* Get NativeConnectionOriented class object */
    if (e->ExceptionCheck())
    {
      LOGE("! LLCP Socket get class <%s> object error", NATIVE_SOCKET_CLASS);
      break;
    }

    clientSocket = pSocket->object;

    e->SetIntField(clientSocket, NativeSocketFields.mHandle,   (jint) pSocket);
    e->SetIntField(clientSocket, NativeSocketFields.mLocalMiu, (jint) miu);
    e->SetIntField(clientSocket, NativeSocketFields.mLocalRW,  (jint) rw);
    
    // create rx-buffer for the socket */

    // add to socket list..
    socketListAdd(&gpService->sockets, &pSocket->listNode);
  } while (FALSE);

  LOGV("NativeLlcpServiceSocket_doAccept() exit %s", clientSocket ? "SUCCESS" : "FAILED");

  if (pServerSocket)
  {
    socketNodeRelease(&pServerSocket->listNode, &gpService->sockets);
  }

  socketListRelease(&gpService->sockets);

  return clientSocket;
} // com_NativeLlcpServiceSocket_doAccept

/*------------------------------------------------------------------*/
/**
 * @fn    com_NativeLlcpServiceSocket_doClose
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM
 * @param jobject o
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_NativeLlcpServiceSocket_doClose (JNIEnv *e, jobject o)
{
  jboolean    jResult = FALSE;
  PLlcpSocket pSocket;

  if (!nfcIsSocketAlive(e, o))
  {
    LOG("NativeLlcpServiceSocket_doClose() Object already destroyed\n");
    return true;
  }

  //
  // Retrieve socket reference
  //

  // only one thread may perform close-calls due to race-conditions
  // in the upper layer java-code
  LOGV("NativeLlcpServiceSocket_doClose() %d ENTER-GLOBAL-CLOSELOCK", (unsigned int)pthread_self());

  pthread_mutex_lock(&gpService->sockets.CloseLock);

  socketListAquire(&gpService->sockets);

  pSocket = nfcSocketGetReference(e, o);

  LOGV("NativeLlcpServiceSocket_doClose() pSocket=%p enter", pSocket);

  if (pSocket)
  {
    if (pSocket->hServer)
    {
      NFCSTATUS status;

      status = pNfcP2PDisconnectRequest(pSocket->hServer);

      if (status == nfcStatusSuccess)
      {
        LOG("Wait until onLlcpSessionDisconnected() ");

        int stat = socketListSuspend(&gpService->sockets, &pSocket->listNode, WAITEVENT_CLOSE);

        if (stat == 0)
        {
          // don't remove the close event, other threads
          // may be waiting on it as well.
        }
        else
        {
          LOG("LLCP doClose interrupted");
        }
      }
    }
    else
    {
      LOG("No hServer handle, no action required");
    }

    jResult = TRUE;

    socketNodeRelease(&pSocket->listNode, &gpService->sockets);

    // unblock all other waiting threads:
    socketListInterrupt(&gpService->sockets, &pSocket->listNode);

    // remove from list
    if (socketListRemove(&gpService->sockets, &pSocket->listNode))
    {
      // free resources.
      nfcSocketDestroy(pSocket);
    }
  }

  socketListRelease(&gpService->sockets);

  LOG("NativeLlcpServiceSocket_doClose() exit");

  // only one thread may perform close-calls due to race-conditions
  // in the upper layer java-code
  pthread_mutex_unlock(&gpService->sockets.CloseLock);

  LOGV("NativeLlcpServiceSocket_doClose() %d LEAVE-GLOBAL-CLOSELOCK", (unsigned int)pthread_self());

  return jResult;
} // com_NativeLlcpServiceSocket_doClose

/*
 * JNI registration.
 */
TNativeServiceSocketFields NativeServiceSocketFields;

static const JNINativeMethod gMethods[] = 
{
  {"doAccept", "(III)L"NATIVE_SOCKET_CLASS";",
   (void *) com_NativeLlcpServiceSocket_doAccept},
  {"doClose",  "()Z",
   (void *) com_NativeLlcpServiceSocket_doClose},
};

static const JNIFields gFields[] = {
  {"mHandle",                  "I", &NativeServiceSocketFields.mHandle                 },
  {"mLocalLinearBufferLength", "I", &NativeServiceSocketFields.mLocalLinearBufferLength},
  {"mLocalMiu",                "I", &NativeServiceSocketFields.mLocalMiu               },
  {"mLocalRw",                 "I", &NativeServiceSocketFields.mLocalRW                },
};

bool register_com_android_nfc_NativeLlcpServiceSocket (JNIEnv *e)
{
  jclass cls = e->FindClass(NATIVE_SERVER_SOCKET_CLASS);

  return JNIRegisterFields(e, cls, gFields, NELEM(gFields)) &&
         e->RegisterNatives(cls, gMethods, NELEM(gMethods)) == 0;
}
} // namespace android
