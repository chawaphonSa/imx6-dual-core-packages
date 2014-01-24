/** --------------------------------------------------------------------------------------------------------
* File      : $RCSfile: nfcSocketList.h,v $
* Revision  : $Revision: 1.6 $
* Date      : $Date: 2011/05/23 08:38:57 $
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
#ifndef __NFCAPI_SOCKETLIST_H
#define __NFCAPI_SOCKETLIST_H
#include <pthread.h>
#include <stddef.h>
#include <basetype.h>

typedef struct tagSocketListNode
{
  struct tagSocketListNode *next;                 // linked list element
  DWORD                     event;                // bitmask of currently active events
  bool                      interrupted;          // true if interrupt in progress
  int                       numClients;           // # of threads currently in ownership of node.
} TSocketListNode;

typedef struct tagSocketList
{
  TSocketListNode *root;                          // linked list root
  pthread_mutex_t  lock;                          // global lock for list and nodes
  pthread_cond_t   gate;                          // wakeup gate (global for all threads)
  pthread_mutex_t  CloseLock;                     // global lock for socket.close() We need this
                                                  // because of some race conditions in the npp java-code
} TSocketList;

// misc events to send/wait for
#define WAITEVENT_CONNECT        1
#define WAITEVENT_READ           2
#define WAITEVENT_WRITE          4
#define WAITEVENT_CLOSE          8
#define WAITEVENT_CONNECTFAILED  16

/*------------------------------------------------------------------*/
/**
* @fn     socketListInit
*
* @brief  Initialize an empty socket list structure
*
* @return -
*/
/*------------------------------------------------------------------*/
void socketListInit (TSocketList *list);

/*------------------------------------------------------------------*/
/**
* @fn     socketListDestroy
*
* @brief  free an empty socket list structure (reclaim resources)
*
* @return -
*/
/*------------------------------------------------------------------*/
void socketListDestroy (TSocketList *list);

/*------------------------------------------------------------------*/
/**
* @fn     socketListAquire
*
* @brief  aquire exclusive access to the socket-list
*
* @return -
*/
/*------------------------------------------------------------------*/
void socketListAquire (TSocketList *list);

/*------------------------------------------------------------------*/
/**
* @fn     socketListRelease
*
* @brief  release exclusive access to the socket-list
*
* @return -
*/
/*------------------------------------------------------------------*/
void socketListRelease (TSocketList *list);

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
int socketListSuspend (TSocketList *list, TSocketListNode *node, DWORD waitEvent);

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
void socketListInterrupt (TSocketList *list, TSocketListNode *node);

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
void socketListEvent (TSocketList *list, TSocketListNode *node, DWORD event);

/*------------------------------------------------------------------*/
/**
* @fn     socketListAdd
*
* @brief  add node to list. no socketNodeAquire call required
*/
/*------------------------------------------------------------------*/
void socketListAdd (TSocketList *list, TSocketListNode *node);

/*------------------------------------------------------------------*/
/**
* @fn     socketListRemove
*
* @brief  remove node to list. no socketNodeAquire call required
*/
/*------------------------------------------------------------------*/
bool socketListRemove (TSocketList *list, TSocketListNode *node);

/*------------------------------------------------------------------*/
/**
* @fn     socketListExists
*
* @brief  check if node exists in list. no socketNodeAquire call required
*/
/*------------------------------------------------------------------*/
bool socketListExists (TSocketList *list, TSocketListNode *node);

/*------------------------------------------------------------------*/
/**
* @fn     socketNodeInit
*
* @brief  initialize node members
*/
/*------------------------------------------------------------------*/
void socketNodeInit (TSocketListNode *node);

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
bool socketNodeAquire (TSocketListNode *node);

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
void socketNodeRelease (TSocketListNode *node, TSocketList *list);

#endif // ifndef __NFCAPI_SOCKETLIST_H
