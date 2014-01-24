/** --------------------------------------------------------------------------------------------------------
* File      : $RCSfile: nfcSocketList.cpp,v $
* Revision  : $Revision: 1.9 $
* Date      : $Date: 2011/10/11 14:42:46 $
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
#include "nfcSocketList.h"
#include "com_android_nfc.h"

// #define VERBOSE
/*------------------------------------------------------------------*/
/**
* @fn     socketListInit
*
* @brief  Initialize an empty socket list structure
*
* @return -
*/
/*------------------------------------------------------------------*/
void socketListInit (TSocketList *list)
{
  list->root = 0;

  pthread_mutex_init(&list->lock, NULL);
  pthread_cond_init(&list->gate, NULL);
  pthread_mutex_init(&list->CloseLock, NULL);
}

/*------------------------------------------------------------------*/
/**
* @fn     socketListDestroy
*
* @brief  free an empty socket list structure (reclaim resources)
*
* @return -
*/
/*------------------------------------------------------------------*/
void socketListDestroy (TSocketList *list)
{
  pthread_mutex_destroy(&list->lock);
  pthread_cond_destroy(&list->gate);
  pthread_mutex_destroy(&list->CloseLock);
}

/*------------------------------------------------------------------*/
/**
* @fn     socketNodeInit
*
* @brief  initialize node members
*/
/*------------------------------------------------------------------*/
void socketNodeInit (TSocketListNode *node)
{
  node->next        = 0;
  node->event       = 0;
  node->numClients  = 0;
  node->interrupted = false;

  #ifdef VERBOSE
  LOGV("SocketList: new node %p, thread=%p", node, pthread_self());
  #endif
} // socketNodeInit

/*------------------------------------------------------------------*/
/**
* @fn     socketListAquire
*
* @brief  aquire exclusive access to the socket-list
*
* @return -
*/
/*------------------------------------------------------------------*/
void socketListAquire (TSocketList *list)
{
  #ifdef VERBOSE
  LOGV("SocketList: aquire thread=%d", pthread_self());
  #endif

  pthread_mutex_lock(&list->lock);
}

/*------------------------------------------------------------------*/
/**
* @fn     socketListRelease
*
* @brief  release exclusive access to the socket-list
*
* @return -
*/
/*------------------------------------------------------------------*/
void socketListRelease (TSocketList *list)
{
  #ifdef VERBOSE
  LOGV("SocketList: release thread=%d", pthread_self());
  #endif
  pthread_mutex_unlock(&list->lock);
}

/*------------------------------------------------------------------*/
/**
* @fn     socketListSuspend
*
* @brief  suspend exclusive access to the socket-list.
*
*         The function will return with exclusive access again if
*         any bit in waitEvent matches with a event send to the node
*         via socketListEvent or if an interrupt is in progress.
*
*         To call this function you need to aquire ownership of the
*         node as well (socketNodeAquire)
*
* @return  0  if wakeup via event
*         -1  interrupt (e.g. terminate quickly)
*/
/*------------------------------------------------------------------*/
int socketListSuspend (TSocketList *list, TSocketListNode *node, DWORD waitEvent)
{
  bool gotEvent = false;
  int  retval   = 0;

  #ifdef VERBOSE
  LOGV("SocketList: SUSPEND thread=%d, node = %p", pthread_self(), node);
  if (waitEvent & WAITEVENT_CONNECT)
  {
    LOG("  thread is waiting for event: WAITEVENT_CONNECT");
  }
  if (waitEvent & WAITEVENT_CLOSE)
  {
    LOG("  thread is waiting for event: WAITEVENT_CLOSE");
  }
  if (waitEvent & WAITEVENT_READ)
  {
    LOG("  thread is waiting for event: WAITEVENT_READ");
  }
  if (waitEvent & WAITEVENT_WRITE)
  {
    LOG("  thread is waiting for event: WAITEVENT_WRITE");
  }
  if (waitEvent & WAITEVENT_CONNECTFAILED)
  {
    LOG("  thread is waiting for event: WAITEVENT_CONNECTFAILED");
  }
  #endif

  do
  {
    pthread_cond_wait(&list->gate, &list->lock);

    if (node->interrupted)
    {
      retval   = -1;
      gotEvent = true;
      #ifdef VERBOSE
      LOGV("SocketList: SUSPEND - got Interrupt node = %p", node);
      #endif
    }
    else
    {
      if (waitEvent & node->event)
      {
        #ifdef VERBOSE
        LOGV("SocketList: WAKEUP got event thread=%d, node = %p", pthread_self(), node);
        #endif
        gotEvent = true;
      }
    }
  } while (!gotEvent);
  return retval;
} // socketListSuspend

/*------------------------------------------------------------------*/
/**
* @fn     socketListEvent
*
* @brief  send a wakeup event to a socketlist node
*
*         This function will wakeup all threads waiting for bits
*         of event on node.
*
*         The caller needs ownwership of the node (socketNodeAquire)
*/
/*------------------------------------------------------------------*/
void socketListEvent (TSocketList *list, TSocketListNode *node, DWORD event)
{
#ifdef VERBOSE
  LOGV("SocketList: send event thread=%d, node = %p", pthread_self(), node);

  if (event & WAITEVENT_CONNECT)
  {
    LOG("  event contains: WAITEVENT_CONNECT");
  }
  if (event & WAITEVENT_CLOSE)
  {
    LOG("  event contains: WAITEVENT_CLOSE");
  }
  if (event & WAITEVENT_READ)
  {
    LOG("  event contains: WAITEVENT_READ");
  }
  if (event & WAITEVENT_WRITE)
  {
    LOG("  event contains: WAITEVENT_WRITE");
  }
  if (event & WAITEVENT_CONNECTFAILED)
  {
    LOG("  event contains: WAITEVENT_CONNECTFAILED");
  }
#endif

  // set the event:
  node->event |= event;

  // wakeup everyone.
  pthread_cond_broadcast(&list->gate);
} // socketListEvent

/*------------------------------------------------------------------*/
/**
* @fn     socketListInterrupt
*
* @brief  wakeup all suspended threads
*
*         This function will wakeup all threads waiting in
*         socketListSuspend and will disallow any further
*         calls to socketNodeAquire
*
*         After this call returns, no thread will have ownership
*         of the node anymore, and it is safe to delete it.
*
*         The caller must not has ownership of the node
*         =============================================
*/
/*------------------------------------------------------------------*/
void socketListInterrupt (TSocketList *list, TSocketListNode *node)
{
// set interrupt signal:
  node->interrupted = true;

  if (node->numClients)
  {
    #ifdef VERBOSE
    LOGV("SocketList: send interrupt thread=%d, node = %p", pthread_self(), node);
    #endif

    // wakeup everyone.
    pthread_cond_broadcast(&list->gate);

    // wait until all threads called socketNodeRelease
    do
    {
      pthread_cond_wait(&list->gate, &list->lock);
    } while (node->numClients != 0);
  }
} // socketListInterrupt

/*------------------------------------------------------------------*/
/**
* @fn     socketNodeAquire
*
* @brief  aquire ownership of the node.
*
*         More than one thread may currently have ownership of a node
*
* @return false if access was denied.
*/
/*------------------------------------------------------------------*/
bool socketNodeAquire (TSocketListNode *node)
{
  if (node->interrupted)
  {
    #ifdef VERBOSE
    LOGV("SocketList: aquire node rejected thread=%d, node = %p", pthread_self(), node);
    #endif
    return false;
  }

  node->numClients++;
  return true;
} // socketNodeAquire

/*------------------------------------------------------------------*/
/**
* @fn     socketNodeRelease
*
* @brief  release ownership of the node.
*
*         if an interrupt is in progress and no thread has ownwership
*         of a node anymore the interrupting thread will be unblocked.
*/
/*------------------------------------------------------------------*/
void socketNodeRelease (TSocketListNode *node, TSocketList *list)
{
  node->numClients--;
  #ifdef VERBOSE
  LOGV("SocketList: released one ownership of node %p (from %d to %d)",
       node,
       node->numClients + 1,
       node->numClients);
  #endif
  if (!node->numClients && node->interrupted)
  {
    #ifdef VERBOSE
    LOGV(
      "SocketList: last ownership lost for node %p. Interrupt requested, broadcast to waiting threads",
      node);
    #endif
    pthread_cond_broadcast(&list->gate);
  }
} // socketNodeRelease

/*------------------------------------------------------------------*/
/**
* @fn     socketListAdd
*
* @brief  add node to list. no socketNodeAquire call required
*/
/*------------------------------------------------------------------*/
void socketListAdd (TSocketList *list, TSocketListNode *node)
{
  #ifdef VERBOSE
  LOGV("SocketList: list ADD thread=%d, node = %p", pthread_self(), node);
  #endif

  if (socketListExists(list, node))
  {
    LOG("!SocketList: node already exists! This should never happen!");
  }
  else
  {
    node->next = list->root;
    list->root = node;
  }
} // socketListAdd

/*------------------------------------------------------------------*/
/**
* @fn     socketListRemove
*
* @brief  remove node to list. no socketNodeAquire call required
*/
/*------------------------------------------------------------------*/
bool socketListRemove (TSocketList *list, TSocketListNode *node)
{
  #ifdef VERBOSE
  LOGV("SocketList: list REMOVE thread=%d, node = %p", pthread_self(), node);
  #endif

  TSocketListNode *prev  = 0;
  bool             found = false;

  for (TSocketListNode *n = list->root; n; n = n->next)
  {
    if (n == node)
    {
      // unlink:
      if (prev)
      {
        prev->next = node->next;
      }
      else
      {
        list->root = node->next;
      }

      found = true;
      break;
    }
    prev = n;
  }

  if (!found)
  {
    LOG("SocketList: list REMOVE - Element not found! This should never happen!");
  }
  return found;
} // socketListRemove

/*------------------------------------------------------------------*/
/**
* @fn     socketListExists
*
* @brief  check if node exists in list. no socketNodeAquire call required
*/
/*------------------------------------------------------------------*/
bool socketListExists (TSocketList *list, TSocketListNode *node)
{
  for (TSocketListNode *n = list->root; n; n = n->next)
    if (n == node)
      return true;

  return false;
}
