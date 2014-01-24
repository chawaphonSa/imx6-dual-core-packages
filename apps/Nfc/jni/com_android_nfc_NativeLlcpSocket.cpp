/** ----------------------------------------------------------------------
* File      : $RCSfile: com_android_nfc_NativeLlcpSocket.cpp,v $
* Revision  : $Revision: 1.28 $
* Date      : $Date: 2012/04/05 13:56:24 $
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
#include <unistd.h>

namespace android {

/*------------------------------------------------------------------*/
/**
 * @fn    onLlcpSessionEstablished
 *
 * @brief  Indication from NFC stack, a new LLCP session is established
 *
 * @param HANDLE handle
 * @param PP2PConnect pInfo Contains new session information
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void onLlcpSessionEstablished (HANDLE handle, PP2PConnect pInfo)
{
  PLlcpSocket pSocket;

  LOGV("onLlcpSessionEstablished() P2P session established %p, handle %p",
       pInfo->pvConnectionContext,
       handle);

  socketListAquire(&gpService->sockets);

  pSocket = (PLlcpSocket) pInfo->pvConnectionContext;

  // Validate in socketList:
  if (!socketNodeAquire(&pSocket->listNode))
  {
    pSocket = 0;
    LOG("llcpSessionEstablished: socket-handle not in socketlist/blocked by shutdown.");
  }

  if (pSocket)
  {
    pSocket->miu      = pInfo->blockSize;
    pSocket->rw       = pInfo->windowSize;
    pSocket->localSap = pInfo->localSap;

    LOGV("Socket %p, hConnection %p, miu=%d, rw=%d", pSocket, pSocket->hConnection,
         pSocket->miu, pSocket->rw);

    socketListEvent(&gpService->sockets, &pSocket->listNode, WAITEVENT_CONNECT);
    socketNodeRelease(&pSocket->listNode, &gpService->sockets);
  }

  socketListRelease(&gpService->sockets);
} // onLlcpSessionEstablished

/*------------------------------------------------------------------*/
/**
 * @fn    onLlcpSessionIndication
 *
 * @brief Incoming LLCP session.
 *
 * @param HANDLE handle        - Pointer to service socket instance
 * @param PP2PConnectInd pInfo - Pointer to LLCP session information
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void onLlcpSessionIndication (HANDLE handle, PP2PConnectInd pInfo)
{
  PLlcpSocket pServerSocket;

  LOGV("onLlcpSessionIndication() P2P incoming hConnection %p, handle %p",
       pInfo->hConnection,
       handle);

  pServerSocket = (PLlcpSocket) handle;

  socketListAquire(&gpService->sockets);

  // Validate in socketList:
  if (!socketNodeAquire(&pServerSocket->listNode))
  {
    pServerSocket = 0;
    LOG("onLlcpSessionIndication: socket-handle not in socketlist/blocked by shutdown.");
  }

  if (pServerSocket)
  {
    if (pServerSocket->hServer == 0)
    {
      LOG("onLlcpSessionIndication: got connect on non-service socket????");
    }

    // store data from incomming connection in server socket structure..
    LOGV("incomming connection for service socket %p\n", pServerSocket);
    LOGV("  pInfo->hConnection = %p\n", pInfo->hConnection);
    LOGV("  pInfo->blockSize   = %d\n", pInfo->blockSize);
    LOGV("  pInfo->windowSize  = %d\n", pInfo->windowSize);
    LOGV("  pInfo->dSap        = %d\n", pInfo->dSap);
    LOGV("  pInfo->localSap    = %d\n", pInfo->localSap);

    pServerSocket->hConnection = pInfo->hConnection;
    pServerSocket->miu         = pInfo->blockSize;
    pServerSocket->rw          = pInfo->windowSize;
    pServerSocket->sap         = pInfo->dSap;
    pServerSocket->localSap    = pInfo->localSap;
    
    LOGV("send connect event, waiting for server socket  %p to respond ", pServerSocket->hServer);
    socketListEvent(&gpService->sockets, &pServerSocket->listNode, WAITEVENT_CONNECT);

    socketNodeRelease(&pServerSocket->listNode, &gpService->sockets);
  }

  socketListRelease(&gpService->sockets);
} // onLlcpSessionIndication

/*------------------------------------------------------------------*/
/**
 * @fn    onLlcpSessionFailed
 *
 * @brief
 *
 * @param HANDLE handle
 * @param PP2PDisconnect pInfo
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void onLlcpSessionFailed (HANDLE handle, PP2PDisconnect pInfo)
{
  PLlcpSocket pSocket;

  LOGV("onLlcpSessionFailed() P2P session Failed %08x, handle %p context %p",
       (unsigned int)pInfo->cause,
       handle,
       pInfo->pvConnectionContext);

  pSocket = (PLlcpSocket) pInfo->pvConnectionContext;

  socketListAquire(&gpService->sockets);

  // Validate in socketList:
  if (!socketNodeAquire(&pSocket->listNode))
  {
    pSocket = 0;
    LOG("onLlcpSessionFailed: socket-handle not in socketlist/blocked by shutdown.");
  }

  if (pSocket)
  {
    socketListEvent(&gpService->sockets, &pSocket->listNode, WAITEVENT_CONNECTFAILED);
    socketNodeRelease(&pSocket->listNode, &gpService->sockets);
  }

  socketListRelease(&gpService->sockets);
} // onLlcpSessionFailed

/*------------------------------------------------------------------*/
/**
 * @fn    onLlcpSessionTerminated
 *
 * @brief
 *
 * @param HANDLE handle
 * @param PP2PDisconnect pInfo
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void onLlcpSessionTerminated (HANDLE handle, PP2PDisconnect pInfo)
{
  PLlcpSocket pSocket;
  
  (void) handle;

  LOGV("onLlcpSessionTerminated() P2P session terminated, context %p", pInfo->pvConnectionContext);

  socketListAquire(&gpService->sockets);

  pSocket = (PLlcpSocket) pInfo->pvConnectionContext;

  // Validate in socketList:
  if (!socketNodeAquire(&pSocket->listNode))
  {
    pSocket = 0;
    LOG("onLlcpSessionTerminated: socket-handle not in socketlist/blocked by shutdown.");
  }

  if (pSocket)
  {
    socketListEvent(&gpService->sockets, &pSocket->listNode, WAITEVENT_CLOSE);
    socketNodeRelease(&pSocket->listNode, &gpService->sockets);
  }

  socketListRelease(&gpService->sockets);
} // onLlcpSessionTerminated

/*------------------------------------------------------------------*/
/**
 * @fn    onLlcpSessionDisconnected
 *
 * @brief
 *
 * @param HANDLE handle
 * @param PP2PDisconnect pInfo
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void onLlcpSessionDisconnected (HANDLE handle, PP2PDisconnect pInfo)
{
  PLlcpSocket pSocket;
  (void) handle;

  LOGV("onLlcpSessionDisconnected() P2P session disconnected cause %08x, context %p",
       (unsigned int)pInfo->cause,
       pInfo->pvConnectionContext);

  socketListAquire(&gpService->sockets);

  pSocket = (PLlcpSocket) pInfo->pvConnectionContext;

  // Validate in socketList:
  if (!socketNodeAquire(&pSocket->listNode))
  {
    pSocket = 0;
    LOG("onLlcpSessionDisconnected: socket-handle not in socketlist/blocked by shutdown.");
  }

  if (pSocket)
  {
    //
    // OK socket not longer valid
    //
    pSocket->hConnection = NULL;
    socketListEvent(&gpService->sockets, &pSocket->listNode, WAITEVENT_CLOSE);
    socketNodeRelease(&pSocket->listNode, &gpService->sockets);
  }

  socketListRelease(&gpService->sockets);
} // onLlcpSessionDisconnected

/*------------------------------------------------------------------*/
/**
 * @fn    onLlcpDataTransmitted
 *
 * @brief
 *
 * @param HANDLE handle
 * @param PP2PData pInfo
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void onLlcpDataTransmitted (HANDLE handle, PP2PData pInfo)
{
  LOGV("onLlcpDataTransmitted() context %p", pInfo->pvConnectionContext);

  PLlcpSocket pSocket;

  socketListAquire(&gpService->sockets);

  pSocket = (PLlcpSocket) pInfo->pvConnectionContext;

  // Validate in socketList:
  if (!socketNodeAquire(&pSocket->listNode))
  {
    pSocket = 0;
    LOG("onLlcpDataTransmitted: socket-handle not in socketlist/blocked by shutdown.");
  }

  if (pSocket)
  {
    socketListEvent(&gpService->sockets, &pSocket->listNode, WAITEVENT_WRITE);
    socketNodeRelease(&pSocket->listNode, &gpService->sockets);
  }
  socketListRelease(&gpService->sockets);

  (void) handle;
} // onLlcpDataTransmitted

/*------------------------------------------------------------------*/
/**
 * @fn    onLlcpDataIndication
 *
 * @brief
 *
 * @param HANDLE handle
 * @param PP2PData pInfo
 *
 *
 * @return void
 */
/*------------------------------------------------------------------*/
void onLlcpDataIndication (HANDLE handle, PP2PData pInfo)
{
  PLlcpSocket pSocket;

  socketListAquire(&gpService->sockets);

  pSocket = (PLlcpSocket) pInfo->pvConnectionContext;

  // Validate in socketList:
  if (!socketNodeAquire(&pSocket->listNode))
  {
    pSocket = 0;
    LOG("onLlcpDataIndication: socket-handle not in socketlist/blocked by shutdown.");
  }

  if (pSocket)
  {
    //
    // Register incoming data here.
    //
    BOOL succ = pSocket->rxBuffer->write (pInfo->pData, pInfo->dataLength);
    pNfcP2PDataResponse(pSocket->hConnection);

    if (!succ)
    {
      LOG ("! rx-buffer overflow!\n")
      // TODO: handle this somehow
    }

    socketListEvent(&gpService->sockets, &pSocket->listNode, WAITEVENT_READ);
    socketNodeRelease(&pSocket->listNode, &gpService->sockets);
  }

  socketListRelease(&gpService->sockets);
  (void) handle;
} // onLlcpDataIndication

/*------------------------------------------------------------------*/
/**
 * @fn    nfcLlcpConnect
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM
* @param jobject o  JAVA reference to current NativeLlcpSocket object
 * @param jint nSap
 * @param jstring sn
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean nfcLlcpConnect (JNIEnv *e, jobject o, jint nSap, jstring sn)
{
  PLlcpSocket pSocket        = NULL;
  jboolean    jResult        = FALSE;
  NFCSTATUS   status;
  PCHAR       pszServiceName = NULL;

  if (!nfcIsSocketAlive(e, o))
  {
    LOG("NataiveLlcpSocket_doConnect - Object already destroyed\n");
    return false;
  }

  if (sn)
  {
    char   szServiceName[128];
    size_t length;

    length = e->GetStringUTFLength(sn);
    if (length > sizeof(szServiceName))
    {
      length = sizeof(szServiceName);
    }
    memcpy(szServiceName, e->GetStringUTFChars(sn, NULL), length);
    szServiceName[length] = 0;
    pszServiceName        = szServiceName;

    LOGV("NativeLlcpSocket_doConnect <%s>", pszServiceName);
  }
  else
  {
    LOGV("NativeLlcpSocket_doConnect sap=0x%.02X", nSap);
  }

  socketListAquire(&gpService->sockets);

  do
  {
    //
    // Retrieve socket reference first
    //
    pSocket = nfcSocketGetReference(e, o);
    if (!pSocket)
    {
      break;
    }

    // delay for compatibility with NXP.
    usleep(100 * 1000);

    //
    // Start LLCP connect request
    //
    status = pNfcP2PConnectRequest(
      gpService->hListener,
      pSocket,
      nfcEventHandler,
      pSocket->localRw,
      (WORD) pSocket->localMiu,
      (BYTE) nSap,
      pszServiceName,
      &pSocket->hConnection
      );

    if (status != nfcStatusSuccess)
    {
      break;
    }

    //
    // Wait for established LLCP session or failed call
    //
    int stat = socketListSuspend(&gpService->sockets,
                                 &pSocket->listNode,
                                 WAITEVENT_CONNECT | WAITEVENT_CONNECTFAILED);

    if (stat == -1)
    {
      // someone is canelling
      LOG("LLCP connect interrupted");
      pSocket->hConnection = 0;
      break;
    }
    else
    {
      if (pSocket->listNode.event & WAITEVENT_CONNECTFAILED)
      {
        LOG("LLCP connect failed");
        pSocket->hConnection = 0;

        // clear connect related events.
        pSocket->listNode.event &= ~(WAITEVENT_CONNECT | WAITEVENT_CONNECTFAILED);

        break;
      }
      else
      {
        // remove event.
        pSocket->listNode.event &= ~WAITEVENT_CONNECT;
      }
    }

    // LLCP connection now exists, we able to send and receive data
    jResult = TRUE;
  } while (FALSE);

  if (pSocket)
  {
    socketNodeRelease(&pSocket->listNode, &gpService->sockets);
  }

  LOGV("NativeLlcpSocket_doConnect %s", jResult ? "OK" : "FAILED");

  socketListRelease(&gpService->sockets);

  return jResult;
} // nfcLlcpConnect

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeLlcpSocket_doConnect
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM
 * @param jobject o  JAVA reference to current NativeLlcpSocket object
 * @param jint nSap
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NativeLlcpSocket_doConnect (JNIEnv *e, jobject o, jint nSap)
{
  if (!nfcIsSocketAlive(e, o))
  {
    LOG("com_android_nfc_NativeLlcpSocket_doConnect - Object already destroyed\n");
    return false;
  }

  return nfcLlcpConnect(e, o, nSap, NULL);
}

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeLlcpSocket_doConnectBy
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM
 * @param jobject o  JAVA reference to current NativeLlcpSocket object
 * @param jstring sn
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NativeLlcpSocket_doConnectBy (JNIEnv *e, jobject o, jstring sn)
{
  if (!nfcIsSocketAlive(e, o))
  {
    LOG("com_android_nfc_NativeLlcpSocket_doConnectBy - Object already destroyed\n");
    return false;
  }
  return nfcLlcpConnect(e, o, 0, sn);
}

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeLlcpSocket_doClose
 *
 * @brief Close given socket. We perform LLCP disconnect first when
 *        a LLCP session exists
 *
 * @param JNIEnv * e  JAVA reference to current VM  JAVA reference to current VM
 * @param jobject o  JAVA reference to current NativeLlcpSocket object   JAVA Reference to NativeLlcpSockect object
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NativeLlcpSocket_doClose (JNIEnv *e, jobject o)
{
  PLlcpSocket pSocket;
  NFCSTATUS   status;

  if (!nfcIsSocketAlive(e, o))
  {
    LOG("com_android_nfc_NativeLlcpSocket_doClose - Object already destroyed\n");
    return false;
  }

  LOGV("NativeLlcpSocket_doClose() %d ENTER-GLOBAL-CLOSELOCK", (int)pthread_self());

  // only one thread may perform close-calls due to race-conditions
  // in the upper layer java-code pthread_mutex_lock (&gpService->sockets.CloseLock);

  socketListAquire(&gpService->sockets);

  pSocket = nfcSocketGetReference(e, o);

  LOGV("NativeLlcpSocket_doClose() enter, pSocket=%p", pSocket);

  if (pSocket)
  {
    //
    // Check for established session
    //

    if (pSocket->hConnection)
    {
      status = pNfcP2PDisconnectRequest(pSocket->hConnection);

      if (status == nfcStatusSuccess)
      {
        // wait for confirmation (any will do)
        int stat = socketListSuspend(&gpService->sockets, &pSocket->listNode, WAITEVENT_CLOSE);

        if (stat == 0)
        {
          // don't clear the close-event, other threads
          // may waiting for it as well...
        }
        else
        {
          LOG("llcp doClose interrupt in progress");
        }
      }
    }
    else
    {
      LOG("No hConnection, no action required");
    }

    // don't own the socket anymore.
    socketNodeRelease(&pSocket->listNode, &gpService->sockets);

    // unblock/kill all other threads waiting on socket..
    socketListInterrupt(&gpService->sockets, &pSocket->listNode);

    // remove from list
    if (socketListRemove(&gpService->sockets, &pSocket->listNode))
    {
      // finally free the socket.
      nfcSocketDestroy(pSocket);
    }
  }

  LOG("NativeLlcpSocket_doClose() exit");

  socketListRelease(&gpService->sockets);

  // only one thread may perform close-calls due to race-conditions
  // in the upper layer java-code
  pthread_mutex_unlock(&gpService->sockets.CloseLock);

  LOGV("NativeLlcpSocket_doClose() %d LEAVE-GLOBAL-CLOSELOCK", (int)pthread_self());

  return TRUE;
} // com_android_nfc_NativeLlcpSocket_doClose

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeLlcpSocket_doSend
 *
 * @brief Send data over established LLCP session
 *
 * @param JNIEnv * e  JAVA reference to current VM
 * @param jobject o  JAVA reference to current NativeLlcpSocket object
 *                   JAVA reference to NativeLlcpSockect object
 * @param jbyteArray data JAVA reference to given data buffer
 *
 *
 * @return jboolean
 */
/*------------------------------------------------------------------*/
static jboolean com_android_nfc_NativeLlcpSocket_doSend (JNIEnv *e, jobject o, jbyteArray data)
{
  PLlcpSocket pSocket;
  NFCSTATUS   status;
  jboolean    jResult = FALSE;

  if (!nfcIsSocketAlive(e, o))
  {
    LOG("com_android_nfc_NativeLlcpSocket_doSend - Object already destroyed\n");
    return false;
  }

  socketListAquire(&gpService->sockets);

  pSocket = nfcSocketGetReference(e, o);

  LOGV("NativeLlcpSocket_doSend() enter, pSocket %p", pSocket);

  if (pSocket)
  {
    if (!pSocket->hConnection)
    {
      LOG("NativeLlcpSocket_doSend() pSocket is a dummy");
      socketNodeRelease(&pSocket->listNode, &gpService->sockets);
      socketListRelease(&gpService->sockets);
      return false;
    }

    jbyte *pBuffer = e->GetByteArrayElements(data, NULL);
    size_t length  = e->GetArrayLength(data);

    status = pNfcP2PDataRequest(
      pSocket->hConnection,
      (unsigned char *) pBuffer,
      length
      );

    e->ReleaseByteArrayElements(data, pBuffer, JNI_ABORT);
    e->DeleteLocalRef(data);

    if (status == nfcStatusSuccess)
    {
      int stat = socketListSuspend(&gpService->sockets,
                                   &pSocket->listNode,
                                   WAITEVENT_WRITE | WAITEVENT_CLOSE);

      if (stat == 0)
      {
        if (pSocket->listNode.event & WAITEVENT_WRITE)
        {
          LOG("doSend data sent.");
          pSocket->listNode.event &= ~WAITEVENT_WRITE;
          jResult = TRUE;
        }
        if (pSocket->listNode.event & WAITEVENT_CLOSE)
        {
          // don't clear the close-event, other threads may are waiting
          // for it as well.
        }
      }
      else
      {
        LOG("doSend interrupted - return false");
      }
    }

    socketNodeRelease(&pSocket->listNode, &gpService->sockets);
  }

  socketListRelease(&gpService->sockets);

  return jResult;
} // com_android_nfc_NativeLlcpSocket_doSend



/*------------------------------------------------------------------*/
/**
 * @fn    loc_ReadFromRingbuffer
 *
 * @brief tries to read data from the internal rx-buffer.
 *
 * @return jint, amount of data read from rx-buffer
 */
/*------------------------------------------------------------------*/
static int loc_ReadFromRingbuffer (JNIEnv *e, PLlcpSocket pSocket, jbyteArray buffer)
{
  int length = 0;
  jbyte * dest = e->GetByteArrayElements(buffer, NULL);
  
  if (dest)
  {
    size_t amountTocopy = stmmin((size_t)e->GetArrayLength(buffer), pSocket->rxBuffer->dataAvail());
    
    pSocket->rxBuffer->read (dest, amountTocopy);
    length = amountTocopy;
    
    // release pinned array with copy-back semantics:
    e->ReleaseByteArrayElements(buffer, dest, 0);
  }
  return length;
}


/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeLlcpSocket_doReceive
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM
 * @param jobject o  JAVA reference to current NativeLlcpSocket object
 * @param jbyteArray buffer
 *
 *
 * @return jint
 */
/*------------------------------------------------------------------*/
static jint com_android_nfc_NativeLlcpSocket_doReceive (JNIEnv *e, jobject o, jbyteArray buffer)
{
  PLlcpSocket pSocket;
  int         length = 0;

  if (!nfcIsSocketAlive(e, o))
  {
    LOG("com_android_nfc_NativeLlcpSocket_doReceive - Object already destroyed\n");
    return -1;
  }

  socketListAquire(&gpService->sockets);

  pSocket = nfcSocketGetReference(e, o);

  LOGV("NativeLlcpSocket_doReceive() enter, socket %p", pSocket);

  if (pSocket)
  {
    if (!pSocket->hConnection)
    {
      LOG("NativeLlcpSocket_doReceive() pSocket is a dummy");
      socketNodeRelease(&pSocket->listNode, &gpService->sockets);
      socketListRelease(&gpService->sockets);
      return -1;
    }
    
    if (pSocket->rxBuffer->dataAvail())
    {
      // we can immediately answer this read-request without blocking!
      length = loc_ReadFromRingbuffer (e, pSocket, buffer);
      socketNodeRelease(&pSocket->listNode, &gpService->sockets);
      socketListRelease(&gpService->sockets);
      LOGV("NativeLlcpSocket_doReceive() exit length %d (non-blocking)", length);
      return length;
    }
    
    int stat = socketListSuspend(&gpService->sockets,
                                 &pSocket->listNode,
                                 WAITEVENT_READ | WAITEVENT_CLOSE);

    if (stat == -1)
    {
      // interrupt in progress:
      LOG("LLCP doReceive interrupted");
      pSocket->rxBuffer->clear();
    }
    else
    {
      if (pSocket->listNode.event & WAITEVENT_READ)
      {
        LOG("LLCP doReceive got read event");
        pSocket->listNode.event &= ~WAITEVENT_READ;
      }

      if (pSocket->listNode.event & WAITEVENT_CLOSE)
      {
        LOG("LLCP doReceive got close event, invalidate read result");
        // don't clear the close event, other threads may be
        // waiting on it as well.
        pSocket->rxBuffer->clear();
      }
    }

    length = loc_ReadFromRingbuffer (e, pSocket, buffer);
    socketNodeRelease(&pSocket->listNode, &gpService->sockets);
  }

  LOGV("NativeLlcpSocket_doReceive() exit length %d", length);

  socketListRelease(&gpService->sockets);

  return length;
} // com_android_nfc_NativeLlcpSocket_doReceive

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeLlcpSocket_doGetRemoteSocketMIU
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM
 * @param jobject o  JAVA reference to current NativeLlcpSocket object
 *
 *
 * @return jint
 */
/*------------------------------------------------------------------*/
static jint com_android_nfc_NativeLlcpSocket_doGetRemoteSocketMIU (JNIEnv *e, jobject o)
{
  PLlcpSocket pSocket;
  int         miu = 0;

  if (!nfcIsSocketAlive(e, o))
  {
    LOG("com_android_nfc_NativeLlcpSocket_doGetRemoteSocketMIU - Object already destroyed\n");
    return 0;
  }

  socketListAquire(&gpService->sockets);

  pSocket = nfcSocketGetReference(e, o);

  if (pSocket)
  {
    miu = pSocket->miu;
    socketNodeRelease(&pSocket->listNode, &gpService->sockets);
  }

  LOGV("NativeLlcpSocket_doGetRemoteSocketMIU(), pSocket %p -> miu %d", pSocket, miu);

  socketListRelease(&gpService->sockets);

  return miu;
} // com_android_nfc_NativeLlcpSocket_doGetRemoteSocketMIU

/*------------------------------------------------------------------*/
/**
 * @fn    com_android_nfc_NativeLlcpSocket_doGetRemoteSocketRW
 *
 * @brief
 *
 * @param JNIEnv * e  JAVA reference to current VM
 * @param jobject o  JAVA reference to current NativeLlcpSocket object
 *
 *
 * @return jint
 */
/*------------------------------------------------------------------*/
static jint com_android_nfc_NativeLlcpSocket_doGetRemoteSocketRW (JNIEnv *e, jobject o)
{
  PLlcpSocket pSocket;
  int         rw = 0;

  if (!nfcIsSocketAlive(e, o))
  {
    LOG("com_android_nfc_NativeLlcpSocket_doGetRemoteSocketRW - Object already destroyed\n");
    return 0;
  }

  socketListAquire(&gpService->sockets);

  pSocket = nfcSocketGetReference(e, o);

  if (pSocket)
  {
    rw = pSocket->rw;
    socketNodeRelease(&pSocket->listNode, &gpService->sockets);
  }

  LOGV("NativeLlcpSocket_doGetRemoteSocketRW(), pSocket %p -> rw %d", pSocket, rw);

  socketListRelease(&gpService->sockets);

  return rw;
} // com_android_nfc_NativeLlcpSocket_doGetRemoteSocketRW

/*
 * JNI registration.
 */
TNativeSocketFields NativeSocketFields;

// *INDENT-OFF*
static const JNINativeMethod gMethods[] =
 {
  {"doConnect", "(I)Z",                    (void *)com_android_nfc_NativeLlcpSocket_doConnect},
  {"doConnectBy", "(Ljava/lang/String;)Z", (void *)com_android_nfc_NativeLlcpSocket_doConnectBy},
  {"doClose", "()Z",                       (void *)com_android_nfc_NativeLlcpSocket_doClose},
  {"doSend", "([B)Z",                      (void *)com_android_nfc_NativeLlcpSocket_doSend},
  {"doReceive", "([B)I",                   (void *)com_android_nfc_NativeLlcpSocket_doReceive},
  {"doGetRemoteSocketMiu", "()I",          (void *)com_android_nfc_NativeLlcpSocket_doGetRemoteSocketMIU},
  {"doGetRemoteSocketRw", "()I",           (void *)com_android_nfc_NativeLlcpSocket_doGetRemoteSocketRW},
};

static const JNIFields gFields[] =
{
  { "mHandle",   "I", &NativeSocketFields.mHandle   },
  { "mSap",      "I", &NativeSocketFields.mSap      },
  { "mLocalMiu", "I", &NativeSocketFields.mLocalMiu },
  { "mLocalRw",  "I", &NativeSocketFields.mLocalRW  },
};
// *INDENT-ON*

bool register_com_android_nfc_NativeLlcpSocket (JNIEnv *e)
{
  jclass cls = e->FindClass(NATIVE_SOCKET_CLASS);

  return JNIRegisterFields(e, cls, gFields, NELEM(gFields)) &&
         e->RegisterNatives(cls, gMethods, NELEM(gMethods)) == 0;
}
} // namespace android
