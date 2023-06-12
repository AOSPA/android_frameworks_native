/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <sys/cdefs.h>
#include "QtiSurfaceFlingerExtensionIntf.h"
#include "QtiNullExtension.h"
#include "QtiNullDisplaySurfaceExtension.h"

namespace android::surfaceflingerextension {

QtiSurfaceFlingerExtensionIntf* qtiCreateSurfaceFlingerExtension(SurfaceFlinger* flinger);
QtiDisplaySurfaceExtensionIntf* qtiCreateDisplaySurfaceExtension(bool isVirtual,
                                                                 VirtualDisplaySurface* vds,
                                                                 bool secure, uint64_t sinkUsage,
                                                                 FramebufferSurface* fbs);

} // namespace android::surfaceflingerextension
