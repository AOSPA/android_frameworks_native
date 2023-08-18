/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <android/hardware/graphics/mapper/4.0/IMapper.h>

#include "../include/gui/Surface.h"

namespace android {

namespace libguiextension {

class QtiSurfaceExtension {
public:
    QtiSurfaceExtension(Surface* surface);
    ~QtiSurfaceExtension() = default;

    void qtiSetBufferDequeueDuration(std::string layerName, android_native_buffer_t* buffer,
                                     nsecs_t dequeue_duration);

private:
    bool isGame(std::string layerName);

    Surface* mQtiSurface = nullptr;
    bool mQtiIsGame = false;
    std::string mQtiLayerName = "";
    sp<android::hardware::graphics::mapper::V4_0::IMapper> mMapper = nullptr;
    bool enable_optimal_refresh_rate_ = false;
};

} // namespace libguiextension
} // namespace android
