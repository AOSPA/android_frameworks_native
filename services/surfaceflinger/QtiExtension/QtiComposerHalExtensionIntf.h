/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <vendor/qti/hardware/display/composer/3.1/IQtiComposer.h>
#include <vendor/qti/hardware/display/composer/3.1/IQtiComposerClient.h>
namespace android {

namespace V2_1 = hardware::graphics::composer::V2_1;
namespace types = hardware::graphics::common;

using V2_1::Display;
using V2_1::Error;
using V2_1_Layer = V2_1::Layer;
using V1_2_Dataspace = types::V1_2::Dataspace;

using vendor::qti::hardware::display::composer::V3_1::IQtiComposer;
using vendor::qti::hardware::display::composer::V3_1::IQtiComposerClient;

namespace Hwc2 {
class Composer;
} // namespace Hwc2

namespace surfaceflingerextension {

class QtiComposerHalExtension {
public:
    virtual ~QtiComposerHalExtension(){};

    virtual Error qtiSetDisplayElapseTime(Display display, uint64_t timeStamp) = 0;
    virtual Error qtiSetLayerType(Display display, V2_1_Layer layer, uint32_t type) = 0;
    virtual Error qtiTryDrawMethod(Display display, IQtiComposerClient::DrawMethod drawMethod) = 0;
    virtual Error qtiSetClientTarget_3_1(Display display, int32_t slot, int acquireFence,
                                         ui::Dataspace dataspace) = 0;
    virtual Error qtiSetLayerFlag(Display display, V2_1_Layer layer,
                                  IQtiComposerClient::LayerFlag layerFlag) = 0;
};

} // namespace surfaceflingerextension
} // namespace android
