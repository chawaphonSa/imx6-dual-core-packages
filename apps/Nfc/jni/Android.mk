LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS    += -DTARGET_TYPE_LINUX
LOCAL_CFLAGS    += -DF_DEBUGOUT
LOCAL_CFLAGS    += -DANDROID
LOCAL_CFLAGS    += -DFX_NO_SYSTEM_PROPERTY
LOCAL_CFLAGS    += -fno-exceptions -fno-rtti -Wno-psabi -Wno-format -Wall -Wunused -Wextra

ifdef NDK_ROOT
LOCAL_C_INCLUDES += ../nfcstack/jni/inc
LOCAL_LDLIBS    := -llog -ldl
else
LOCAL_C_INCLUDES += external/nfcstack/inc
LOCAL_SHARED_LIBRARIES := liblog libdl
endif

LOCAL_MODULE := libnfc_jni

LOCAL_REQUIRED_MODULES := libstmnfc
 
LOCAL_SRC_FILES:= \
    com_android_nfc_NativeSignalNotifications.cpp \
    com_android_nfc_NativeLlcpServiceSocket.cpp \
    com_android_nfc_NativeLlcpSocket.cpp \
    com_android_nfc_NativeNfcSecureElement.cpp \
    com_android_nfc_NativeNfcManager.cpp \
    com_android_nfc_NativeNfcTag.cpp \
    com_android_nfc_NativeP2pDevice.cpp \
    com_android_nfc_ApiExtension.cpp \
    com_android_nfc.cpp \
    udsServer.cpp \
    nfcInit.cpp \
    nfcSocketList.cpp \
    ringbuffer.cpp \
    st21nfca_sefix.cpp

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
