/**
 * Copyright 2024 The Android Open Source Project
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

#define LOG_TAG "OneEuroFilter"

#include <chrono>
#include <cmath>

#include <android-base/logging.h>
#include <input/CoordinateFilter.h>

namespace android {
namespace {

inline float cutoffFreq(float minCutoffFreq, float beta, float filteredSpeed) {
    return minCutoffFreq + beta * std::abs(filteredSpeed);
}

inline float smoothingFactor(std::chrono::duration<float> samplingPeriod, float cutoffFreq) {
    return samplingPeriod.count() / (samplingPeriod.count() + (1.0 / (2.0 * M_PI * cutoffFreq)));
}

inline float lowPassFilter(float rawPosition, float prevFilteredPosition, float smoothingFactor) {
    return smoothingFactor * rawPosition + (1 - smoothingFactor) * prevFilteredPosition;
}

} // namespace

OneEuroFilter::OneEuroFilter(float minCutoffFreq, float beta, float speedCutoffFreq)
      : mMinCutoffFreq{minCutoffFreq}, mBeta{beta}, mSpeedCutoffFreq{speedCutoffFreq} {}

float OneEuroFilter::filter(std::chrono::duration<float> timestamp, float rawPosition) {
    LOG_IF(FATAL, mPrevFilteredPosition.has_value() && (timestamp <= *mPrevTimestamp))
            << "Timestamp must be greater than mPrevTimestamp";

    const std::chrono::duration<float> samplingPeriod = (mPrevTimestamp.has_value())
            ? (timestamp - *mPrevTimestamp)
            : std::chrono::duration<float>{1.0};

    const float rawVelocity = (mPrevFilteredPosition.has_value())
            ? ((rawPosition - *mPrevFilteredPosition) / samplingPeriod.count())
            : 0.0;

    const float speedSmoothingFactor = smoothingFactor(samplingPeriod, mSpeedCutoffFreq);

    const float filteredVelocity = (mPrevFilteredVelocity.has_value())
            ? lowPassFilter(rawVelocity, *mPrevFilteredVelocity, speedSmoothingFactor)
            : rawVelocity;

    const float positionCutoffFreq = cutoffFreq(mMinCutoffFreq, mBeta, filteredVelocity);

    const float positionSmoothingFactor = smoothingFactor(samplingPeriod, positionCutoffFreq);

    const float filteredPosition = (mPrevFilteredPosition.has_value())
            ? lowPassFilter(rawPosition, *mPrevFilteredPosition, positionSmoothingFactor)
            : rawPosition;

    mPrevTimestamp = timestamp;
    mPrevRawPosition = rawPosition;
    mPrevFilteredVelocity = filteredVelocity;
    mPrevFilteredPosition = filteredPosition;

    return filteredPosition;
}

} // namespace android
