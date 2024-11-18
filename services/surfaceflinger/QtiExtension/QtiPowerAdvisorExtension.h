/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "../PowerAdvisor/PowerAdvisor.h"

namespace android {

namespace adpf::impl {
class PowerAdvisor;
}

namespace surfaceflingerextension {

class QtiPowerAdvisorExtension {
public:
    QtiPowerAdvisorExtension(adpf::impl::PowerAdvisor* powerAdvisor);
    ~QtiPowerAdvisorExtension() = default;

    bool qtiCanNotifyDisplayUpdateImminent();

private:
    adpf::impl::PowerAdvisor* mQtiPowerAdvisor = nullptr;
};

} // namespace surfaceflingerextension
} // namespace android
