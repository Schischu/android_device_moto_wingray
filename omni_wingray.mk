# Inherit device configuration for wingray.
$(call inherit-product, device/moto/wingray/wingray.mk)

# Inherit some common cyanogenmod stuff.
$(call inherit-product, vendor/omni/config/common.mk)

#
# Setup device specific product configuration.
#
PRODUCT_NAME := omni_wingray
PRODUCT_BRAND := motorola
PRODUCT_DEVICE := wingray
PRODUCT_MODEL := Xoom Wifi
PRODUCT_MANUFACTURER := motorola
#PRODUCT_BUILD_PROP_OVERRIDES += PRODUCT_NAME=GT-P7510 BUILD_FINGERPRINT=samsung/GT-P7510/GT-P7510:4.0.4/IMM76D/UELPL:user/release-keys PRIVATE_BUILD_DESC="GT-P7510-user 4.0.4 IMM76D UELPL release-keys"

# Release name and versioning
PRODUCT_RELEASE_NAME := wingray
