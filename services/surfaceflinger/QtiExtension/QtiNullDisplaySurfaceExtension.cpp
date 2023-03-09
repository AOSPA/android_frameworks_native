/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0
#include "QtiNullDisplaySurfaceExtension.h"

#include <log/log.h>

namespace android::surfaceflingerextension {

QtiNullDisplaySurfaceExtension::QtiNullDisplaySurfaceExtension(VirtualDisplaySurface* vds,
                                                               bool secure, uint64_t sinkUsage)
      : mQtiVDS(vds) {
    if (!mQtiVDS) {
        ALOGW("Passed an invalid pointer to VirtualDisplaySurface");
    }

    mQtiSecure = secure;
    mQtiSinkUsage = sinkUsage;
    ALOGV("Successfully created QtiVDSExtension %p isSecure %d mQtiSinkUsage 0x%" PRIx64, mQtiVDS,
          mQtiSecure, mQtiSinkUsage);
}

uint64_t QtiNullDisplaySurfaceExtension::qtiSetOutputUsage() {
    return qtiSetOutputUsage(mQtiSinkUsage);
}

/* Helper to update the output usage when the display is secure */
uint64_t QtiNullDisplaySurfaceExtension::qtiSetOutputUsage(uint64_t flag) {
    return flag;
}

uint64_t QtiNullDisplaySurfaceExtension::qtiExcludeVideoFromScratchBuffer(std::string source,
                                                                          uint64_t usage) {
    return usage;
}

int QtiNullDisplaySurfaceExtension::getClientTargetCurrentSlot() {
    return -1;
}

ui::Dataspace QtiNullDisplaySurfaceExtension::getClientTargetCurrentDataspace() {
    return ui::Dataspace::UNKNOWN;
}

} // namespace android::surfaceflingerextension
