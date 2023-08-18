/* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

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
    bool mEnableOptimalRefreshRate = false;
};

} // namespace libguiextension
} // namespace android
