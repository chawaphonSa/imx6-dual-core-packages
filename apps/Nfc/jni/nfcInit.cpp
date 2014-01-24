/** --------------------------------------------------------------------------------------------------------
* File      : $RCSfile: nfcInit.cpp,v $
* Revision  : $Revision: 1.16 $
* Date      : $Date: 2012/03/01 13:31:23 $
* Author    : Mathias Ellinger
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

#include "com_android_nfc.h"
#include "nfcInit.h"
#include <fcntl.h>

namespace android
{
// -----------------------------------------------------------------
// function-pointers and names for dynamic import
// -----------------------------------------------------------------

PNfcInitialize           pNfcInitialize           = NULL;
PNfcDone                 pNfcDone                 = NULL;
PNfcCreate               pNfcCreate               = NULL;
PNfcDestroy              pNfcDestroy              = NULL;
PNfcRegisterListener     pNfcRegisterListener     = NULL;
PNfcUnregisterListener   pNfcUnregisterListener   = NULL;
PNfcWriteNdefMessage     pNfcWriteNdefMessage     = NULL;
PNfcReadNdefMessage      pNfcReadNdefMessage      = NULL;
PNfcConnectTag           pNfcConnectTag           = NULL;
PNfcDisconnectTag        pNfcDisconnectTag        = NULL;
PNfcGetCapabilities      pNfcGetCapabilities      = NULL;
PNfcCheckTag             pNfcCheckTag             = NULL;
PNfcMakeReadOnly         pNfcMakeReadOnly         = NULL;
PNfcGetTagInfo           pNfcGetTagInfo           = NULL;
PNfcExchangePdu          pNfcExchangePdu          = NULL;
PNfcFormat               pNfcFormat               = NULL;
PNfcFormatNdef           pNfcFormatNdef           = NULL;
PNfcTransmitPdu          pNfcTransmitPdu          = NULL;
PNfcDeviceIoControl      pNfcDeviceIoControl      = NULL;
PNfcP2PConnectRequest    pNfcP2PConnectRequest    = NULL;
PNfcP2PListenRequest     pNfcP2PListenRequest     = NULL;
PNfcP2PConnectResponse   pNfcP2PConnectResponse   = NULL;
PNfcP2PDisconnectRequest pNfcP2PDisconnectRequest = NULL;
PNfcP2PDataRequest       pNfcP2PDataRequest       = NULL;
PNfcP2PDataResponse      pNfcP2PDataResponse      = NULL;
PNfcP2PSdpRequest        pNfcP2PSdpRequest        = NULL;
PNfcP2POpenCLSocket      pNfcOpenCLSocket         = NULL;
PNfcP2PCloseCLSocket     pNfcP2PCloseCLSocket     = NULL;
PNfcP2PDataRequestTo     pNfcP2PDataRequestTo     = NULL;
PDebugCaptureControl     pDebugCaptureControl     = NULL;
PDebugSetFlagsEx         pDebugSetFlagsEx         = NULL;
PNfcSecureGetList        pNfcSecureGetList        = NULL;
PNfcSecureSetMode        pNfcSecureSetMode        = NULL;
PNfcSecureSetBatteryMode pNfcSecureSetBatteryMode = NULL;
PNfcSecureSetAidTable    pNfcSecureSetAidTable    = NULL;
PNfcSecureGetAidTable    pNfcSecureGetAidTable    = NULL;
PDebugSetOutputHandler   pDebugSetOutputHandler   = NULL;
PNfcSecureOpen           pNfcSecureOpen           = NULL;
PNfcSecureClose          pNfcSecureClose          = NULL;
PNfcSecureExchangePdu    pNfcSecureExchangePdu    = NULL;


static const struct
{
  void      **funcPtr;
  const char *funcName;
}
ImportTable[] = {
  {(void **) &pNfcInitialize,           "nfcInitialize"          },
  {(void **) &pNfcDone,                 "nfcDone"                },
  {(void **) &pNfcCreate,               "nfcCreate"              },
  {(void **) &pNfcDestroy,              "nfcDestroy"             },
  {(void **) &pNfcRegisterListener,     "nfcRegisterListener"    },
  {(void **) &pNfcUnregisterListener,   "nfcUnregisterListener"  },
  {(void **) &pNfcWriteNdefMessage,     "nfcWriteNdefMessage"    },
  {(void **) &pNfcReadNdefMessage,      "nfcReadNdefMessage"     },
  {(void **) &pNfcConnectTag,           "nfcConnectTag"          },
  {(void **) &pNfcDisconnectTag,        "nfcDisconnectTag"       },
  {(void **) &pNfcCheckTag,             "nfcCheckTag"            },
  {(void **) &pNfcGetCapabilities,      "nfcGetCapabilities"     },
  {(void **) &pNfcGetTagInfo,           "nfcGetTagInfo"          },
  {(void **) &pNfcExchangePdu,          "nfcExchangePdu"         },
  {(void **) &pNfcFormat,               "nfcFormat"              },
  {(void **) &pNfcFormatNdef,           "nfcFormatNdef"          },
  {(void **) &pNfcMakeReadOnly,         "nfcMakeReadOnly"        },
  {(void **) &pNfcTransmitPdu,          "nfcTransmitPdu"         },
  {(void **) &pNfcP2PConnectRequest,    "nfcP2PConnectRequest"   },
  {(void **) &pNfcP2PListenRequest,     "nfcP2PListenRequest"    },
  {(void **) &pNfcP2PConnectResponse,   "nfcP2PConnectResponse"  },
  {(void **) &pNfcP2PDisconnectRequest, "nfcP2PDisconnectRequest"},
  {(void **) &pNfcP2PDataRequest,       "nfcP2PDataRequest"      },
  {(void **) &pNfcP2PDataResponse,      "nfcP2PDataResponse"     },
  {(void **) &pNfcP2PSdpRequest,        "nfcP2PSdpRequest"       },
  {(void **) &pDebugCaptureControl,     "debugCaptureControl"    },
  {(void **) &pNfcSecureGetList,        "nfcSecureGetList"       },
  {(void **) &pNfcSecureSetMode,        "nfcSecureSetMode"       },
  {(void **) &pNfcSecureSetBatteryMode, "nfcSecureSetBatteryMode"},
  {(void **) &pDebugSetFlagsEx,         "debugSetFlagsEx"        },
  {(void **) &pNfcOpenCLSocket,         "nfcP2POpenCLSocket"     },
  {(void **) &pNfcP2PCloseCLSocket,     "nfcP2PCloseCLSocket"    },
  {(void **) &pNfcP2PDataRequestTo,     "nfcP2PDataRequestTo"    },
  {(void **) &pNfcDeviceIoControl,      "nfcDeviceIoControl"     },
  {(void **) &pNfcSecureSetAidTable,    "nfcSecureSetAidTable"   },
  {(void **) &pNfcSecureGetAidTable,    "nfcSecureGetAidTable"   },
  {(void **) &pDebugSetOutputHandler,   "debugSetOutputHandler"  },
  {(void **) &pNfcSecureOpen,           "nfcSecureOpen"  },
  {(void **) &pNfcSecureClose,          "nfcSecureClose"  },
  {(void **) &pNfcSecureExchangePdu,    "nfcSecureExchangePdu"  },
};

#define MAX_SO_PATH  256
static char  SO_NAME[MAX_SO_PATH + 1];
static void *fd = 0;

/*
 * These functions allow the Linux behavior to emulate
 * the Windows behavior as specified below in the Windows
 * support section.
 *
 * First, search for the shared object in the application
 * binary path, then in the current working directory.
 *
 * Searching the application binary path requires /proc
 * filesystem support, which is standard in 2.4.x kernels.
 *
 * If the /proc filesystem is not present, the shared object
 * will not be loaded from the execution path unless that path
 * is either the current working directory or explicitly
 * specified in LD_LIBRARY_PATH.
 */
static int osCheckPath (const char *path)
{
  char *filename = (char *) malloc(strlen(path) + 1 + strlen(SO_NAME) + 1);
  int   fd;

  if (filename == NULL)
  {
    return 0;
  }

  // Check if the file is readable
  sprintf(filename, "%s/%s", path, SO_NAME);
  fd = open(filename, O_RDONLY);
  if (fd >= 0)
  {
    strncpy(SO_NAME, filename, MAX_SO_PATH);
    close(fd);
  }
  else
  {
    LOGV("osCheckPath failed <%s>", filename);
  }

  // Clean up and exit
  free(filename);
  return (fd >= 0);
} // osCheckPath

/*------------------------------------------------------------------*/
/**
 * @fn     osGetExecPath
 *
 * @brief  get path of current executable
 *
 * @return
 */
/*------------------------------------------------------------------*/
static int osGetExecPath (char *path, unsigned long maxlen)
{
  return readlink("/proc/self/exe", path, maxlen);
}

/*------------------------------------------------------------------*/
/**
 * @fn     _setSearchPath
 *
 * @brief   internal helper
 *
 * @return
 */
/*------------------------------------------------------------------*/
static void _setSearchPath (void)
{
  char  path[MAX_SO_PATH + 1];
  int   count;
  char *p;

  /* Make sure that SO_NAME is not an absolute path. */
  if (SO_NAME[0] == '/')
  {
    return;
  }

  /* Check the execution directory name. */
  memset(path, 0, sizeof(path));
  count = osGetExecPath(path, MAX_SO_PATH);

  if (count > 0)
  {
    LOGV("_setSearchPath <%s>", path);
    char *p = strrchr(path, '/');
    if (p == path)
    {
      ++p;
    }
    if (p != 0)
    {
      *p = '\0';
    }

    /* If there is a match, return immediately. */
    if (osCheckPath(path))
    {
      return;
    }
  }

  /* Check the current working directory. */
  p = getcwd(path, MAX_SO_PATH);
  if (p != 0)
  {
    LOGV("getCWD <%s>", path);
    osCheckPath(path);
  }
} // _setSearchPath

/*------------------------------------------------------------------*/
/**
 * @fn     osLoadLibrary
 *
 * @brief  LoadLibrary/dlopen abstraction
 *
 * @return
 */
/*------------------------------------------------------------------*/
static HANDLE osLoadLibrary (const char *pszLibraryName)
{
  HANDLE hLib;

  SO_NAME[0] = 0;
  getcwd(SO_NAME, MAX_SO_PATH);
  LOGV("current path <%s>", SO_NAME);

  strcpy(SO_NAME, pszLibraryName);
  _setSearchPath();

  if (!strstr(SO_NAME, ".so"))
  {
    strcat(SO_NAME, ".so");
  }

  LOGV("load library <%s>", SO_NAME);

  hLib = dlopen(SO_NAME, RTLD_NOW);

  if (hLib == NULL)
  {
    LOGV("error on dlopen: %s, err=%s", SO_NAME, dlerror());
  }

  return hLib;
} // osLoadLibrary

/*------------------------------------------------------------------*/
/**
 * @fn    nfcLoadLibrary
 *
 * @brief Load NFC lower access layer.
 *
 * @param void
 *
 *
 * @return NFCSTATUS
 */
/*------------------------------------------------------------------*/
NFCSTATUS nfcLoadLibrary (void)
{
  int error = 0;
  int i;

  LOG("load libstmnfc.so");

  // TODO change
  // Remember this is only valid for installable APK packets

  fd = osLoadLibrary("libstmnfc.so");
  if (fd == 0)
  {
    fd = osLoadLibrary("/system/lib/libstmnfc.so");
    if (fd == 0)
    {
      return nfcStatusLowLayerNotFound;
    }
  }

  // import all symbols:
  for (i = 0; i < (int) (sizeof(ImportTable) / sizeof(ImportTable[0])); i++)
  {
    void *func = dlsym(fd, ImportTable[i].funcName);
    if (func == 0)
    {
      LOGV("! unable to import dynamic symbol '%s'", ImportTable[i].funcName);
      error++;
    }
    else
    {
      // LOGV ("imported dynamic symbol '%s'", ImportTable[i].funcName);
    }

    *ImportTable[i].funcPtr = func;
  }

  /* TODO Add legacy access function pointer here , (P2P, card emulation, ...) */

  if (error)
  {
    LOGE("! nfcapi interface %s", "function missing");
    return nfcStatusFailed;
  }
  
  pDebugSetOutputHandler (threadsaveOutputProc);

  return nfcStatusSuccess;
} // nfcLoadLibrary

/*------------------------------------------------------------------*/
/**
 * @fn     nfcFreeLibrary
 *
 * @brief  unload shared library and clear all function pointers
 *
 * @return
 */
/*------------------------------------------------------------------*/
void nfcFreeLibrary (void)
{
  int i;

  if (fd != NULL)
  {
    LOG("Free libstmnfc.so");
    dlclose(fd);
    fd = NULL;

    // clear dangling function pointers
    for (i = 0; i < (int) (sizeof(ImportTable) / sizeof(ImportTable[0])); i++)
    {
      *ImportTable[i].funcPtr = 0;
    }
  }
} // nfcFreeLibrary
}
