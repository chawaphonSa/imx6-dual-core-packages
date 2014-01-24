/** --------------------------------------------------------------------------------------------------------
* File      : $RCSfile: udsServer.h,v $
* Revision  : $Revision: 1.8 $
* Date      : $Date: 2012/03/30 14:13:53 $
* Author    : Nils Pipenbrinck
* Copyright (c)2008 Stollmann E+V GmbH
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
-------------------------------------------------------------------------------------------------*/
#ifndef __NFCAPI_UDSSERVER_H
#define __NFCAPI_UDSSERVER_H

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <basetype.h>
#include <semaphore.h>

// event types for the callback function:
typedef enum tagUdsEvent
{
  udsDataIndication       = 0,
  udsConnectIndication    = 1,
  udsDisconnectIndication = 2
} UdsEvent;

typedef enum tagUdsState
{
  UDS_STATE_DISCONNECTED,
  UDS_STATE_CONNECTED
} UdsState;

// callback function for data-indications
typedef void (*udsDataCallback)(void *context, UdsEvent event, void *data, size_t length);

// ---------------------------------------------------------------------
//
// A simple Unix Domain Socket server.
//
// This server allows applications to send and receive data via
// Unix Domain Sockets (somewhat similar to named pipes)
//
// Writes to the server are blocking,
// Reads are implemented via a callback function.
//
// This server has a worker-thread running in background, so all
// callback will occur in the context of a different thread. Keep
// this in mind and do your syncronization!
//
// ---------------------------------------------------------------------

class UdsServer
{
private:
  int fdSocket;
  int fdClient;
  int fdPipe[2];
  pthread_t workerThread;
  sem_t initSemaphore;
  volatile BOOL isOpen;
  volatile BOOL isConnected;
  BOOL          isSecure;
  const char *SocketName;
  udsDataCallback cbProc;
  void *cbContext;
  BYTE rxBuffer[512];

  BOOL         startThread ();
  BOOL         writePipeCmd (BYTE command);
  void        *threadproc (void);
  static void *threadprocProxy (PVOID pvContext);

  void callback (UdsEvent event, void *data, size_t length);

  // check client authentication
  bool checkAutentication (int clientFd);

  // override copy and assignment operator. Makes this object
  // non-copyable.
  UdsServer &operator= (const UdsServer & other)
  {
    (void) other; // suspress unused parameter warning
    return *(UdsServer *) 0;
  }

  UdsServer (const UdsServer & other)
  {
    (void) other; // suspress unused parameter warning
  }

public:
  /*------------------------------------------------------------------*/
  /**
   * @fn    UdsServer
   *
   * @brief Create the server instance
   *
   * @param aSocketName - Name of the socket (ascii-string)
   * @param aCallback   - callback function pointer
   * @param aContext    - callback context (opaque handle)
   *
   * @return -/
   */
  /*------------------------------------------------------------------*/
  UdsServer (const char *aSocketName, udsDataCallback aCallback, void *aContext, BOOL secure);

  /*------------------------------------------------------------------*/
  /**
   * @fn    ~UdsServer
   *
   * @brief Destroy the server instance (release resources)
   *
   * @return -/
   */
  /*------------------------------------------------------------------*/
  ~UdsServer ();

  /*------------------------------------------------------------------*/
  /**
   * @fn    openServer
   *
   * @brief opens the Socket, so clients can connect.
   *
   * @return BOOL - status code.
   */
  /*------------------------------------------------------------------*/
  BOOL openServer ();

  /*------------------------------------------------------------------*/
  /**
   * @fn    closeServer
   *
   * @brief  close the socket. No clients can connect anymore.
   *         active connections will be dropped.
   *
   * @return BOOL - status code.
   */
  /*------------------------------------------------------------------*/
  BOOL closeServer ();

  /*------------------------------------------------------------------*/
  /**
   * @fn     sendToClient
   *
   * @brief  send data to the client. This function is blocking
   *
   * @return BOOL - status code.
   */
  /*------------------------------------------------------------------*/
  BOOL sendToClient (const BYTE *data, size_t length);

  /*------------------------------------------------------------------*/
  /**
   * @fn     dropConnection
   *
   * @brief  disconnect any client (if exist)
   *
   * @return BOOL - status code.
   */
  /*------------------------------------------------------------------*/
  BOOL dropConnection (void);
};

#endif // ifndef __NFCAPI_UDSSERVER_H
