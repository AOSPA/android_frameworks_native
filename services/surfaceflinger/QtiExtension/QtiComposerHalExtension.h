/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "../DisplayHardware/ComposerHal.h"
#include "../DisplayHardware/HidlComposerHal.h"

#include "QtiHidlComposerHalExtension.h"

namespace android {

namespace Hwc2 {
class Composer;
class HidlComposer;
} // namespace Hwc2

namespace surfaceflingerextension {

class QtiComposerHalExtension {
public:
    QtiComposerHalExtension();
    QtiComposerHalExtension(Hwc2::Composer* composerHal);

    Error qtiSetDisplayElapseTime(Display display, uint64_t timeStamp);

private:
    Hwc2::Composer* mQtiComposerHal;
    QtiHidlComposerHalExtension* mQtiHidlComposerExtn = nullptr;
};

} // namespace surfaceflingerextension
} // namespace android
