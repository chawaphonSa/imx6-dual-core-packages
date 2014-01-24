/** --------------------------------------------------------------------------------------------------------
* File      : $RCSfile: nfcInit.h,v $
* Revision  : $Revision: 1.12 $
* Date      : $Date: 2012/03/01 13:31:23 $
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
* $log:$
*
-------------------------------------------------------------------------------------------------*/
#ifndef __NFCAPI_BINDING_INCLUDED
#define __NFCAPI_BINDING_INCLUDED

#include <debugout.h>

namespace android {
// load the dynamic nfc library
NFCSTATUS nfcLoadLibrary (void);

// remove the library binding.
void nfcFreeLibrary (void);

// general functions
extern PNfcInitialize         pNfcInitialize;
extern PNfcDone               pNfcDone;
extern PNfcCreate             pNfcCreate;
extern PNfcDestroy            pNfcDestroy;
extern PNfcRegisterListener   pNfcRegisterListener;
extern PNfcUnregisterListener pNfcUnregisterListener;
extern PNfcConnectTag         pNfcConnectTag;
extern PNfcDisconnectTag      pNfcDisconnectTag;
extern PNfcGetCapabilities    pNfcGetCapabilities;
extern PNfcCheckTag           pNfcCheckTag;
extern PNfcGetTagInfo         pNfcGetTagInfo;
extern PNfcDeviceIoControl    pNfcDeviceIoControl;

// ndef functions:
extern PNfcWriteNdefMessage pNfcWriteNdefMessage;
extern PNfcReadNdefMessage  pNfcReadNdefMessage;
extern PNfcMakeReadOnly     pNfcMakeReadOnly;

// legacy functions
extern PNfcExchangePdu pNfcExchangePdu;
extern PNfcFormat      pNfcFormat;
extern PNfcFormatNdef  pNfcFormatNdef;

// card emulation
extern PNfcTransmitPdu pNfcTransmitPdu;

// P2P access
extern PNfcP2PConnectRequest    pNfcP2PConnectRequest;
extern PNfcP2PListenRequest     pNfcP2PListenRequest;
extern PNfcP2PConnectResponse   pNfcP2PConnectResponse;
extern PNfcP2PDisconnectRequest pNfcP2PDisconnectRequest;
extern PNfcP2PDataRequest       pNfcP2PDataRequest;
extern PNfcP2PDataResponse      pNfcP2PDataResponse;
extern PNfcP2PSdpRequest        pNfcP2PSdpRequest;

extern PNfcP2POpenCLSocket  pNfcOpenCLSocket;
extern PNfcP2PCloseCLSocket pNfcP2PCloseCLSocket;
extern PNfcP2PDataRequestTo pNfcP2PDataRequestTo;

// SE access
extern PNfcSecureGetList        pNfcSecureGetList;
extern PNfcSecureSetMode        pNfcSecureSetMode;
extern PNfcSecureSetBatteryMode pNfcSecureSetBatteryMode;
extern PNfcSecureOpen           pNfcSecureOpen;
extern PNfcSecureClose          pNfcSecureClose;
extern PNfcSecureExchangePdu    pNfcSecureExchangePdu;

// AID Table
extern PNfcSecureSetAidTable pNfcSecureSetAidTable;
extern PNfcSecureGetAidTable pNfcSecureGetAidTable;

extern PDebugSetOutputHandler pDebugSetOutputHandler;
extern PDebugCaptureControl pDebugCaptureControl;
extern PDebugSetFlagsEx     pDebugSetFlagsEx;
}      // namespace

#endif // ifndef __NFCAPI_BINDING_INCLUDED
