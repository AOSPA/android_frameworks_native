/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// #define LOG_NDEBUG 0
#include "QtiPhaseOffsetsExtension.h"

#include <log/log.h>

namespace android::surfaceflingerextension {

namespace {
std::chrono::nanoseconds sfOffsetToDuration(nsecs_t sfOffset, nsecs_t vsyncDuration) {
    return std::chrono::nanoseconds(vsyncDuration - sfOffset);
}

std::chrono::nanoseconds appOffsetToDuration(nsecs_t appOffset, nsecs_t sfOffset,
                                             nsecs_t vsyncDuration) {
    auto duration = vsyncDuration + (sfOffset - appOffset);
    if (duration < vsyncDuration) {
        duration += vsyncDuration;
    }

    return std::chrono::nanoseconds(duration);
}
} // namespace

QtiPhaseOffsetsExtension::QtiPhaseOffsetsExtension(scheduler::VsyncConfiguration* vsyncConfig) {
    if (!vsyncConfig) {
        ALOGW("Invalid pointer to VsyncConfiguration passed");
        return;
    }

    mQtiVsyncConfiguration = static_cast<scheduler::impl::PhaseOffsets*>(vsyncConfig);
    ALOGV_IF(mQtiVsyncConfiguration, "Successfully create QtiPhaseOffsetsExtn %p",
             mQtiVsyncConfiguration);
}

bool qtiFpsEqualsWithMargin(float fpsA, float fpsB) {
    static constexpr float MARGIN = 0.01f;
    return std::abs(fpsA - fpsB) <= MARGIN;
}

void QtiPhaseOffsetsExtension::qtiUpdateWorkDurations(
        unordered_map<float, pair<int64_t, int64_t>>* workDurationConfigs) {
    ALOGW("PhaseOffsets not supported when WorkDurations is in use");
}

void QtiPhaseOffsetsExtension::qtiUpdateSfOffsets(
        unordered_map<float, int64_t>* advancedSfOffsets) {
    if (!mQtiVsyncConfiguration) {
        return;
    }

    std::lock_guard lock(mQtiVsyncConfiguration->mLock);
    for (auto& item : *advancedSfOffsets) {
        float fps = item.first;
        auto& mOffsetsCache = mQtiVsyncConfiguration->mOffsetsCache;
        auto iter = mOffsetsCache.begin();
        for (iter = mOffsetsCache.begin(); iter != mOffsetsCache.end(); iter++) {
            float candidateFps = iter->first.getValue();
            if (qtiFpsEqualsWithMargin(fps, candidateFps)) {
                break;
            }
        }

        if (iter != mOffsetsCache.end()) {
            auto vsyncDuration = iter->first.getPeriodNsecs();
            auto& [early, earlyGpu, late, hwcMinWorkDuration] = iter->second;
            late.sfOffset = item.second;
            late.sfWorkDuration = sfOffsetToDuration(late.sfOffset, vsyncDuration);
            late.appWorkDuration =
                    appOffsetToDuration(late.appOffset, late.sfOffset, vsyncDuration);
            early.sfOffset = item.second;
            early.sfWorkDuration = sfOffsetToDuration(early.sfOffset, vsyncDuration),
            early.appWorkDuration =
                    appOffsetToDuration(early.appOffset, early.sfOffset, vsyncDuration);
            earlyGpu.sfOffset = item.second;
            earlyGpu.sfWorkDuration = sfOffsetToDuration(earlyGpu.sfOffset, vsyncDuration);
            earlyGpu.appWorkDuration =
                    appOffsetToDuration(earlyGpu.appOffset, earlyGpu.sfOffset, vsyncDuration);
        }
    }
}

} // namespace android::surfaceflingerextension
