/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "../DisplayHardware/PowerAdvisor.h"

namespace android {

namespace Hwc2::impl {
class PowerAdvisor;
}

namespace surfaceflingerextension {

class QtiPowerAdvisorExtension {
public:
    QtiPowerAdvisorExtension(Hwc2::impl::PowerAdvisor* powerAdvisor);
    ~QtiPowerAdvisorExtension() = default;

    bool qtiCanNotifyDisplayUpdateImminent();

private:
    Hwc2::impl::PowerAdvisor* mQtiPowerAdvisor = nullptr;
};

} // namespace surfaceflingerextension
} // namespace android
