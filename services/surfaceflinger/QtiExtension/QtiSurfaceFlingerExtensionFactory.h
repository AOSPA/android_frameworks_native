/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <sys/cdefs.h>
#include "QtiSurfaceFlingerExtensionIntf.h"
#include "QtiNullExtension.h"
#include "QtiNullDisplaySurfaceExtension.h"

class LegacyVirtualDisplaySurface;
namespace android::surfaceflingerextension {

QtiSurfaceFlingerExtensionIntf* qtiCreateSurfaceFlingerExtension(SurfaceFlinger* flinger);
QtiDisplaySurfaceExtensionIntf* qtiCreateDisplaySurfaceExtension(bool isVirtual,
                                                                 LegacyVirtualDisplaySurface* vds,
                                                                 bool secure, uint64_t sinkUsage,
                                                                 LegacyFramebufferSurface* fbs);
QtiDisplaySurfaceExtensionIntf* qtiCreateDisplaySurfaceExtension(bool isVirtual,
                                                                  LegacyVirtualDisplaySurface* vds,
                                                                  bool secure, uint64_t sinkUsage,
                                                                  FramebufferSurface* fbs);

} // namespace android::surfaceflingerextension
