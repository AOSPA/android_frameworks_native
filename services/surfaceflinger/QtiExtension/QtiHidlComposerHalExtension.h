/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "../DisplayHardware/HidlComposerHal.h"
#include "QtiComposerHalExtensionIntf.h"

#include <vendor/qti/hardware/display/composer/3.1/IQtiComposer.h>
#include <vendor/qti/hardware/display/composer/3.1/IQtiComposerClient.h>
#include <cstdint>

namespace android {

using vendor::qti::hardware::display::composer::V3_1::IQtiComposer;
using vendor::qti::hardware::display::composer::V3_1::IQtiComposerClient;

namespace types = hardware::graphics::common;
namespace V2_1 = hardware::graphics::composer::V2_1;

using Hwc2::Error;
using V2_1::Display;
using V2_1_Layer = V2_1::Layer;

namespace Hwc2 {
class HidlComposer;
}

namespace surfaceflingerextension {

class QtiHidlComposerHalExtension : public QtiComposerHalExtension {
public:
    QtiHidlComposerHalExtension(Hwc2::Composer* composerHal);

    Error qtiSetDisplayElapseTime(Display display, uint64_t timeStamp) override;
    Error qtiSetLayerType(Display display, V2_1_Layer layer, uint32_t type) override;
    Error qtiSetLayerFlag(Display display, V2_1_Layer layer,
                          uint32_t layerFlag) override;
    Error qtiSetClientTarget_3_1(Display display, int32_t slot, int acquireFence,
                                 uint32_t dataspace) override;
    Error qtiTryDrawMethod(Display display, uint32_t drawMethod) override;

private:
    Hwc2::HidlComposer* mQtiHidlComposer = nullptr;
    sp<IQtiComposerClient> mClient_3_1;
};

} // namespace surfaceflingerextension
} // namespace android
