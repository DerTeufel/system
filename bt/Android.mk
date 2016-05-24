LOCAL_PATH := $(call my-dir)

# Setup bdroid local make variables for handling configuration
ifneq ($(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR),)
  bdroid_C_INCLUDES := $(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR)
  bdroid_CFLAGS += -DHAS_BDROID_BUILDCFG
else
  bdroid_C_INCLUDES :=
  bdroid_CFLAGS += -DHAS_NO_BDROID_BUILDCFG
endif

ifneq ($(BOARD_BLUETOOTH_BDROID_HCILP_INCLUDED),)
  bdroid_CFLAGS += -DHCILP_INCLUDED=$(BOARD_BLUETOOTH_BDROID_HCILP_INCLUDED)
endif

ifneq ($(TARGET_BUILD_VARIANT),user)
bdroid_CFLAGS += -DBLUEDROID_DEBUG
endif

bdroid_CFLAGS += \
  -Wall \
  -Wno-unused-parameter \
  -Wunused-but-set-variable \
  -UNDEBUG \
  -DLOG_NDEBUG=1

ifeq ($(MTK_BT_BLUEDROID_PLUS), yes)
mdroid_CFLAGS += \
  -DMTK_BT_COMMON

ifeq ($(MTK_BT_BLUEDROID_AVRCP_TG_16), yes)
mdroid_CFLAGS += \
  -DMTK_BT_AVRCP_TG_16
endif

ifeq ($(MTK_BT_BLUEDROID_AVRCP_TG_15), yes)
mdroid_CFLAGS += \
  -DMTK_BT_AVRCP_TG_15
endif

ifeq ($(MTK_BT_BLUEDROID_HFP_AG_17), yes)
mdroid_CFLAGS += \
  -DMTK_BT_HFP_AG_17
endif

ifeq ($(LINUX_KERNEL_VERSION),kernel-3.18)
mdroid_CFLAGS += \
  -DMTK_BT_KERNEL_3_18
endif

ifeq ($(MTK_BT_BLUEDROID_A2DP_APTX), yes)
mdroid_CFLAGS += \
  -DMTK_BT_A2DP_SRC_APTX_CODEC

CSR_VENDOR_PATH = $(TOP)/vendor/csr/aptX
$(shell mkdir -p $(TARGET_OUT_SHARED_LIBRARIES)/../../obj/lib)
$(shell mkdir -p $(TARGET_OUT_SHARED_LIBRARIES)/../../obj/SHARED_LIBRARIES/libbt-aptX-ARM-4.2.2_intermediates/export_includes)
$(shell mkdir -p $(TARGET_OUT_SHARED_LIBRARIES))

$(shell cp $(CSR_VENDOR_PATH)/libbt-aptX-ARM-4.2.2.so $(TARGET_OUT_SHARED_LIBRARIES)/libbt-aptX-ARM-4.2.2.so)
$(shell cp $(CSR_VENDOR_PATH)/libbt-aptX-ARM-4.2.2.so $(TARGET_OUT_SHARED_LIBRARIES)/../../obj/lib/libbt-aptX-ARM-4.2.2.so)
$(shell cp $(CSR_VENDOR_PATH)/libbt-aptX-ARM-4.2.2.so $(TARGET_OUT_SHARED_LIBRARIES)/../../obj/SHARED_LIBRARIES/libbt-aptX-ARM-4.2.2_intermediates/export_includes/libbt-aptX-ARM-4.2.2.so)

LOCAL_MODULE := bt-aptX
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := libbt-aptX-ARM-4.2.2.so
include $(PREBUILT_SHARED_LIBRARY)
endif #MTK_BT_BLUEDROID_A2DP_APTX

ifeq ($(MTK_BT_BLUEDROID_DUN_GW_12), yes)
mdroid_CFLAGS += \
  -DMTK_BT_DUN_GW_12
endif

ifneq ($(TARGET_BUILD_VARIANT), user)
mdroid_CFLAGS += \
  -DMTK_BT_NET_DEBUG
endif

# enable default override of stack config only in userdebug and eng builds
ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
mdroid_CFLAGS += \
  -DMTK_BT_DEFAULT_OVERRIDE
endif

endif #end of ifeq ($(MTK_BT_BLUEDROID_PLUS), yes)

# To include mdroid_buildcfg.h
mdroid_CFLAGS += \
  -DHAS_MDROID_BUILDCFG

bdroid_CFLAGS += $(mdroid_CFLAGS)

include $(call all-subdir-makefiles)

# Cleanup our locals
bdroid_C_INCLUDES :=
bdroid_CFLAGS :=
