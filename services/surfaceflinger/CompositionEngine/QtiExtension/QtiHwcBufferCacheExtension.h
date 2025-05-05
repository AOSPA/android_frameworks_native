/* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <QtiGrallocDefs.h>
#include <ui/GraphicBuffer.h>

#include "compositionengine/impl/HwcBufferCache.h"

using android::compositionengine::impl::HwcBufferCache;

namespace android::compositionengineextension {
// Singleton for HwcBufferCache extension
class QtiHwcBufferCacheExtension {
public:
    QtiHwcBufferCacheExtension();
    ~QtiHwcBufferCacheExtension()=default;

    static QtiHwcBufferCacheExtension& Instance() {
        static QtiHwcBufferCacheExtension extension;
        return extension;
    }

    bool formatIsYuv(const PixelFormat format);
    void ResetFreeSlotsForWideVideo(const sp<GraphicBuffer>& buffer,
                                        HwcBufferCache* hwcBufferCache);

private:
    bool mReduceSlotsForWideVideo = false;
};

} // namespace android::compositionengineextension
