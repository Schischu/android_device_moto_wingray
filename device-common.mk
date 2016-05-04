# Copyright (C) 2011 The Android Open-Source Project
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

DEVICE_PACKAGE_OVERLAYS := $(LOCAL_PATH)/overlay
    
PRODUCT_AAPT_CONFIG := xlarge mdpi hdpi
PRODUCT_AAPT_PREF_CONFIG := mdpi

PRODUCT_COPY_FILES := \
    $(LOCAL_PATH)/rootdir/init.stingray.rc:root/init.stingray.rc \
    $(LOCAL_PATH)/rootdir/fstab.stingray:root/fstab.stingray \
    $(LOCAL_PATH)/rootdir/ueventd.stingray.rc:root/ueventd.stingray.rc \
    $(LOCAL_PATH)/rootdir/twrp.fstab:recovery/root/etc/twrp.fstab \
    $(LOCAL_PATH)/rootdir/init.stingray.usb.rc:root/init.stingray.usb.rc


PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/fw/mXT1386_08_AA.bin:system/etc/firmware/mXT1386_08_AA.bin \
    $(LOCAL_PATH)/fw/mXT1386_08_E1.bin:system/etc/firmware/mXT1386_08_E1.bin \
    $(LOCAL_PATH)/fw/mXT1386_09_AA.bin:system/etc/firmware/mXT1386_09_AA.bin \
    $(LOCAL_PATH)/fw/mXT1386_10_AA.bin:system/etc/firmware/mXT1386_10_AA.bin \
    $(LOCAL_PATH)/fw/mXT1386_10_FF.bin:system/etc/firmware/mXT1386_10_FF.bin

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/media_profiles.xml:system/etc/media_profiles.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:system/etc/media_codecs_google_telephony.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml \
    $(LOCAL_PATH)/media_codecs.xml:system/etc/media_codecs.xml

PRODUCT_COPY_FILES += \
     $(LOCAL_PATH)/gps.conf:system/etc/gps.conf

#PRODUCT_COPY_FILES += \
#     $(LOCAL_PATH)/97random:system/etc/init.d/97random

PRODUCT_PROPERTY_OVERRIDES := \
    wifi.interface=wlan0 \
    wifi.supplicant_scan_interval=15

PRODUCT_PROPERTY_OVERRIDES += \
    media.stagefright.cache-params=6144/-1/30 \
    ro.sf.lcd_density=160 \
    dalvik.vm.dex2oat-flags=--compiler-backend=Quick \
    dalvik.vm.backgroundgctype=SS \
    debug.hwui.render_dirty_regions=false \
    ro.zygote.disable_gl_preload=true \
    ro.opengles.surface.rgb565=true \
     persist.sys.media.legacy-drm=true

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp

#why the FUUUUUUUUUUUCK is wpa_supplicant no longer default
#what a major ass pain
#it takes up 1mb. what kind of storage doesn't have 1 fucking megabyte to spare???!!!!!
PRODUCT_PACKAGES := \
	libwpa_client \
	hostapd \
	wpa_supplicant \
	dhcpcd.conf \
	su

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/rootdir/init.recovery.stingray.rc:root/init.recovery.stingray.rc

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.root_access=1

#PRODUCT_COPY_FILES += \

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
    $(LOCAL_PATH)/wifi/wpa_supplicant_overlay.conf:system/etc/wifi/wpa_supplicant_overlay.conf \
    $(LOCAL_PATH)/wifi/p2p_supplicant_overlay.conf:system/etc/wifi/p2p_supplicant_overlay.conf \
    $(LOCAL_PATH)/wifi/hostapd_default.conf:system/etc/hostapd/hostapd_default.conf

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/idc/qtouch-touchscreen.idc:system/usr/idc/qtouch-touchscreen.idc \
    $(LOCAL_PATH)/keychars/cpcap-key.kcm:system/usr/keychars/cpcap-key.kcm \
    $(LOCAL_PATH)/keychars/stingray-keypad.kcm:system/usr/keychars/stingray-keypad.kcm \
    $(LOCAL_PATH)/keylayout/cpcap-key.kl:system/usr/keylayout/cpcap-key.kl \
    $(LOCAL_PATH)/keylayout/stingray-keypad.kl:system/usr/keylayout/stingray-keypad.kl

PRODUCT_PACKAGES += \
    hwcomposer.tegra

# Audio
PRODUCT_COPY_FILES += \
        device/moto/wingray/audio/audio_policy.conf:system/etc/audio_policy.conf

PRODUCT_PACKAGES += \
        audio.primary.stingray \
        audio_policy.stingray \
        audio.a2dp.default \
        libaudioutils \
        libtinyalsa

PRODUCT_PACKAGES += \
        lights.stingray

PRODUCT_PACKAGES += \
        sensors.stingray

PRODUCT_PACKAGES += \
    whisperd

# These are the hardware-specific features
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.location.xml:system/etc/permissions/android.hardware.location.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.sensor.barometer.xml:system/etc/permissions/android.hardware.sensor.barometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/bluetooth/bt_vendor.conf:system/etc/bluetooth/bt_vendor.conf

PRODUCT_CHARACTERISTICS := tablet

PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_PACKAGES += \
    librs_jni \
    com.android.future.usb.accessory \
    power.stingray \
    libnetcmdiface

# Preload compability symbols
PRODUCT_PACKAGES += \
    libp4utl

# Filesystem management tools
PRODUCT_PACKAGES += \
	make_ext4fs \
	setup_fs

$(call inherit-product, frameworks/native/build/tablet-dalvik-heap.mk)

$(call inherit-product, vendor/broadcom/wingray/wingray-vendor.mk)
$(call inherit-product, vendor/moto/wingray/wingray-vendor.mk)
$(call inherit-product, vendor/nvidia/wingray/wingray-vendor.mk)

$(call inherit-product, vendor/google/google-vendor.mk)
