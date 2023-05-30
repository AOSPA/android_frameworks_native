/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "../DisplayHardware/FramebufferSurface.h"
#include "../DisplayHardware/VirtualDisplaySurface.h"

namespace android {

class FramebufferSurface;
class VirtualDisplaySurface;

using FramebufferSurface = android::FramebufferSurface;
using VirtualDisplaySurface = android::VirtualDisplaySurface;

namespace surfaceflingerextension {

class QtiDisplaySurfaceExtensionIntf {
public:
    virtual ~QtiDisplaySurfaceExtensionIntf() {}

    virtual int getClientTargetCurrentSlot() = 0;
    virtual ui::Dataspace getClientTargetCurrentDataspace() = 0;

    /* Methods used by VirtualDisplaySurface */
    virtual uint64_t qtiSetOutputUsage() = 0;
    virtual uint64_t qtiSetOutputUsage(uint64_t flag) = 0;
    virtual uint64_t qtiExcludeVideoFromScratchBuffer(std::string source, uint64_t usage) = 0;
};

} // namespace surfaceflingerextension
} // namespace android
