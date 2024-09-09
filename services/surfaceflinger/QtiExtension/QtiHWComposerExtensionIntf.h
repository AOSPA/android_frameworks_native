/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "../DisplayHardware/HWC2.h"
#include "../DisplayHardware/HWComposer.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wextra"

#include "../DisplayHardware/ComposerHal.h"

#include <ui/DisplayId.h>
#include "QtiComposerHalExtensionIntf.h"

#pragma clang diagnostic pop

namespace android {


namespace impl {
class HWComposer;
} // namespace impl

namespace Hwc2 {
class Composer;
}

namespace HWC2::impl {
class Layer;
}

namespace surfaceflingerextension {

class QtiHWComposerExtensionIntf {
public:
    virtual ~QtiHWComposerExtensionIntf() {}

    virtual std::optional<hal::HWDisplayId> qtiFromVirtualDisplayId(HalVirtualDisplayId) const = 0;
    virtual status_t qtiSetDisplayElapseTime(HalDisplayId displayId, uint64_t timeStamp) = 0;
    virtual status_t qtiSetLayerType(HWC2::Layer* layer, uint32_t type) = 0;
    virtual status_t qtiTryDrawMethod(HalDisplayId displayId,
                                      uint32_t drawMethod) = 0;
    virtual status_t qtiSetClientTarget_3_1(HalDisplayId displayId, int32_t slot,
                                            const sp<Fence>& acquireFence,
                                            uint32_t dataspace) = 0;
    virtual status_t qtiSetLayerFlag(HWC2::Layer* layer,
                                     uint32_t layerFlag) = 0;
};

QtiHWComposerExtensionIntf* qtiCreateHWComposerExtension(android::impl::HWComposer& hwc,
                                                         Hwc2::Composer* composerHal);

class QtiHWComposerExtension : public QtiHWComposerExtensionIntf {
public:
    QtiHWComposerExtension(android::impl::HWComposer& hwc);
    QtiHWComposerExtension(android::impl::HWComposer& hwc, Hwc2::Composer* composerHal);
    ~QtiHWComposerExtension() = default;

    std::optional<hal::HWDisplayId> qtiFromVirtualDisplayId(HalVirtualDisplayId) const override;
    status_t qtiSetDisplayElapseTime(HalDisplayId displayId, uint64_t timeStamp) override;
    status_t qtiSetLayerType(HWC2::Layer* layer, uint32_t type) override;
    status_t qtiSetLayerFlag(HWC2::Layer* layer, uint32_t layerFlag) override;
    status_t qtiSetClientTarget_3_1(HalDisplayId displayId, int32_t slot,
                                    const sp<Fence>& acquireFence,
                                    uint32_t dataspace) override;
    status_t qtiTryDrawMethod(HalDisplayId displayId,
                              uint32_t drawMethod) override;

private:
    android::impl::HWComposer& mQtiHWComposer;
    QtiComposerHalExtension* mQtiComposerHalExtn = nullptr;
};

} // namespace surfaceflingerextension
} // namespace android
