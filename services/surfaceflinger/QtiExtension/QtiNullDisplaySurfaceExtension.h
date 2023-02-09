/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "QtiDisplaySurfaceExtensionIntf.h"

namespace android {

namespace surfaceflingerextension {

class QtiNullDisplaySurfaceExtension : public QtiDisplaySurfaceExtensionIntf {
public:
    QtiNullDisplaySurfaceExtension() {}
    QtiNullDisplaySurfaceExtension(VirtualDisplaySurface* vds, bool secure, uint64_t sinkUsage);
    ~QtiNullDisplaySurfaceExtension() = default;

    uint64_t qtiSetOutputUsage();
    uint64_t qtiSetOutputUsage(uint64_t flag);
    uint64_t qtiExcludeVideoFromScratchBuffer(std::string source, uint64_t usage);

private:
    VirtualDisplaySurface* mQtiVDS = nullptr;

    bool mQtiSecure = false;
    uint64_t mQtiSinkUsage = 0;
};

} // namespace surfaceflingerextension
} // namespace android
