/** ----------------------------------------------------------------------
* File      : $RCSfile: com_android_nfc_errorcodes.h,v $
* Revision  : $Revision: 1.1 $
* Date      : $Date: 2012/03/01 17:06:45 $
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
* $log$
*
----------------------------------------------------------------------*/

#ifndef __COM_ANDROID_NFC_ERRORCODES_H__
#define __COM_ANDROID_NFC_ERRORCODES_H__

// These constants must match with android.nfc.ErrorCodes.java
#define ERROR_SUCCESS  0
#define ERROR_IO  -1
#define ERROR_CANCELLED  -2
#define ERROR_TIMEOUT  -3
#define ERROR_BUSY  -4
#define ERROR_CONNECT  -5
#define ERROR_DISCONNECT  -5
#define ERROR_READ  -6
#define ERROR_WRITE  -7
#define ERROR_INVALID_PARAM  -8
#define ERROR_INSUFFICIENT_RESOURCES  -9
#define ERROR_SOCKET_CREATION  -10
#define ERROR_SOCKET_NOT_CONNECTED  -11
#define ERROR_BUFFER_TO_SMALL  -12
#define ERROR_SAP_USED  -13
#define ERROR_SERVICE_NAME_USED  -14
#define ERROR_SOCKET_OPTIONS  -15
#define ERROR_NFC_ON  -16
#define ERROR_NOT_INITIALIZED  -17
#define ERROR_SE_ALREADY_SELECTED  -18
#define ERROR_SE_CONNECTED  -19
#define ERROR_NO_SE_CONNECTED  -20
#define ERROR_NOT_SUPPORTED  -21

#endif
