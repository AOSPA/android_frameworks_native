/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "../DisplayHardware/HidlComposerHal.h"
#include "QtiComposerHalExtensionIntf.h"

namespace android {

namespace Hwc2 {
class HidlComposer;
}

namespace surfaceflingerextension {

class QtiHidlComposerHalExtension : public QtiComposerHalExtension {
public:
    QtiHidlComposerHalExtension(Hwc2::Composer* composerHal);

    Error qtiSetDisplayElapseTime(Display display, uint64_t timeStamp) override;
    Error qtiSetLayerType(Display display, V2_1_Layer layer, uint32_t type) { return Error::NONE; }

    Error qtiTryDrawMethod(Display display, IQtiComposerClient::DrawMethod drawMethod) {
        return Error::NONE;
    }
    Error qtiSetClientTarget_3_1(Display display, int32_t slot, int acquireFence,
                                 ui::Dataspace dataspace) {
        return Error::NONE;
    }
    Error qtiSetLayerFlag(Display display, V2_1_Layer layer,
                          IQtiComposerClient::LayerFlag layerFlag) {
        return Error::NONE;
    }

private:
    Hwc2::HidlComposer* mQtiHidlComposer = nullptr;
};

} // namespace surfaceflingerextension
} // namespace android
