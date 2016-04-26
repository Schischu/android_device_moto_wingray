# Copyright (C) 2012 The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)
# HAL module implemenation, not prelinked and stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SHARED_LIBRARIES := liblog libcutils libhardware \
    libhardware_legacy libutils libdl

LOCAL_SRC_FILES := hwc_tegra2.cpp

# Workaround for buggy Samsung Tegra 2 hwcomposer
ifeq ($(BOARD_HAVE_SAMSUNG_T20_HWCOMPOSER),true)
	LOCAL_CFLAGS += -DSAMSUNG_T20_HWCOMPOSER
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_CPP_INCLUDES += $(LOCAL_PATH)

LOCAL_MODULE := hwcomposer.tegra_2
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
