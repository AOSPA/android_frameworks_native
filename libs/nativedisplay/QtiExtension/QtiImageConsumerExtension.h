/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "surfacetexture/ImageConsumer.h"

namespace android {

namespace libnativedisplay {

class QtiImageConsumerExtension {
public:
    QtiImageConsumerExtension(ImageConsumer* consumer);
    ~QtiImageConsumerExtension() = default;

    void updateBufferDataSpace(const sp<GraphicBuffer> graphicBuffer, BufferItem& item);

private:
    bool mQtiEnableExtn = false;
    ImageConsumer* mQtiImageConsumer = nullptr;
};

} // namespace libnativedisplay
} // namespace android
