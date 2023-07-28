/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

// #define LOG_NDEBUG 0
#include "QtiAidlComposerHalExtension.h"
#include "QtiHWComposerExtensionIntf.h"
#include "QtiHidlComposerHalExtension.h"
#include "utils/Errors.h"

#define LOG_DISPLAY_ERROR(displayId, msg) \
    ALOGE("%s failed for display %s: %s", __FUNCTION__, to_string(displayId).c_str(), msg)

namespace android::surfaceflingerextension {

QtiHWComposerExtensionIntf* qtiCreateHWComposerExtension(android::impl::HWComposer& hwc,
                                                         Hwc2::Composer* composerHal) {
    return new QtiHWComposerExtension(hwc, composerHal);
}

QtiHWComposerExtension::QtiHWComposerExtension(android::impl::HWComposer& hwc)
      : mQtiHWComposer(hwc) {}

QtiHWComposerExtension::QtiHWComposerExtension(android::impl::HWComposer& hwc,
                                               Hwc2::Composer* composerHal)
      : mQtiHWComposer(hwc) {
    if (Hwc2::AidlComposer::isDeclared("default")) {
        mQtiComposerHalExtn =
                static_cast<QtiComposerHalExtension*>(new QtiAidlComposerHalExtension(composerHal));
    } else {
        mQtiComposerHalExtn =
                static_cast<QtiComposerHalExtension*>(new QtiHidlComposerHalExtension(composerHal));
        new QtiHidlComposerHalExtension(composerHal);
    }
}

std::optional<hal::HWDisplayId> QtiHWComposerExtension::qtiFromVirtualDisplayId(
        HalVirtualDisplayId displayId) const {
    if (const auto it = mQtiHWComposer.mDisplayData.find(displayId);
        it != mQtiHWComposer.mDisplayData.end()) {
        ALOGV("Virtual display id %d", static_cast<uint32_t>((it->second.hwcDisplay->getId())));
        return it->second.hwcDisplay->getId();
    }
    return {};
}

status_t QtiHWComposerExtension::qtiSetDisplayElapseTime(HalDisplayId displayId,
                                                         uint64_t timeStamp) {
    if (!mQtiComposerHalExtn) {
        return NO_ERROR;
    }

    if (mQtiHWComposer.mDisplayData.empty()) {
        ALOGV("HWComposer's displayData is empty");
        return BAD_VALUE;
    }

    if (mQtiHWComposer.mDisplayData.count(displayId) == 0) {
        LOG_DISPLAY_ERROR(displayId, "Invalid display");
        return UNKNOWN_ERROR;
    }

    const auto& displayData = mQtiHWComposer.mDisplayData[displayId];
    auto halHWDisplayId = displayData.hwcDisplay->getId();
    auto error = mQtiComposerHalExtn->qtiSetDisplayElapseTime(halHWDisplayId, timeStamp);
    if (error != hal::Error::NONE) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

status_t QtiHWComposerExtension::qtiSetLayerType(HWC2::Layer* layer, uint32_t type) {
    if (type == mQtiType) {
        return NO_ERROR;
    }

    if (!mQtiComposerHalExtn) {
        return NO_ERROR;
    }

    if (layer == nullptr) {
        return BAD_VALUE;
    }

    HWC2::impl::Layer* implLayer = static_cast<HWC2::impl::Layer*>(layer);
    auto intError = mQtiComposerHalExtn->qtiSetLayerType(implLayer->qtiGetDisplayId(),
                                                         implLayer->getId(), type);
    Error error = static_cast<Error>(intError);
    if (error != hal::Error::NONE) {
        ALOGW("Failed to send SET_LAYER_TYPE command to HWC");
        return BAD_VALUE;
    }

    mQtiType = type;
    return NO_ERROR;
}

status_t QtiHWComposerExtension::qtiSetLayerFlag(HWC2::Layer* layer,
                                                 uint32_t flags) {
    if (!mQtiComposerHalExtn) {
        return NO_ERROR;
    }
    if (layer == nullptr) {
        return BAD_VALUE;
    }

    HWC2::impl::Layer* implLayer = static_cast<HWC2::impl::Layer*>(layer);
    auto intError = mQtiComposerHalExtn->qtiSetLayerFlag(implLayer->qtiGetDisplayId(),
                                                         implLayer->getId(), flags);
    Error error = static_cast<Error>(intError);
    if (error != hal::Error::NONE) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

status_t QtiHWComposerExtension::qtiSetClientTarget_3_1(HalDisplayId displayId, int32_t slot,
                                                        const sp<Fence>& acquireFence,
                                                        uint32_t dataspace) {
    if (!mQtiComposerHalExtn) {
        return NO_ERROR;
    }

    if (mQtiHWComposer.mDisplayData.empty()) {
        ALOGV("HWComposer's displayData is empty");
        return BAD_VALUE;
    }

    if (mQtiHWComposer.mDisplayData.count(displayId) == 0) {
        LOG_DISPLAY_ERROR(displayId, "Invalid display");
        return UNKNOWN_ERROR;
    }

    const auto& displayData = mQtiHWComposer.mDisplayData[displayId];
    auto halHWDisplayId = displayData.hwcDisplay->getId();
    int32_t fenceFd = acquireFence->dup();
    auto error =
            mQtiComposerHalExtn->qtiSetClientTarget_3_1(halHWDisplayId, slot, fenceFd, dataspace);
    if (error != hal::Error::NONE) {
        return BAD_VALUE;
    }
    return NO_ERROR;
}

status_t QtiHWComposerExtension::qtiTryDrawMethod(HalDisplayId displayId,
                                                  uint32_t drawMethod) {
    if (!mQtiComposerHalExtn) {
        return NO_ERROR;
    }

    if (mQtiHWComposer.mDisplayData.empty()) {
        ALOGV("HWComposer's displayData is empty");
        return BAD_VALUE;
    }

    if (mQtiHWComposer.mDisplayData.count(displayId) == 0) {
        LOG_DISPLAY_ERROR(displayId, "Invalid display");
        return UNKNOWN_ERROR;
    }

    const auto& displayData = mQtiHWComposer.mDisplayData[displayId];
    auto halHWDisplayId = displayData.hwcDisplay->getId();
    auto error = mQtiComposerHalExtn->qtiTryDrawMethod(halHWDisplayId, drawMethod);
    if (error != hal::Error::NONE) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

} // namespace android::surfaceflingerextension
