/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "QtiOutputExtensionIntf.h"

namespace android {

namespace compositionengineextension {

class QtiOutputExtension : public QtiOutputExtensionIntf {
public:
    QtiOutputExtension(compositionengine::impl::Output* output);
    ~QtiOutputExtension() = default;

    bool qtiHasSecureContent() override;
    bool qtiHasSecureDisplay() override;

private:
    compositionengine::impl::Output* mQtiOutput = nullptr;
};

} // namespace compositionengineextension
} // namespace android
