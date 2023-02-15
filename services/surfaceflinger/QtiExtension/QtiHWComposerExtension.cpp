/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

// #define LOG_NDEBUG 0
#include "QtiHWComposerExtension.h"
#include "QtiComposerHalExtension.h"

#define LOG_DISPLAY_ERROR(displayId, msg) \
    ALOGE("%s failed for display %s: %s", __FUNCTION__, to_string(displayId).c_str(), msg)

namespace android::surfaceflingerextension {

QtiHWComposerExtension::QtiHWComposerExtension(android::impl::HWComposer& hwc)
      : mQtiHWComposer(hwc) {}

QtiHWComposerExtension::QtiHWComposerExtension(android::impl::HWComposer& hwc,
                                               Hwc2::Composer* composerHal)
      : mQtiHWComposer(hwc) {
    mQtiComposerHalExtn = new QtiComposerHalExtension(composerHal);
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

} // namespace android::surfaceflingerextension
