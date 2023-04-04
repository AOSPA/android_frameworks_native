/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <utils/RefBase.h>
#include "../include/gui/BufferQueueProducer.h"

namespace android {

namespace libguiextension {

class QtiBufferQueueProducerExtension : public virtual RefBase {
public:
    QtiBufferQueueProducerExtension(BufferQueueProducer* bufferQueueProducer);
    ~QtiBufferQueueProducerExtension();

    void qtiQueueBuffer(bool isAutoTimestamp, int64_t requestedPresentTimestamp,
                        int window_api);

private:
    BufferQueueProducer* mQtiBufferQueueProducer = nullptr;
};

} // namespace libguiextension
} // namespace android
