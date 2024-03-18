/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <gui/BufferItem.h>

#include "surfacetexture/EGLConsumer.h"

namespace android {

namespace libnativedisplay {

class QtiEglImageExtension {
public:
    QtiEglImageExtension(EGLConsumer::EglImage* consumer);
    ~QtiEglImageExtension() = default;

    bool dataSpaceChanged();
    void setDataSpace();

    static bool extensionEnabled();

private:
    static bool mQtiEnableExtn;
    ui::Dataspace mQtiDataSpace = ui::Dataspace::UNKNOWN;
    EGLConsumer::EglImage* mQtiEglImage = nullptr;
};

} // namespace libnativedisplay
} // namespace android
