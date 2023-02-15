/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <ui/DisplayId.h>
#include "../DisplayHardware/HWC2.h"
#include "../DisplayHardware/HWComposer.h"
#include "QtiComposerHalExtension.h"

namespace android {

namespace impl {
class HWComposer;
} // namespace impl

namespace surfaceflingerextension {

class QtiHWComposerExtension {
public:
    QtiHWComposerExtension(android::impl::HWComposer& hwc);
    QtiHWComposerExtension(android::impl::HWComposer& hwc, Hwc2::Composer* composerHal);
    ~QtiHWComposerExtension() = default;

    std::optional<hal::HWDisplayId> qtiFromVirtualDisplayId(HalVirtualDisplayId) const;
    status_t qtiSetDisplayElapseTime(HalDisplayId displayId, uint64_t timeStamp);

private:
    android::impl::HWComposer& mQtiHWComposer;
    QtiComposerHalExtension* mQtiComposerHalExtn = nullptr;
};

} // namespace surfaceflingerextension
} // namespace android
