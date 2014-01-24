/*
 * Copyright (C) 2010 The Android Open Source Project
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
 */

package com.android.nfc.stollmann;

import android.nfc.LlcpPacket;

/**
 * LlcpConnectionlessSocket represents a LLCP Connectionless object to be used
 * in a connectionless communication. This is a dummy implementation because 
 * connectionless LLCP is not in use in Android 4.0.3 and earlier.
 */
public class NativeLlcpConnectionlessSocket {

    private int mHandle;
    private int mSap;
    private int mLinkMiu;

    public NativeLlcpConnectionlessSocket(){ };

    public boolean doSendTo(int sap, byte[] data) {
      return false;
    }

    public LlcpPacket doReceiveFrom(int linkMiu) {
      return null;
    }

    public boolean doClose() {
      return false;
    }

    public int getLinkMiu(){
        return mLinkMiu;
    }

    public int getSap(){
        return mSap;
    }

    public int getHandle(){
        return mHandle;
    }

}