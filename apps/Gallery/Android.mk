LOCAL_PATH:= $(call my-dir)
ifeq ($(HAVE_FSL_IMX_GPU3D),false)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := Gallery
LOCAL_CERTIFICATE := media

include $(BUILD_PACKAGE)

# Use the following include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
