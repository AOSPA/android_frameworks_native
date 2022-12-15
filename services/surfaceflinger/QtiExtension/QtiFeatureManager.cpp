/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0
#include "QtiFeatureManager.h"

#include <android-base/properties.h>

namespace android::surfaceflingerextension {

std::shared_ptr<IDisplayConfig> mQtiAidl = nullptr;
::DisplayConfig::ClientInterface* mQtiHidl = nullptr;

QtiFeatureManager::QtiFeatureManager(QtiSurfaceFlingerExtension* extension) {
    mQtiSFExtension = extension;
}

void QtiFeatureManager::init() {
    ALOGV("Initializing QtiFeatureManager");

    string propName = "";

    propName = qtiGetPropName(QtiFeature::kHwcForWfd);
    mQtiAllowHwcForWFD = base::GetBoolProperty(propName, false);
    ALOGI_IF(mQtiAllowHwcForWFD, "Allow HWC for WFD");

    propName = qtiGetPropName(QtiFeature::kHwcForVds);
    mQtiAllowHwcForVDS = mQtiAllowHwcForWFD && base::GetBoolProperty(propName, false);
    ALOGI_IF(mQtiAllowHwcForVDS, "Allow HWC for VDS");

    propName = qtiGetPropName(QtiFeature::kDynamicSfIdle);
    mQtiEnableDynamicSfIdle = base::GetBoolProperty(propName, false);
    ALOGI_IF(mQtiEnableDynamicSfIdle, "Enable dynamic sf idle timer");

    propName = qtiGetPropName(QtiFeature::kEarlyWakeUp);
    mQtiEnableEarlyWakeUp = base::GetBoolProperty(propName, false);
    ALOGI_IF(mQtiEnableEarlyWakeUp, "Enable Early Wake Up");

    propName = qtiGetPropName(QtiFeature::kLatchMediaContent);
    mQtiLatchMediaContent = base::GetBoolProperty(propName, false);
    ALOGI_IF(mQtiLatchMediaContent, "Enable Latch Media Content");

    propName = qtiGetPropName(QtiFeature::kFbScaling);
    mQtiUseFbScaling = base::GetBoolProperty(propName, false);
    ALOGI_IF(mQtiUseFbScaling, "Enable FrameBuffer Scaling");

    propName = qtiGetPropName(QtiFeature::kAdvanceSfOffset);
    mQtiUseAdvanceSfOffset = base::GetBoolProperty(propName, false);
    ALOGI_IF(mQtiUseAdvanceSfOffset, "Enable Advance SF Phase Offset");

    propName = qtiGetPropName(QtiFeature::kWorkDurations);
    mQtiUseWorkDurations = base::GetBoolProperty(propName, false);
    ALOGI_IF(mQtiUseAdvanceSfOffset, "Enable Work Durations");

    propName = qtiGetPropName(QtiFeature::kLayerExtension);
    mQtiUseLayerExt = base::GetBoolProperty(propName, false);
    ALOGI_IF(mQtiUseLayerExt, "Enable Layer Extension");

    propName = qtiGetPropName(QtiFeature::kVsyncSourceReliableOnDoze);
    mQtiVsyncSourceReliableOnDoze = base::GetBoolProperty(propName, false);

    ALOGI_IF(mQtiVsyncSourceReliableOnDoze, "Enable Vsync Source Reliable on Doze");

    propName = qtiGetPropName(QtiFeature::kPluggableVsyncPrioritized);
    mQtiPluggableVsyncPrioritized = base::GetBoolProperty(propName, false);

    ALOGI_IF(mQtiPluggableVsyncPrioritized, "Prioritize pluggable displays");

    propName = qtiGetPropName(QtiFeature::kQsyncIdle);
    mQtiUseQsyncIdle = base::GetBoolProperty(propName, false);
    if (mQtiAidl && mQtiUseQsyncIdle) {
        mQtiAidl->controlIdleStatusCallback(mQtiUseQsyncIdle);
        ALOGI("Enable Qsync Idle");
    }
}

void QtiFeatureManager::qtiSetIDisplayConfig(std::shared_ptr<IDisplayConfig> aidl) {
    mQtiAidl = aidl;
}

void QtiFeatureManager::qtiSetIDisplayConfig(::DisplayConfig::ClientInterface* hidl) {
    mQtiHidl = hidl;
}

bool QtiFeatureManager::qtiIsExtensionFeatureEnabled(QtiFeature feature) {
    switch (feature) {
        case QtiFeature::kAdvanceSfOffset:
            return mQtiUseAdvanceSfOffset;
        case QtiFeature::kDynamicSfIdle:
            return mQtiEnableDynamicSfIdle;
        case QtiFeature::kEarlyWakeUp:
            return mQtiEnableEarlyWakeUp;
        case QtiFeature::kFbScaling:
            return mQtiUseFbScaling;
        case QtiFeature::kHwcForVds:
            return mQtiAllowHwcForVDS;
        case QtiFeature::kHwcForWfd:
            return mQtiAllowHwcForWFD;
        case QtiFeature::kLatchMediaContent:
            return mQtiLatchMediaContent;
        case QtiFeature::kLayerExtension:
            return mQtiUseLayerExt;
        case QtiFeature::kPluggableVsyncPrioritized:
            return mQtiPluggableVsyncPrioritized;
        case QtiFeature::kQsyncIdle:
            return mQtiUseQsyncIdle;
        case QtiFeature::kVsyncSourceReliableOnDoze:
            return mQtiVsyncSourceReliableOnDoze;
        case QtiFeature::kWorkDurations:
            return mQtiUseWorkDurations;
        default:
            ALOGW("Queried unknown SF extension feature %d", feature);
            return false;
    }
}

string QtiFeatureManager::qtiGetPropName(QtiFeature feature) {
    switch (feature) {
        case QtiFeature::kAdvanceSfOffset:
            return "debug.sf.enable_advanced_sf_phase_offset";
        case QtiFeature::kDynamicSfIdle:
            return "vendor.display.disable_dynamic_sf_idle";
        case QtiFeature::kEarlyWakeUp:
            return "vendor.display.enable_early_wakeup";
        case QtiFeature::kFbScaling:
            return "vendor.display.enable_fb_scaling";
        case QtiFeature::kHwcForVds:
            return "debug.sf.enable_hwc_vds";
        case QtiFeature::kHwcForWfd:
            return "vendor.display.vds_allow_hwc";
        case QtiFeature::kLatchMediaContent:
            return "vendor.display.enable_latch_media_content";
        case QtiFeature::kLayerExtension:
            return "vendor.display.use_layer_ext";
        case QtiFeature::kPluggableVsyncPrioritized:
            return "vendor.display.pluggable_vsync_prioritized";
        case QtiFeature::kQsyncIdle:
            return "vendor.display.enable_qsync_idle";
        case QtiFeature::kVsyncSourceReliableOnDoze:
            return "vendor.display.vsync_reliable_on_doze";
        case QtiFeature::kWorkDurations:
            return "debug.sf.use_phase_offsets_as_durations";
        default:
            ALOGW("Queried unknown SF extension feature %d", feature);
            return "";
    }
}

} // namespace android::surfaceflingerextension
