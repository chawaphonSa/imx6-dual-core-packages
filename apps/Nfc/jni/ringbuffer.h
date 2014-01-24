/** ----------------------------------------------------------------------
* File      : $RCSfile: ringbuffer.h,v $
* Revision  : $Revision: 1.2 $
* Date      : $Date: 2012/04/05 13:56:24 $
* Author    : Nils Pipenbrinck
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
#ifndef __COM_ANDROID_NFC_RINGBUFFER_H__
#define __COM_ANDROID_NFC_RINGBUFFER_H__

#include <stdint.h>

class RingBuffer
{
  uint8_t *mStorage;
  size_t   mCapacity;          // total capacity
  size_t   mAvail;             // capacity available
  size_t   mReadPos;           // current read position
  size_t   mWritePos;          // current write posistion

  void log (const char *topic, size_t amount);

public:
  RingBuffer(void);

  ~RingBuffer ();

  int allocate (size_t aCapacity);

  void deallocate (void);

  size_t freeAvail ();
  size_t dataAvail ();

  void clear (void);

  int write (void *data, size_t amount);
  int read (void *data, size_t amount);
};

#endif /* ifndef __COM_ANDROID_NFC_RINGBUFFER_H__ */
