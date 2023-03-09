/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "QtiDisplaySurfaceExtensionIntf.h"

namespace android {

namespace surfaceflingerextension {

class QtiFramebufferSurfaceExtension : public QtiDisplaySurfaceExtensionIntf {
public:
    QtiFramebufferSurfaceExtension() {}
    QtiFramebufferSurfaceExtension(FramebufferSurface* fbs);
    ~QtiFramebufferSurfaceExtension() = default;

    int getClientTargetCurrentSlot() override;
    ui::Dataspace getClientTargetCurrentDataspace() override;

    /* Methods used by VirtualDisplaySurface */
    uint64_t qtiSetOutputUsage() override;
    uint64_t qtiSetOutputUsage(uint64_t flag) override;
    uint64_t qtiExcludeVideoFromScratchBuffer(std::string source, uint64_t usage) override;

private:
    FramebufferSurface* mQtiFBS = nullptr;
};

} // namespace surfaceflingerextension
} // namespace android
