/** ----------------------------------------------------------------------
* File      : $RCSfile: st21nfca_sefix.h,v $
* Revision  : $Revision: 1.2 $
* Date      : $Date: 2012/08/03 15:28:50 $
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
#ifndef __COM_ANDROID_NFC_ST21NFCA_UICC_BLACKLIST_H__
#define __COM_ANDROID_NFC_ST21NFCA_UICC_BLACKLIST_H__

namespace android 
{
  
  BOOL fix_GetUiccActivationFlags(char * initString, DWORD oemOptions);
  
  BOOL fix_IsUiccOnBlacklist (int id);
  
}

#endif
