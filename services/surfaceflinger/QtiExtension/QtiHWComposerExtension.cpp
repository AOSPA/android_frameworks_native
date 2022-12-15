/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

// #define LOG_NDEBUG 0
#include "QtiHWComposerExtension.h"

namespace android::surfaceflingerextension {

QtiHWComposerExtension::QtiHWComposerExtension(android::impl::HWComposer& hwc)
      : mQtiHWComposer(hwc) {}

std::optional<hal::HWDisplayId> QtiHWComposerExtension::qtiFromVirtualDisplayId(
        HalVirtualDisplayId displayId) const {
    if (const auto it = mQtiHWComposer.mDisplayData.find(displayId);
        it != mQtiHWComposer.mDisplayData.end()) {
        ALOGV("Virtual display id %d", static_cast<uint32_t>((it->second.hwcDisplay->getId())));
        return it->second.hwcDisplay->getId();
    }
    return {};
}

} // namespace android::surfaceflingerextension
