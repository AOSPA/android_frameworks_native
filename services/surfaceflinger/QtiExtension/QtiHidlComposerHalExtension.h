/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "../DisplayHardware/ComposerHal.h"
#include "../DisplayHardware/HidlComposerHal.h"

namespace android {

namespace types = hardware::graphics::common;
namespace V2_1 = hardware::graphics::composer::V2_1;

using Hwc2::Error;
using V2_1::Display;

namespace Hwc2 {
class HidlComposer;
}

namespace surfaceflingerextension {

class QtiHidlComposerHalExtension {
public:
    QtiHidlComposerHalExtension(Hwc2::HidlComposer* composerHal);

    Error qtiSetDisplayElapseTime(Display display, uint64_t timeStamp);

private:
    Hwc2::HidlComposer* mQtiHidlComposer = nullptr;
};

} // namespace surfaceflingerextension
} // namespace android
