/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <android/hardware/graphics/composer/2.4/IComposerClient.h>

namespace android {

namespace V2_1 = hardware::graphics::composer::V2_1;

using V2_1::Display;
using V2_1::Error;
using V2_1_Layer = V2_1::Layer;

namespace Hwc2 {
class Composer;
} // namespace Hwc2

namespace surfaceflingerextension {

class QtiComposerHalExtension {
public:
    virtual ~QtiComposerHalExtension(){};

    virtual Error qtiSetDisplayElapseTime(Display display, uint64_t timeStamp) = 0;
    virtual Error qtiSetLayerType(Display display, V2_1_Layer layer, uint32_t type) = 0;
    virtual Error qtiTryDrawMethod(Display display, uint32_t drawMethod) = 0;
    virtual Error qtiSetClientTarget_3_1(Display display, int32_t slot, int acquireFence,
                                         uint32_t dataspace) = 0;
    virtual Error qtiSetLayerFlag(Display display, V2_1_Layer layer,
                                  uint32_t layerFlag) = 0;
};

} // namespace surfaceflingerextension
} // namespace android
