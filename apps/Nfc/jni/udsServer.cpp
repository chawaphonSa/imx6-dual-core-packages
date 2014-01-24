/** --------------------------------------------------------------------------------------------------------
* File      : $RCSfile: udsServer.cpp,v $
* Revision  : $Revision: 1.15 $
* Date      : $Date: 2012/03/27 12:55:53 $
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
#include "udsServer.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <pwd.h>
#include <assert.h>

#include "com_android_nfc.h"

#define CMD_EXIT  0          // command to exit the thread
#define CMD_DROP  1          // command to drop current client connection

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
UdsServer::UdsServer (const char *aSocketName, udsDataCallback aCallback, void *aContext, BOOL secure)
{
  SocketName  = aSocketName;
  cbProc      = aCallback;
  cbContext   = aContext;
  fdSocket    = -1;
  fdClient    = -1;
  fdPipe[0]   = 0;
  fdPipe[1]   = 0;
  isOpen      = FALSE;
  isConnected = FALSE;
  isSecure    = secure;
}

/*------------------------------------------------------------------*/
/**
 * @fn    ~UdsServer
 *
 * @brief Destroy the server instance (release resources)
 *
 * @return -/
 */
/*------------------------------------------------------------------*/
UdsServer::~UdsServer ()
{
  if (isOpen)
    closeServer();
}

/*------------------------------------------------------------------*/
/**
 * @fn     loc_makeAddr
 *
 * @brief  Helper to make unix domain socket addresses.
 *
 * @return -1 on failure
 */
/*------------------------------------------------------------------*/
static int loc_makeAddr (const char *name, struct sockaddr_un *pAddr, socklen_t *pSockLen)
{
  int nameLen = strlen(name);

  if (nameLen >= (int) sizeof(pAddr->sun_path) - 1) /* too long? */
    return -1;

  pAddr->sun_path[0] = '\0';  /* abstract namespace */
  strcpy(pAddr->sun_path + 1, name);

  pAddr->sun_family = AF_LOCAL;
  *pSockLen         = 1 + nameLen + offsetof(struct sockaddr_un, sun_path);
  return 0;
} // loc_makeAddr

/*------------------------------------------------------------------*/
/**
 * @fn     writePipeCmd
 *
 * @brief  write a byte to the uds server communication pipe
 *
 * @return successcode
 */
/*------------------------------------------------------------------*/
BOOL UdsServer::writePipeCmd (BYTE command)
{
  int result = write(fdPipe[1], &command, sizeof(command));

  return (result == sizeof(command));
}

/*------------------------------------------------------------------*/
/**
 * @fn     callback
 *
 * @brief  callback proxy
 *
 * @return -
 */
/*------------------------------------------------------------------*/
void UdsServer::callback (UdsEvent event, void *data, size_t length)
{
  if (cbProc)
  {
    cbProc(cbContext, event, data, length);
  }
}


/*------------------------------------------------------------------*/
/**
 * @fn     checkAutentication
 *
 * @brief  for secure connections, check that user is NFC or ROOT
 *
 * @return
 */
/*------------------------------------------------------------------*/
bool UdsServer::checkAutentication (int suspectFd)
{
  struct ucred peerCreds;
  struct passwd *pwd;
  
  if (!isSecure)
  {
    return true;
  }

  memset (&peerCreds, 0, sizeof (peerCreds));
  int length = sizeof (peerCreds);
            
  if (-1 == getsockopt (suspectFd, SOL_SOCKET, SO_PEERCRED, &peerCreds, &length))
  {
    LOGE("UdsServer - getsockopt failed for SO_PEERCRED with error %d\n", errno);
    return false;
  }
    
  pwd = getpwuid(peerCreds.uid);

  if (pwd)
  {
    return (strcmp(pwd->pw_name, "nfc")     == 0) ||
           (strcmp(pwd->pw_name, "system")  == 0) ||
           (strcmp(pwd->pw_name, "root")    == 0);
  } 
  else
  {
    LOG("UdsServer - unable to get peer user-name for authentication\n");
    return false;
  }
}
    
/*------------------------------------------------------------------*/
/**
 * @fn     threadproc
 *
 * @brief  Thread main loop.
 *
 * @return
 */
/*------------------------------------------------------------------*/
void *UdsServer::threadproc (void)
{
  struct pollfd eventTable[2];

  // socket startup
  struct sockaddr_un sockAddr;
  socklen_t          sockLen;
  int                on = 1;

  if (loc_makeAddr(SocketName, &sockAddr, &sockLen) < 0)
  {
    LOG("UdsServer - failed top make address\n");
    sem_post(&initSemaphore);
    return 0;
  }

  fdSocket = socket(AF_LOCAL, SOCK_STREAM, PF_UNIX);
  if (fdSocket < 0)
  {
    LOG("UdsServer - failed to create socket\n");
    sem_post(&initSemaphore);
    return 0;
  }
  
  if (-1 == setsockopt (fdSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)))
  {
    LOGE("UdsServer - setsockopt failed for SO_REUSEADDR with error %d\n", errno);
    sem_post(&initSemaphore);
    return 0;
  }  

  if (bind(fdSocket, (const struct sockaddr *) &sockAddr, sockLen) < 0)
  {
    LOG("UdsServer - failed to bind socket\n");
    sem_post(&initSemaphore);
    return 0;
  }

  if (listen(fdSocket, 1) < 0)
  {
    LOG("UdsServer - failed to listen to socket\n");
    sem_post(&initSemaphore);
    return 0;
  }
  
  isOpen = TRUE;
  sem_post(&initSemaphore);

  do
  {
    eventTable[0].events  = POLLIN;
    eventTable[0].revents = 0;
    eventTable[1].fd      = fdPipe[0];
    eventTable[1].events  = POLLIN;
    eventTable[1].revents = 0;

    if (isConnected)
    {
      // waiting for data...
      eventTable[0].fd = fdClient;
    }
    else
    {
      // waiting for connections.
      eventTable[0].fd = fdSocket;
    }

    poll(eventTable, 2, -1);

    if (eventTable[1].revents & POLLIN)
    {
      // we received a thread-command:
      BYTE cmd;
      read(fdPipe[0], &cmd, sizeof(BYTE));

      switch (cmd)
      {
        case CMD_EXIT:
          LOG("UdsServer - got exit command\n");
          
          if (fdClient!=-1)         
          {
            LOG("UdsServer - close remote client connection\n");
            close(fdClient);
            fdClient=-1;
          }

          if (fdSocket!=-1)
          {
            LOG("UdsServer - close server connection\n");
            close(fdSocket);
            fdSocket=-1;
          }          
          return 0;

          break;

        case CMD_DROP:
          if (isConnected)
          {
            isConnected           = FALSE;
            close(fdClient);
            fdClient              = -1;
            eventTable[0].revents = 0;
            LOG("UdsServer - client connection dropped\n");
            callback(udsDisconnectIndication, 0, 0);
          }
          else
          {
            LOG("UdsServer - no client connection.\n");
          }
          break;

        default:
          LOGE("UdsServer - unexpected thread command %d\n", (int) cmd);
          return 0;

          break;
      } // switch
    }

    if (eventTable[0].revents & POLLIN)
    {
      if (!isConnected)
      {
        fdClient = accept(fdSocket, NULL, NULL);

        if (fdClient < 0)
        {
          LOG("UdsServer - error during accept\n");
        }
        else
        {
          if (!checkAutentication(fdClient))
          {
            LOG("UdsServer - client failed authentication\n");
            close (fdClient);
            fdClient = -1;
          }
          else
          {
            isConnected = TRUE;
            callback(udsConnectIndication, 0, 0);
          }
        }
      }
      else
      {
        int length = read(fdClient, rxBuffer, sizeof(rxBuffer));
        if (length > 0)
        {
          if (cbProc)
          {
            cbProc(cbContext, udsDataIndication, rxBuffer, length);
          }
        }
        else
        {
          LOG("io-error or disconnect.. drop connection\n");
          isConnected = FALSE;
          close(fdClient);
          fdClient    = -1;
          callback(udsDisconnectIndication, 0, 0);
        }
      }
    }
  } while (1);

  return 0;
} // threadproc

/*------------------------------------------------------------------*/
/**
 * @fn     threadprocProxy
 *
 * @brief  threadprocProxy
 *
 * @return -
 */
/*------------------------------------------------------------------*/
void *UdsServer::threadprocProxy (PVOID pvContext)
{
  // the proxy just relays the call to a member-function
  UdsServer *me = (UdsServer *) pvContext;

  return me->threadproc();
}

/*------------------------------------------------------------------*/
/**
 * @fn    openServer
 *
 * @brief opens the Socket, so clients can connect.
 *
 * @return BOOL - status code.
 */
/*------------------------------------------------------------------*/
BOOL UdsServer::openServer ()
{
  assert(!isOpen);

  if (pipe(fdPipe) == -1)
  {
    LOG("UdsServer - failed to create pipes\n");
    return FALSE;
  }

  if (sem_init(&initSemaphore, 0, 0) != 0)
  {
    LOG("UdsServer - failed to create semaphore\n");
    return FALSE;
  }

  if (pthread_create(&workerThread, NULL, UdsServer::threadprocProxy, this) != 0)
  {
    LOG("UdsServer - failed to create thread\n");
    sem_post(&initSemaphore);
  }

  // wait until the thread has signalled its init-state.
  sem_wait(&initSemaphore);
  sem_destroy(&initSemaphore);
  return isOpen;
} // openServer

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
BOOL UdsServer::closeServer ()
{
// send stop/process command:
  LOG("UdsServer - send stop command\n");
  writePipeCmd(CMD_EXIT);
  pthread_join(workerThread, NULL);

  // close the com-pipes
  close(fdPipe[0]);
  close(fdPipe[1]);

  isOpen = FALSE;
  return TRUE;
} // closeServer

/*------------------------------------------------------------------*/
/**
 * @fn     sendToClient
 *
 * @brief  send data to the client. This function is blocking
 *
 * @return BOOL - status code.
 */
/*------------------------------------------------------------------*/
BOOL UdsServer::sendToClient (const BYTE *data, size_t length)
{
  if ((!isConnected) || (!isOpen))
    return FALSE;

  int e = write(fdClient, data, length);
  if (e != (int) length)
  {
    LOGE("UdsServer - failed to write to socket (%d), errno = %s\n", e,strerror(errno));
    return FALSE;
  }
  
  return TRUE;
} // sendToClient

/*------------------------------------------------------------------*/
/**
 * @fn     dropConnection
 *
 * @brief  disconnect any client (if exist)
 *
 * @return BOOL - status code.
 */
/*------------------------------------------------------------------*/
BOOL UdsServer::dropConnection (void)
{
  return writePipeCmd(CMD_DROP);
}

/* test code

int main(int argc, char** argv)
{
  UdsServer * srv = new UdsServer("hallo");

  srv->openServer();

  getchar();

  srv->closeServer();

  delete srv;
}

*/
