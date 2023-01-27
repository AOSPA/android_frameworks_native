/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "../include/compositionengine/impl/Output.h"

namespace android {

namespace compositionengine::impl {
class Output;
}

namespace compositionengineextension {

class QtiOutputExtension {
public:
    QtiOutputExtension(compositionengine::impl::Output* output);
    ~QtiOutputExtension() = default;

    bool qtiHasSecureContent();
    bool qtiHasSecureDisplay();

private:
    compositionengine::impl::Output* mQtiOutput = nullptr;
};

} // namespace compositionengineextension
} // namespace android
