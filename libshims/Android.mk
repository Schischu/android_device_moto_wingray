# Copyright (C) 2015 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

#
# Shim for camera and audio
#     Shims VectorImpl symbols

include $(CLEAR_VARS)

LOCAL_SRC_FILES := RefBase.cpp \
                   SharedBuffer.cpp \
                   VectorImpl.cpp \

LOCAL_MODULE := libstingrayutl
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += external/safe-iop/include
LOCAL_SHARED_LIBRARIES := libbacktrace libcutils libdl liblog
LOCAL_CLANG := false

include $(BUILD_SHARED_LIBRARY)


#
# Shim for libsec-ril-apalone.so
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    libp4shim.cpp

LOCAL_SHARED_LIBRARIES := libbinder libcutils libstdc++

LOCAL_MODULE := libstingrayshim
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SHARED_LIBRARIES := libstingrayutl libbinder libcutils
LOCAL_CLANG := false

include $(BUILD_SHARED_LIBRARY)
