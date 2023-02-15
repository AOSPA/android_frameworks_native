/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "QtiOutputExtensionIntf.h"

namespace android {

namespace compositionengineextension {

class QtiNullOutputExtension : public QtiOutputExtensionIntf {
public:
    QtiNullOutputExtension(compositionengine::impl::Output* output);
    ~QtiNullOutputExtension() = default;

    bool qtiHasSecureContent() override;
    bool qtiHasSecureDisplay() override;

private:
    compositionengine::impl::Output* mQtiOutput = nullptr;
};

} // namespace compositionengineextension
} // namespace android
