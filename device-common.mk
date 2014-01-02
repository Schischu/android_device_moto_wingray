#
# Copyright (C) 2013 The Android Open Source Project
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

# This file includes all definitions that apply to ALL wingray devices, and
# are also specific to wingray devices
#
# Everything in this directory will become public

# This device is xhdpi.  However the platform doesn't
# currently contain all of the bitmaps at xhdpi density so
# we do this little trick to fall back to the hdpi version
# if the xhdpi doesn't exist.
PRODUCT_AAPT_CONFIG := normal hdpi xhdpi
PRODUCT_AAPT_PREF_CONFIG := xhdpi

PRODUCT_PACKAGES := \
    lights.stingray

PRODUCT_PACKAGES += \
    sensors.stingray


# LOCAL_KERNEL := device/moto/stingray-kernel/kernel

# PRODUCT_COPY_FILES := \
# 	$(LOCAL_KERNEL):kernel

PRODUCT_COPY_FILES := \
        device/moto/wingray/libaudio/audio_policy.conf:system/etc/audio_policy.conf

PRODUCT_COPY_FILES += \
    device/moto/wingray/init.stingray.rc:root/init.stingray.rc \
    device/moto/wingray/init.stingray.usb.rc:root/init.stingray.usb.rc \
    device/moto/wingray/fstab.stingray:root/fstab.stingray \
    device/moto/wingray/ueventd.stingray.rc:root/ueventd.stingray.rc \
    device/moto/wingray/media_profiles.xml:system/etc/media_profiles.xml \
    device/moto/wingray/media_codecs.xml:system/etc/media_codecs.xml

PRODUCT_COPY_FILES += \
    device/moto/wingray/twrp.fstab:recovery/root/etc/twrp.fstab

# ifneq ($(TARGET_PREBUILT_WIFI_MODULE),)
# PRODUCT_COPY_FILES += \
#     $(TARGET_PREBUILT_WIFI_MODULE):system/lib/modules/bcm4329.ko
# endif

PRODUCT_COPY_FILES += \
    device/moto/wingray/mXT1386_08_AA.bin:system/etc/firmware/mXT1386_08_AA.bin \
    device/moto/wingray/mXT1386_08_E1.bin:system/etc/firmware/mXT1386_08_E1.bin \
    device/moto/wingray/mXT1386_09_AA.bin:system/etc/firmware/mXT1386_09_AA.bin \
    device/moto/wingray/mXT1386_10_AA.bin:system/etc/firmware/mXT1386_10_AA.bin \
    device/moto/wingray/mXT1386_10_FF.bin:system/etc/firmware/mXT1386_10_FF.bin

PRODUCT_COPY_FILES += \
        device/moto/wingray/qtouch-touchscreen.idc:system/usr/idc/qtouch-touchscreen.idc \
        device/moto/wingray/cpcap-key.kl:system/usr/keylayout/cpcap-key.kl \
        device/moto/wingray/cpcap-key.kcm:system/usr/keychars/cpcap-key.kcm \
        device/moto/wingray/stingray-keypad.kl:system/usr/keylayout/stingray-keypad.kl \
        device/moto/wingray/stingray-keypad.kcm:system/usr/keychars/stingray-keypad.kcm

# These are the hardware-specific features
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.barometer.xml:system/etc/permissions/android.hardware.sensor.barometer.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \

PRODUCT_PROPERTY_OVERRIDES += \
    ro.opengles.version=131072

PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=160

PRODUCT_CHARACTERISTICS := tablet

PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_PACKAGES += \
    librs_jni \
    com.android.future.usb.accessory

# Filesystem management tools
PRODUCT_PACKAGES += \
    e2fsck

PRODUCT_PACKAGES += \
    audio_policy.stingray \
    audio.primary.stingray \
    audio.a2dp.default \
    audio.usb.default \

PRODUCT_PACKAGES += \
    tctl \
    tplay \
    trec \
    twav \
    tdownsample

PRODUCT_PACKAGES += \
    l2ping \
    hcitool \
    bttest \
    whisperd

PRODUCT_PACKAGES += \
    libemoji

# Bluetooth config file
# PRODUCT_COPY_FILES += \
#     system/bluetooth/data/main.nonsmartphone.conf:system/etc/bluetooth/main.conf \


PRODUCT_PACKAGES += \
    camera.stingray

PRODUCT_PACKAGES += \
    wpa_supplicant_overlay.conf \
    p2p_supplicant_overlay.conf

PRODUCT_PROPERTY_OVERRIDES += \
    wifi.interface=wlan0

WIFI_BAND := 802_11_ABG

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp

PRODUCT_PROPERTY_OVERRIDES += \
    ro.board.platform=tegra \
    debug.hwui.render_dirty_regions=false \
    ro.zygote.disable_gl_preload=true \
    ro.bq.gpu_to_cpu_unsupported=true \
    ro.opengles.surface.rgb565=true

$(call inherit-product, frameworks/native/build/tablet-dalvik-heap.mk)
$(call inherit-product, hardware/broadcom/wlan/bcmdhd/firmware/bcm4329/device-bcm.mk)

