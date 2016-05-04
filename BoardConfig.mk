# Copyright (C) 2010 The Android Open Source Project
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
#


-include vendor/broadcom/wingray/BoardConfigVendor.mk
-include vendor/moto/wingray/BoardConfigVendor.mk
-include vendor/nvidia/wingray/BoardConfigVendor.mk

DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a
TARGET_ARCH_VARIANT_CPU := cortex-a9
TARGET_CPU_VARIANT := tegra2

TARGET_ARCH_VARIANT_FPU := vfpv3-d16

TARGET_BOARD_PLATFORM := tegra

TARGET_USERIMAGES_USE_EXT4 := true

# Kernel
BOARD_PAGE_SIZE := 2048
BOARD_KERNEL_BASE := 0x10000000
BOARD_KERNEL_CMDLINE := androidboot.carrier=wifi-only product_type=w selinux=0
TARGET_KERNEL_SOURCE := kernel/moto/stingray-kernel
TARGET_KERNEL_CONFIG := elementalxvanilla_defconfig

#TARGET_PREBUILT_KERNEL := device/moto/wingray/kernel

# Healthd
BOARD_AC_SYSFS_PATH := $(BOARD_POWER_SUPPLY_PATH)/ac
BOARD_BATTERY_SYSFS_PATH := $(BOARD_POWER_SUPPLY_PATH)/battery
BOARD_CHARGER_DIM_SCREEN_BRIGHTNESS := true

# Filesystem
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1056858112
BOARD_USERDATAIMAGE_PARTITION_SIZE := 29859115008
BOARD_FLASH_BLOCK_SIZE := 4096
# Use this flag if the board has a ext4 partition larger than 2gb
BOARD_HAS_LARGE_FILESYSTEM := true

TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true

# Audio
#AUDIO_LEGACY_HACK := true
BOARD_USES_GENERIC_AUDIO := false
# Omnirom has added omx legacy audio with flag BOARD_NEED_OMX_COMPAT
BOARD_OMX_NEEDS_LEGACY_AUDIO := true
BOARD_NEED_OMX_COMPAT := true
BOARD_NEED_OMX_COMPAT_XOOM := true
BOARD_USB_AUDIO_CARD_ID := 0
# USE_PROPRIETARY_AUDIO_EXTENSIONS := false
# BOARD_USES_PROPRIETARY_OMX := TF101

# HAVE_PRE_KITKAT_AUDIO_BLOB := true
# BOARD_HAVE_PRE_KITKAT_AUDIO_POLICY_BLOB := true
#USE_LEGACY_AUDIO_POLICY := 1
TARGET_LEGACY_UNSUPPORTED_LIBAUDIO := true #Else we would have to implement audioPatch and AudioPort

# Camera
USE_CAMERA_STUB := false
# ICS_CAMERA_BLOB := true
#X BOARD_CAMERA_HAVE_ISO := true

# Graphics
USE_OPENGL_RENDERER := true
BOARD_EGL_WORKAROUND_BUG_10194508 := true
SKIP_SET_METADATA := true
BOARD_USE_MHEAP_SCREENSHOT := true

#SAMSUNG_T20_HWCOMPOSER: It seems that the samsung hwc has a bug, so this define should only be needed on p3/p4
#BOARD_HAVE_SAMSUNG_T20_HWCOMPOSER := true
#BOARD_EGL_NEEDS_LEGACY_FB := true
BOARD_NEEDS_OLD_HWC_API := true

BOARD_EGL_CFG := device/moto/wingray/egl.cfg
BOARD_EGL_SKIP_FIRST_DEQUEUE := true

MAX_EGL_CACHE_KEY_SIZE := 4096
MAX_EGL_CACHE_SIZE := 2146304
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3

TARGET_32_BIT_SURFACEFLINGER := true

BOARD_NEEDS_LOWFPS_BOOTANI := true

BOARD_USE_SKIA_LCDTEXT := true
SMALLER_FONT_FOOTPRINT := true

TARGET_BOOTANIMATION_PRELOAD := true
TARGET_BOOTANIMATION_TEXTURE_CACHE := true
TARGET_BOOTANIMATION_USE_RGB565 := true
TARGET_BOOTLOADER_BOARD_NAME := stingray

# PRODUCT_PREBUILT_WEBVIEWCHROMIUM := yes

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/moto/wingray/bluetooth

# Wifi-related defines
WPA_BUILD_SUPPLICANT        	:= true
WPA_SUPPLICANT_VERSION      	:= VER_0_8_X
BOARD_WLAN_DEVICE           	:= bcmdhd

BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)

#A WIFI_DRIVER_MODULE_PATH     := "/system/lib/modules/dhd.ko"
#A WIFI_DRIVER_MODULE_NAME     := "dhd"
#A WIFI_DRIVER_MODULE_ARG      := "iface_name=wlan0 firmware_path=/system/etc/wifi/bcmdhd_sta.bin nvram_path=/system/etc/wifi/nvram_net.txt"
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/bcmdhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA     := "/system/vendor/firmware/fw_bcmdhd.bin"
WIFI_DRIVER_FW_PATH_AP      := "/system/vendor/firmware/fw_bcmdhd_apsta.bin"
#A WIFI_DRIVER_FW_PATH_P2P     := "/system/etc/wifi/bcmdhd_p2p.bin"

#A BOARD_LEGACY_NL80211_STA_EVENTS := true

BOARD_HAS_SDCARD_INTERNAL := true

BOARD_HAS_NO_SELECT_BUTTON := true

# Misc flags
ARCH_ARM_HIGH_OPTIMIZATION := true
ARCH_ARM_HAVE_TLS_REGISTER := true
ARCH_ARM_USE_NON_NEON_MEMCPY := true
MALLOC_IMPL := dlmalloc
COMMON_GLOBAL_CFLAGS += -DNEEDS_VECTORIMPL_SYMBOLS #flash compatibility
TARGET_ENABLE_NON_PIE_SUPPORT := true

COMMON_GLOBAL_CFLAGS += -DREFBASE_JB_MR1_COMPAT_SYMBOLS #system/core/libutils/RefBase.cpp

# COMMON_GLOBAL_CFLAGS += -DICS_AUDIO_BLOB #system/core/include/system/audio.h

BOARD_SUPPRESS_EMMC_WIPE := true
TARGET_RECOVERY_PIXEL_FORMAT := "RGBX_8888"

RECOVERY_FSTAB_VERSION := 2
#TARGET_RECOVERY_FSTAB = device/moto/wingray/recovery/twrp.fstab
TARGET_RECOVERY_FSTAB = device/moto/wingray/rootdir/fstab.stingray

#TARGET_ENABLE_OFFLOAD_ENHANCEMENTS := true

#TARGET_RELEASETOOLS_EXTENSIONS := device/moto/wingray
#TWRP config
DEVICE_RESOLUTION := 1280x800
RECOVERY_SDCARD_ON_DATA := true
WIPE_IS_SUPPORTED := false

TW_ALWAYS_RMRF := true
#TW_DISABLE_TTF := true #Has been deprecated
#TW_USE_TOOLBOX := true #causes weirdness. do not use. kill supersu instead

TW_HAS_DOWNLOAD_MODE := true
TW_NO_REBOOT_BOOTLOADER := true

# Suppress EMMC WIPE
SUPPRESS_EMMC_WIPE := true
SUPPRESS_SECURE_ERASE := true

TW_NO_EXFAT := true
TW_NO_EXFAT_FUSE := true

TW_FLASH_FROM_STORAGE := true
TW_NO_USB_STORAGE := true
TW_DEFAULT_EXTERNAL_STORAGE := true

TW_EXCLUDE_SUPERSU := true

TW_BRIGHTNESS_PATH := "/sys/devices/platform/tegra-i2c.0/i2c-0/0-002c/leds/lcd-backlight/brightness"
TW_MAX_BRIGHTNESS := 255

TW_INTERNAL_STORAGE_PATH := "/data/media"
TW_INTERNAL_STORAGE_MOUNT_POINT := "data"
TW_EXTERNAL_STORAGE_PATH := "/sdcard"
TW_EXTERNAL_STORAGE_MOUNT_POINT := "sdcard"

HAVE_SELINUX := true
#WITH_DEXPREOPT := true
