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
USE_PROPRIETARY_AUDIO_EXTENSIONS := false

# Use a smaller subset of system fonts to keep image size lower
SMALLER_FONT_FOOTPRINT := true

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a
TARGET_ARCH_VARIANT_CPU := cortex-a9
TARGET_CPU_VARIANT := generic

# Avoid the generation of ldrcc instructions
NEED_WORKAROUND_CORTEX_A9_745320 := true

# DO NOT change the following line to vfpv3 as it is not really supported on our device!
TARGET_ARCH_VARIANT_FPU := vfpv3-d16
ARCH_ARM_HAVE_TLS_REGISTER := true
ARCH_ARM_USE_NON_NEON_MEMCPY := true


TARGET_BOARD_PLATFORM := tegra
TARGET_TEGRA_VERSION := ap20
TARGET_BOARD_PLATFORM_GPU := tegra
TARGET_USERIMAGES_USE_EXT4 := true


BOARD_KERNEL_PAGESIZE := 2048
BOARD_KERNEL_BASE := 0x10000000
BOARD_KERNEL_CMDLINE := androidboot.carrier=wifi-only product_type=w
KERNEL_MODULES_DIR := /system/lib/modules
TARGET_KERNEL_SOURCE := device/moto/stingray-kernel
TARGET_KERNEL_CONFIG := stingray_defconfig
TARGET_KERNEL_CUSTOM_TOOLCHAIN := arm-eabi-4.6
# TARGET_PREBUILT_KERNEL := device/moto/wingray/kernel

# Filesystem
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1056858112
BOARD_USERDATAIMAGE_PARTITION_SIZE := 29859115008
BOARD_FLASH_BLOCK_SIZE := 4096
# Use this flag if the board has a ext4 partition larger than 2gb
BOARD_HAS_LARGE_FILESYSTEM := true


TARGET_NO_KERNEL := false
TARGET_NO_RECOVERY := false
TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true


# Audio
BOARD_USES_GENERIC_AUDIO := false
BOARD_OMX_NEEDS_LEGACY_AUDIO := true

# Camera
USE_CAMERA_STUB := false

# Graphics
USE_OPENGL_RENDERER := true
TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK := true
BOARD_EGL_WORKAROUND_BUG_10194508 := true
SKIP_SET_METADATA := true
BOARD_USE_MHEAP_SCREENSHOT := true
BOARD_USES_HWCOMPOSER := true
BOARD_EGL_NEEDS_LEGACY_FB := true

# Enable WEBGL in WebKit
ENABLE_WEBGL := true

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true
# BOARD_BLUEDROID_VENDOR_CONF :=
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/moto/wingray/bluetooth



TARGET_BOARD_INFO_FILE := device/moto/wingray/board-info.txt


# Wifi-related defines
WPA_BUILD_SUPPLICANT        	:= true
WPA_SUPPLICANT_VERSION      	:= VER_0_8_X
BOARD_WLAN_DEVICE           	:= bcmdhd

BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)

#WIFI_DRIVER_MODULE_PATH     := "/system/lib/modules/bcm4329.ko"
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/bcmdhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA     := "/vendor/firmware/fw_bcmdhd.bin"
WIFI_DRIVER_FW_PATH_AP      := "/vendor/firmware/fw_bcmdhd_apsta.bin"
#WIFI_DRIVER_FW_PATH_MFG     := "/vendor/firmware/fw_bcmdhd_mfg.bin"


# Vold
BOARD_VOLD_MAX_PARTITIONS := 12
BOARD_VOLD_EMMC_SHARES_DEV_MAJOR := true
BOARD_VOLD_DISC_HAS_MULTIPLE_MAJORS := true
BOARD_USE_USB_MASS_STORAGE_SWITCH := true
TARGET_USE_CUSTOM_LUN_FILE_PATH := "/sys/devices/platform/fsl-tegra-udc/gadget/lun%d/file"

BOARD_HAS_SDCARD_INTERNAL := true

BOARD_UMS_LUNFILE := "/sys/devices/platform/fsl-tegra-udc/gadget/lun%d/file"
BOARD_USES_MMCUTILS := true
BOARD_HAS_NO_SELECT_BUTTON := true

# Misc flags
COMMON_GLOBAL_CFLAGS += -DNEEDS_VECTORIMPL_SYMBOLS

# Avoid the generation of ldrcc instructions
NEED_WORKAROUND_CORTEX_A9_745320 := true

TARGET_ARCH_LOWMEM := true

TARGET_RECOVERY_PIXEL_FORMAT := "RGB_565"
TARGET_RECOVERY_UI_LIB := librecovery_ui_stingray

TARGET_RECOVERY_FSTAB = device/moto/wingray/fstab.stingray

# BOARD_USES_SECURE_SERVICES := false

# BOARD_USES_EXTRA_THERMAL_SENSOR := false

# Compatibility with pre-kitkat Sensor HALs
SENSORS_NEED_SETRATE_ON_ENABLE := true

# BOARD_LIB_DUMPSTATE := libdumpstate.wingray
TARGET_RELEASETOOLS_EXTENSIONS := device/moto/wingray

-include vendor/moto/stingray/BoardConfigVendor.mk
#TWRP config
DEVICE_RESOLUTION := 720x1280
RECOVERY_SDCARD_ON_DATA := true
RECOVERY_GRAPHICS_USE_LINELENGTH := true
PRODUCT_BUILD_PROP_OVERRIDES += BUILD_UTC_DATE=0
#TARGET_RECOVERY_INITRC := device/moto/wingray/init.recovery.rc
#TW_INCLUDE_CRYPTO := true
TW_INCLUDE_JB_CRYPTO := true
TW_FLASH_FROM_STORAGE := true
TW_NO_USB_STORAGE := true

TW_INTERNAL_STORAGE_PATH := "/data/media"
TW_INTERNAL_STORAGE_MOUNT_POINT := "data"
TW_EXTERNAL_STORAGE_PATH := "/usb-otg"
TW_EXTERNAL_STORAGE_MOUNT_POINT := "usb-otg"
