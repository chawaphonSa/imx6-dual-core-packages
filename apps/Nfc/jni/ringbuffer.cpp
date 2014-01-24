/** --------------------------------------------------------------------------------------------------------
* File      : $RCSfile: ringbuffer.cpp,v $
* Revision  : $Revision: 1.2 $
* Date      : $Date: 2012/04/05 13:56:24 $
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
#include "ringbuffer.h"
#include "com_android_nfc.h"

RingBuffer::RingBuffer (void)
{
  mStorage  = 0;
  mCapacity = 0;
  mReadPos  = 0;
  mWritePos = 0;
  mAvail    = 0;
}

RingBuffer::~RingBuffer ()
{
  deallocate();
}

int RingBuffer::allocate (size_t aCapacity)
{
  mStorage = (uint8_t *) malloc(aCapacity);
  if (mStorage)
  {
    mCapacity = aCapacity;
    mReadPos  = 0;
    mWritePos = 0;
    mAvail    = aCapacity;
    return 1;   // return okay
  }
  else
  {
    mCapacity = 0;
    mReadPos  = 0;
    mWritePos = 0;
    mAvail    = 0;
    return 0; // allocation error
  }
} /* allocate */

void RingBuffer::deallocate (void)
{
  if (mStorage)
    free(mStorage);
  mStorage = 0;
}

size_t RingBuffer::freeAvail ()
{
  // return amount of free memory
  return mAvail;
}

size_t RingBuffer::dataAvail ()
{
  // return amount of used memory
  return mCapacity - mAvail;
}

void RingBuffer::clear (void)
{
  mReadPos  = 0;
  mWritePos = 0;
}

int RingBuffer::write (void *data, size_t amount)
{
  log("Ringbuffer-Write", amount);

  if (amount > freeAvail())
  {
    /* grow on demand */
    uint8_t *newData = (uint8_t *) realloc(mStorage, amount + mCapacity);
    if (newData)
    {
      mStorage   = newData;
      mCapacity += amount;
      mAvail    += amount;
    }
    else
    {
      LOGE("! allocation failure in ringbuffer for %d bytes", amount + mCapacity);
      return 0;
    }
  }

  if ((!mStorage) || (!data))
  {
    LOGE("! ringbuffer, data or mStorage is null %p, %p", mStorage, data);
    return 0;
  }

  uint8_t *alias = (uint8_t *) data;

  if (amount <= freeAvail())
  {
    // allocate data from buffer:
    mAvail -= amount;

    // copy loop
    while (amount)
    {
      // get current largest piece in buffer:
      size_t chunk = mCapacity - mWritePos;

      // make it smaller if not entirely used:
      if (chunk > amount)
        chunk = amount;

      // copy to ringbuffer storage:
      memcpy(mStorage + mWritePos, alias, chunk);

      // move write-pos and reduce to-copy amount:
      mWritePos += chunk;
      alias     += chunk;
      amount    -= chunk;

      // check for wrap-around:
      if (mWritePos == mCapacity)
      {
        mWritePos = 0;
      }
    }
    return 1;
  }
  else
  {
    // not enough free memory in buffer:
    LOGV("! ringbuffer: overflow: amount %d freeAvail %d", amount, freeAvail());
    return 0;
  }
} /* write */

int RingBuffer::read (void *data, size_t amount)
{
  log("Ringbuffer-Read", amount);

  if ((!mStorage) || (!data))
  {
    LOGE("! ringbuffer, data or mStorage is null %p, %p", mStorage, data);
    return 0;
  }

  if (amount > dataAvail())
  {
    // read past the end of the buffer
    LOGE("! ringbuffer, read past end of data: %d, avail %d", amount, dataAvail());
    return 0;
  }

  uint8_t *alias = (uint8_t *) data;

  // make data read from buffer free again:
  mAvail += amount;

  // copy-loop:
  while (amount)
  {
    // get current largest piece in buffer:
    size_t chunk = mCapacity - mReadPos;

    // make it smaller if not entirely used:
    if (chunk > amount)
      chunk = amount;

    // copy to ringbuffer storage:
    memcpy(alias, mStorage + mReadPos, chunk);

    // move write-pos and reduce to-copy amount:
    mReadPos += chunk;
    alias    += chunk;
    amount   -= chunk;

    // check for wrap-around:
    if (mReadPos == mCapacity)
    {
      mReadPos = 0;
    }
  }

  if (mReadPos == mWritePos)
  {
    // ringbuffer is empty, reset readwrite pointers
    mReadPos  = 0;
    mWritePos = 0;
  }

  return 1;
} /* read */

void RingBuffer::log (const char *topic, size_t amount)
{
  (void)topic;
  (void)amount;
  /*
  LOGV("! %s: amount = %d\n", topic, amount);
  LOGV("      mCapacity = %d\n", mCapacity);
  LOGV("      mAvail    = %d\n", mAvail);
  LOGV("      mReadPos = %d\n", mReadPos);
  LOGV("      mWritePos = %d\n", mWritePos);
  */
}
