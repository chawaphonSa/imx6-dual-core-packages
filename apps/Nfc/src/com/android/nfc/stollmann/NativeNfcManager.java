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

import com.android.nfc.DeviceHost;
import com.android.nfc.LlcpException;
import com.android.nfc.NfcService;
import android.content.SharedPreferences;

import android.annotation.SdkConstant;
import android.annotation.SdkConstant.SdkConstantType;
import android.content.Context;
import android.content.SharedPreferences;
import android.nfc.ErrorCodes;
import android.nfc.tech.Ndef;
import android.nfc.tech.TagTechnology;
import android.util.Log;

import java.io.File;

/**
 * Native interface to the NFC Manager functions
 */
public class NativeNfcManager implements DeviceHost {
    private static final String TAG  = "NativeNfcManager";
    private static final String PREF = "StollmannDeviceHost";

    static {
        System.loadLibrary("nfc_jni");
    }

    @SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
    public static final String INTERNAL_TARGET_DESELECTED_ACTION = "com.android.nfc.action.INTERNAL_TARGET_DESELECTED";

    /* Native structure */
    private int mNative;

    private final DeviceHostListener  mListener;
    private final Context             mContext;
    private SharedPreferences         mPrefs;

    static final String         PREF_READER_ENABLE_ON         = "reader_enable_on";
    static final String         PREF_IP1TARGET_ENABLE_ON      = "reader_ip1target_on";
    static final String         PREF_WLFEAST_ENABLE_ON        = "reader_wlfeat_on";
    
    static final boolean        PREF_READER_ENABLE_DEFAULT    = true;
       

    public NativeNfcManager(Context context, DeviceHostListener listener) {
        mListener = listener;
        mContext  = context;
        mPrefs    = mContext.getSharedPreferences(PREF, Context.MODE_PRIVATE);
        
        initializeNativeStructure();
    }
    
    public native boolean initializeNativeStructure();
    
    public boolean getReaderEnablePersistant ()
    {
      return mPrefs.getBoolean(PREF_READER_ENABLE_ON, PREF_READER_ENABLE_DEFAULT);
    }

    public void setReaderEnablePersistant (boolean mode)
    {
      SharedPreferences.Editor mPrefsEditor = mPrefs.edit();
      mPrefsEditor.putBoolean(PREF_READER_ENABLE_ON, mode);
      mPrefsEditor.apply();
    }

    public boolean getIp1TargetEnablePersistant ()
    {
      return mPrefs.getBoolean(PREF_IP1TARGET_ENABLE_ON, PREF_READER_ENABLE_DEFAULT);
    }

    public void setIp1TargetEnablePersistant (boolean mode)
    {
      SharedPreferences.Editor mPrefsEditor = mPrefs.edit();
      mPrefsEditor.putBoolean(PREF_IP1TARGET_ENABLE_ON, mode);
      mPrefsEditor.apply();
    }

    public boolean getWlFeatEnablePersistant ()
    {
      boolean value = mPrefs.getBoolean(PREF_WLFEAST_ENABLE_ON, false);
      Log.d(TAG, "getWlFeatEnablePersistant returns " + value);
      return value;
    }

    public void setWlFeaetEnablePersistant (boolean mode)
    {
      Log.d(TAG, "setWlFeatEnablePersistant: " + mode);
      SharedPreferences.Editor mPrefsEditor = mPrefs.edit();
      mPrefsEditor.putBoolean(PREF_WLFEAST_ENABLE_ON, mode);
      mPrefsEditor.apply();
    }


    @Override
    public void checkFirmware() {
      // This is a NOP for Stollmann NFC.
    }

    @Override
    public native boolean initialize();

    @Override
    public boolean deinitialize(){ return deinitialize(mServiceState);};
    
    int mServiceState=0;
    public void setServiceState(int state){ mServiceState = state;};
    public native boolean deinitialize(int state);

    @Override
    public native void enableDiscovery();

    @Override
    public native void disableDiscovery();

    @Override
    public int[] doGetSecureElementList()
	{
	  throw new RuntimeException ("doGetSecureElementList not implemented");
	}

    @Override
    public native void doSelectSecureElement();

    @Override
    public native void doDeselectSecureElement();

    @Override
    public native int doGetLastError();

    private NativeLlcpConnectionlessSocket doCreateLlcpConnectionlessSocket(int nSap)
    {
      // Connectionless LLCP is not used in Android 4.0.3 and earlier.
      return null;
    }

    private native NativeLlcpServiceSocket doCreateLlcpServiceSocket(int nSap, String sn, int miu,
            int rw, int linearBufferLength);
            
    @Override
    public LlcpServerSocket createLlcpServerSocket(int nSap, String sn, int miu,
            int rw, int linearBufferLength) throws LlcpException {
        LlcpServerSocket socket = doCreateLlcpServiceSocket(nSap, sn, miu, rw, linearBufferLength);
        if (socket != null) {
            return socket;
        } else {
            /* Get Error Status */
            int error = doGetLastError();

            Log.d(TAG, "failed to create llcp socket: " + ErrorCodes.asString(error));

            switch (error) {
                case ErrorCodes.ERROR_BUFFER_TO_SMALL:
                case ErrorCodes.ERROR_INSUFFICIENT_RESOURCES:
                    throw new LlcpException(error);
                default:
                    throw new LlcpException(ErrorCodes.ERROR_SOCKET_CREATION);
            }
        }
    }

    private native NativeLlcpSocket doCreateLlcpSocket(int sap, int miu, int rw,
            int linearBufferLength);
            
    @Override
    public LlcpSocket createLlcpSocket(int sap, int miu, int rw,
            int linearBufferLength) throws LlcpException {
        LlcpSocket socket = doCreateLlcpSocket(sap, miu, rw, linearBufferLength);
        if (socket != null) {
            return socket;
        } else {
            /* Get Error Status */
            int error = doGetLastError();

            Log.d(TAG, "failed to create llcp socket: " + ErrorCodes.asString(error));

            switch (error) {
                case ErrorCodes.ERROR_BUFFER_TO_SMALL:
                case ErrorCodes.ERROR_INSUFFICIENT_RESOURCES:
                    throw new LlcpException(error);
                default:
                    throw new LlcpException(ErrorCodes.ERROR_SOCKET_CREATION);
            }
        }
    }

    @Override
    public boolean doCheckLlcp()
    {
      // Always possible with Stollmann NFC
      return true;
    }

    @Override
    public boolean doActivateLlcp()
    {
      // Always enabled with Stollmann NFC
      return true;
    }

    private native void doResetTimeouts();

    @Override
    public void resetTimeouts() {
        doResetTimeouts();
    }

    public native void doAbort();

    private native boolean doSetTimeout(int tech, int timeout);
    @Override
    public boolean setTimeout(int tech, int timeout) {
        return doSetTimeout(tech, timeout);
    }

    private native int doGetTimeout(int tech);
    @Override
    public int getTimeout(int tech) {
        return doGetTimeout(tech);
    }


    @Override
    public boolean canMakeReadOnly(int ndefType) {
        /* Type1 and Type2 are always possible to set into read-only mode 
         * for Type3 this is only possible if it's a FeliCa light. 
         * 
         * Since there is no finer grained control on the tag we just advertise 
         * all FeliCa tags as protectable. 
         * 
         * Trying to write-protect ordinary FeliCa cards fill fail of course */
         
        return ((ndefType == Ndef.TYPE_1) || 
                (ndefType == Ndef.TYPE_2) ||
                (ndefType == Ndef.TYPE_3));
    }

    @Override
    public int getMaxTransceiveLength(int technology) {
        switch (technology) {
            case (TagTechnology.NFC_A):
            case (TagTechnology.MIFARE_CLASSIC):
            case (TagTechnology.MIFARE_ULTRALIGHT):
                return 253; // PN544 RF buffer = 255 bytes, subtract two for CRC
            case (TagTechnology.NFC_B):
                return 0; // PN544 does not support transceive of raw NfcB
            case (TagTechnology.NFC_V):
                return 253; // PN544 RF buffer = 255 bytes, subtract two for CRC
            case (TagTechnology.ISO_DEP):
                /* The maximum length of a normal IsoDep frame consists of:
                 * CLA, INS, P1, P2, LC, LE + 255 payload bytes = 261 bytes
                 * such a frame is supported. Extended length frames however
                 * are not supported.
                 */
                return 261; // Will be automatically split in two frames on the RF layer
            case (TagTechnology.NFC_F):
                return 252; // PN544 RF buffer = 255 bytes, subtract one for SoD, two for CRC
            default:
                return 0;
        }

    }

    private native String doDump();
    @Override
    public String dump() {
        return doDump();
    }

    /**
     * Notifies Ndef Message (TODO: rename into notifyTargetDiscovered)
     */
    private void notifyNdefMessageListeners(NativeNfcTag tag) {
        mListener.onRemoteEndpointDiscovered(tag);
    }

    /**
     * Notifies transaction
     */
    private void notifyTargetDeselected() {
        mListener.onCardEmulationDeselected();
    }

    /**
     * Notifies transaction
     */
    private void notifyTransactionListeners(byte[] aid) {
        mListener.onCardEmulationAidSelected(aid);
    }

    /**
     * Notifies P2P Device detected, to activate LLCP link
     */
    private void notifyLlcpLinkActivation(NativeP2pDevice device) {
        mListener.onLlcpLinkActivated(device);
    }

    /**
     * Notifies P2P Device detected, to activate LLCP link
     */
    private void notifyLlcpLinkDeactivated(NativeP2pDevice device) {
        mListener.onLlcpLinkDeactivated(device);
    }

    private void notifySeFieldActivated() {
        mListener.onRemoteFieldActivated();
    }

    private void notifySeFieldDeactivated() {
        mListener.onRemoteFieldDeactivated();
    }

    private void notifySeApduReceived(byte[] apdu) {
        mListener.onSeApduReceived(apdu);
    }

    private void notifySeEmvCardRemoval() {
        mListener.onSeEmvCardRemoval();
    }

    private void notifySeMifareAccess(byte[] block) {
        mListener.onSeMifareAccess(block);
    }
}
