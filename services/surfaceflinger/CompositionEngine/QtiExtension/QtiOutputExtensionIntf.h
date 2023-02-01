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

class QtiOutputExtensionIntf {
public:
    virtual ~QtiOutputExtensionIntf() {}

    virtual bool qtiHasSecureContent() = 0;
    virtual bool qtiHasSecureDisplay() = 0;
};

QtiOutputExtensionIntf* qtiCreateOutputExtension(compositionengine::impl::Output* output);

} // namespace compositionengineextension
} // namespace android
