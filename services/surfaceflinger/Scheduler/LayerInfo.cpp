/*
 * Copyright 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra"

// #define LOG_NDEBUG 0
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "LayerInfo.h"

#include <algorithm>
#include <utility>

#include <cutils/compiler.h>
#include <cutils/trace.h>
#include <ftl/enum.h>
#include <gui/TraceUtils.h>

#undef LOG_TAG
#define LOG_TAG "LayerInfo"

namespace android::scheduler {

bool LayerInfo::sTraceEnabled = false;

LayerInfo::LayerInfo(const std::string& name, uid_t ownerUid,
                     LayerHistory::LayerVoteType defaultVote)
      : mName(name),
        mOwnerUid(ownerUid),
        mDefaultVote(defaultVote),
        mLayerVote({defaultVote, Fps()}),
        mLayerProps(std::make_unique<LayerProps>()),
        mRefreshRateHistory(name) {
    ;
}

void LayerInfo::setLastPresentTime(nsecs_t lastPresentTime, nsecs_t now, LayerUpdateType updateType,
                                   bool pendingModeChange, const LayerProps& props) {
    lastPresentTime = std::max(lastPresentTime, static_cast<nsecs_t>(0));

    mLastUpdatedTime = std::max(lastPresentTime, now);
    *mLayerProps = props;
    switch (updateType) {
        case LayerUpdateType::AnimationTX:
            mLastAnimationTime = std::max(lastPresentTime, now);
            break;
        case LayerUpdateType::SetFrameRate:
        case LayerUpdateType::Buffer:
            FrameTimeData frameTime = {.presentTime = lastPresentTime,
                                       .queueTime = mLastUpdatedTime,
                                       .pendingModeChange = pendingModeChange,
                                       .isSmallDirty = props.isSmallDirty};
            mFrameTimes.push_back(frameTime);
            if (mFrameTimes.size() > HISTORY_SIZE) {
                mFrameTimes.pop_front();
            }
            break;
    }
}

bool LayerInfo::isFrameTimeValid(const FrameTimeData& frameTime) const {
    return frameTime.queueTime >= std::chrono::duration_cast<std::chrono::nanoseconds>(
                                          mFrameTimeValidSince.time_since_epoch())
                                          .count();
}

LayerInfo::Frequent LayerInfo::isFrequent(nsecs_t now) const {
    // If we know nothing about this layer (e.g. after touch event),
    // we consider it as frequent as it might be the start of an animation.
    if (mFrameTimes.size() < kFrequentLayerWindowSize) {
        return {/* isFrequent */ true, /* clearHistory */ false, /* isConclusive */ true};
    }

    // Non-active layers are also infrequent
    if (mLastUpdatedTime < getActiveLayerThreshold(now)) {
        return {/* isFrequent */ false, /* clearHistory */ false, /* isConclusive */ true};
    }

    // We check whether we can classify this layer as frequent or infrequent:
    //  - frequent: a layer posted kFrequentLayerWindowSize within
    //              kMaxPeriodForFrequentLayerNs of each other.
    // -  infrequent: a layer posted kFrequentLayerWindowSize with longer
    //                gaps than kFrequentLayerWindowSize.
    // If we can't determine the layer classification yet, we return the last
    // classification.
    bool isFrequent = true;
    bool isInfrequent = true;
    int32_t smallDirtyCount = 0;
    const auto n = mFrameTimes.size() - 1;
    for (size_t i = 0; i < kFrequentLayerWindowSize - 1; i++) {
        if (mFrameTimes[n - i].queueTime - mFrameTimes[n - i - 1].queueTime <
            kMaxPeriodForFrequentLayerNs.count()) {
            isInfrequent = false;
            if (mFrameTimes[n - i].presentTime == 0 && mFrameTimes[n - i].isSmallDirty) {
                smallDirtyCount++;
            }
        } else {
            isFrequent = false;
        }
    }

    // Vote the small dirty when a layer contains at least HISTORY_SIZE of small dirty updates.
    bool isSmallDirty = false;
    if (smallDirtyCount >= kNumSmallDirtyThreshold) {
        if (mLastSmallDirtyCount >= HISTORY_SIZE) {
            isSmallDirty = true;
        } else {
            mLastSmallDirtyCount++;
        }
    } else {
        mLastSmallDirtyCount = 0;
    }

    if (isFrequent || isInfrequent) {
        // If the layer was previously inconclusive, we clear
        // the history as indeterminate layers changed to frequent,
        // and we should not look at the stale data.
        return {isFrequent, isFrequent && !mIsFrequencyConclusive, /* isConclusive */ true,
                isSmallDirty};
    }

    // If we can't determine whether the layer is frequent or not, we return
    // the last known classification and mark the layer frequency as inconclusive.
    isFrequent = !mLastRefreshRate.infrequent;

    // If the layer was previously tagged as animating, we clear
    // the history as it is likely the layer just changed its behavior,
    // and we should not look at stale data.
    return {isFrequent, isFrequent && mLastRefreshRate.animating, /* isConclusive */ false};
}

Fps LayerInfo::getFps(nsecs_t now) const {
    // Find the first active frame
    auto it = mFrameTimes.begin();
    for (; it != mFrameTimes.end(); ++it) {
        if (it->queueTime >= getActiveLayerThreshold(now)) {
            break;
        }
    }

    const auto numFrames = std::distance(it, mFrameTimes.end());
    if (numFrames < kFrequentLayerWindowSize) {
        return Fps();
    }

    // Layer is considered frequent if the average frame rate is higher than the threshold
    const auto totalTime = mFrameTimes.back().queueTime - it->queueTime;
    return Fps::fromPeriodNsecs(totalTime / (numFrames - 1));
}

bool LayerInfo::isAnimating(nsecs_t now) const {
    return mLastAnimationTime >= getActiveLayerThreshold(now);
}

bool LayerInfo::hasEnoughDataForHeuristic() const {
    // The layer had to publish at least HISTORY_SIZE or HISTORY_DURATION of updates
    if (mFrameTimes.size() < 2) {
        ALOGV("fewer than 2 frames recorded: %zu", mFrameTimes.size());
        return false;
    }

    if (!isFrameTimeValid(mFrameTimes.front())) {
        ALOGV("stale frames still captured");
        return false;
    }

    const auto totalDuration = mFrameTimes.back().queueTime - mFrameTimes.front().queueTime;
    if (mFrameTimes.size() < HISTORY_SIZE && totalDuration < HISTORY_DURATION.count()) {
        ALOGV("not enough frames captured: %zu | %.2f seconds", mFrameTimes.size(),
              totalDuration / 1e9f);
        return false;
    }

    return true;
}

std::optional<nsecs_t> LayerInfo::calculateAverageFrameTime() const {
    // Ignore frames captured during a mode change
    const bool isDuringModeChange =
            std::any_of(mFrameTimes.begin(), mFrameTimes.end(),
                        [](const auto& frame) { return frame.pendingModeChange; });
    if (isDuringModeChange) {
        return std::nullopt;
    }

    const bool isMissingPresentTime =
            std::any_of(mFrameTimes.begin(), mFrameTimes.end(),
                        [](auto frame) { return frame.presentTime == 0; });

    // Calculate the average frame time based on presentation timestamps. If those
    // doesn't exist, we look at the time the buffer was queued only. We can do that only if
    // we calculated a refresh rate based on presentation timestamps in the past. The reason
    // we look at the queue time is to handle cases where hwui attaches presentation timestamps
    // when implementing render ahead for specific refresh rates. When hwui no longer provides
    // presentation timestamps we look at the queue time to see if the current refresh rate still
    // matches the content.

    auto getFrameTime = isMissingPresentTime ? [](FrameTimeData data) { return data.queueTime; }
                                             : [](FrameTimeData data) { return data.presentTime; };

    nsecs_t totalDeltas = 0;
    int numDeltas = 0;
    int32_t smallDirtyCount = 0;
    auto prevFrame = mFrameTimes.begin();
    for (auto it = mFrameTimes.begin() + 1; it != mFrameTimes.end(); ++it) {
        const auto currDelta = getFrameTime(*it) - getFrameTime(*prevFrame);
        if (currDelta < kMinPeriodBetweenFrames) {
            // Skip this frame, but count the delta into the next frame
            continue;
        }

        // If this is a small area update, we don't want to consider it for calculating the average
        // frame time. Instead, we let the bigger frame updates to drive the calculation.
        if (it->isSmallDirty && currDelta < kMinPeriodBetweenSmallDirtyFrames) {
            smallDirtyCount++;
            continue;
        }

        prevFrame = it;

        if (currDelta > kMaxPeriodBetweenFrames) {
            // Skip this frame and the current delta.
            continue;
        }

        totalDeltas += currDelta;
        numDeltas++;
    }

    if (smallDirtyCount > 0) {
        ATRACE_FORMAT_INSTANT("small dirty = %" PRIu32, smallDirtyCount);
    }

    if (numDeltas == 0) {
        return std::nullopt;
    }

    const auto averageFrameTime = static_cast<double>(totalDeltas) / static_cast<double>(numDeltas);
    return static_cast<nsecs_t>(averageFrameTime);
}

std::optional<Fps> LayerInfo::calculateRefreshRateIfPossible(const RefreshRateSelector& selector,
                                                             nsecs_t now) {
    ATRACE_CALL();
    static constexpr float MARGIN = 1.0f; // 1Hz
    if (!hasEnoughDataForHeuristic()) {
        ALOGV("Not enough data");
        return std::nullopt;
    }

    if (const auto averageFrameTime = calculateAverageFrameTime()) {
        const auto refreshRate = Fps::fromPeriodNsecs(*averageFrameTime);
        const bool refreshRateConsistent = mRefreshRateHistory.add(refreshRate, now);
        if (refreshRateConsistent) {
            const auto knownRefreshRate = selector.findClosestKnownFrameRate(refreshRate);
            using fps_approx_ops::operator!=;

            // To avoid oscillation, use the last calculated refresh rate if it is close enough.
            if (std::abs(mLastRefreshRate.calculated.getValue() - refreshRate.getValue()) >
                        MARGIN &&
                mLastRefreshRate.reported != knownRefreshRate) {
                mLastRefreshRate.calculated = refreshRate;
                mLastRefreshRate.reported = knownRefreshRate;
            }

            ALOGV("%s %s rounded to nearest known frame rate %s", mName.c_str(),
                  to_string(refreshRate).c_str(), to_string(mLastRefreshRate.reported).c_str());
        } else {
            ALOGV("%s Not stable (%s) returning last known frame rate %s", mName.c_str(),
                  to_string(refreshRate).c_str(), to_string(mLastRefreshRate.reported).c_str());
        }
    }

    return mLastRefreshRate.reported.isValid() ? std::make_optional(mLastRefreshRate.reported)
                                               : std::nullopt;
}

LayerInfo::LayerVote LayerInfo::getRefreshRateVote(const RefreshRateSelector& selector,
                                                   nsecs_t now) {
    ATRACE_CALL();
    if (mLayerVote.type != LayerHistory::LayerVoteType::Heuristic) {
        ALOGV("%s voted %d ", mName.c_str(), static_cast<int>(mLayerVote.type));
        return mLayerVote;
    }

    if (isAnimating(now)) {
        ATRACE_FORMAT_INSTANT("animating");
        ALOGV("%s is animating", mName.c_str());
        mLastRefreshRate.animating = true;
        return {LayerHistory::LayerVoteType::Max, Fps()};
    }

    const LayerInfo::Frequent frequent = isFrequent(now);
    mIsFrequencyConclusive = frequent.isConclusive;
    if (!frequent.isFrequent) {
        ATRACE_FORMAT_INSTANT("infrequent");
        ALOGV("%s is infrequent", mName.c_str());
        mLastRefreshRate.infrequent = true;
        mLastSmallDirtyCount = 0;
        // Infrequent layers vote for minimal refresh rate for
        // battery saving purposes and also to prevent b/135718869.
        return {LayerHistory::LayerVoteType::Min, Fps()};
    }

    if (frequent.clearHistory) {
        clearHistory(now);
    }

    // Return no vote if the recent frames are small dirty.
    if (frequent.isSmallDirty && !mLastRefreshRate.reported.isValid()) {
        ATRACE_FORMAT_INSTANT("NoVote (small dirty)");
        ALOGV("%s is small dirty", mName.c_str());
        return {LayerHistory::LayerVoteType::NoVote, Fps()};
    }

    auto refreshRate = calculateRefreshRateIfPossible(selector, now);
    if (refreshRate.has_value()) {
        ALOGV("%s calculated refresh rate: %s", mName.c_str(), to_string(*refreshRate).c_str());
        return {LayerHistory::LayerVoteType::Heuristic, refreshRate.value()};
    }

    ALOGV("%s Max (can't resolve refresh rate)", mName.c_str());
    return {LayerHistory::LayerVoteType::Max, Fps()};
}

const char* LayerInfo::getTraceTag(LayerHistory::LayerVoteType type) const {
    if (mTraceTags.count(type) == 0) {
        auto tag = "LFPS " + mName + " " + ftl::enum_string(type);
        mTraceTags.emplace(type, std::move(tag));
    }

    return mTraceTags.at(type).c_str();
}

LayerInfo::FrameRate LayerInfo::getSetFrameRateVote() const {
    return mLayerProps->setFrameRateVote;
}

bool LayerInfo::isVisible() const {
    return mLayerProps->visible;
}

int32_t LayerInfo::getFrameRateSelectionPriority() const {
    return mLayerProps->frameRateSelectionPriority;
}

FloatRect LayerInfo::getBounds() const {
    return mLayerProps->bounds;
}

ui::Transform LayerInfo::getTransform() const {
    return mLayerProps->transform;
}

LayerInfo::RefreshRateHistory::HeuristicTraceTagData
LayerInfo::RefreshRateHistory::makeHeuristicTraceTagData() const {
    const std::string prefix = "LFPS ";
    const std::string suffix = "Heuristic ";
    return {.min = prefix + mName + suffix + "min",
            .max = prefix + mName + suffix + "max",
            .consistent = prefix + mName + suffix + "consistent",
            .average = prefix + mName + suffix + "average"};
}

void LayerInfo::RefreshRateHistory::clear() {
    mRefreshRates.clear();
}

bool LayerInfo::RefreshRateHistory::add(Fps refreshRate, nsecs_t now) {
    mRefreshRates.push_back({refreshRate, now});
    while (mRefreshRates.size() >= HISTORY_SIZE ||
           now - mRefreshRates.front().timestamp > HISTORY_DURATION.count()) {
        mRefreshRates.pop_front();
    }

    if (CC_UNLIKELY(sTraceEnabled)) {
        if (!mHeuristicTraceTagData.has_value()) {
            mHeuristicTraceTagData = makeHeuristicTraceTagData();
        }

        ATRACE_INT(mHeuristicTraceTagData->average.c_str(), refreshRate.getIntValue());
    }

    return isConsistent();
}

bool LayerInfo::RefreshRateHistory::isConsistent() const {
    if (mRefreshRates.empty()) return true;

    const auto [min, max] =
            std::minmax_element(mRefreshRates.begin(), mRefreshRates.end(),
                                [](const auto& lhs, const auto& rhs) {
                                    return isStrictlyLess(lhs.refreshRate, rhs.refreshRate);
                                });

    const bool consistent =
            max->refreshRate.getValue() - min->refreshRate.getValue() < MARGIN_CONSISTENT_FPS;

    if (CC_UNLIKELY(sTraceEnabled)) {
        if (!mHeuristicTraceTagData.has_value()) {
            mHeuristicTraceTagData = makeHeuristicTraceTagData();
        }

        ATRACE_INT(mHeuristicTraceTagData->max.c_str(), max->refreshRate.getIntValue());
        ATRACE_INT(mHeuristicTraceTagData->min.c_str(), min->refreshRate.getIntValue());
        ATRACE_INT(mHeuristicTraceTagData->consistent.c_str(), consistent);
    }

    return consistent;
}

} // namespace android::scheduler

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic pop // ignored "-Wextra"
