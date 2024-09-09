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

#define LOG_TAG "LegacyResampler"

#include <algorithm>
#include <chrono>

#include <android-base/logging.h>
#include <android-base/properties.h>

#include <input/Resampler.h>
#include <utils/Timers.h>

using std::chrono::nanoseconds;

namespace android {

namespace {

const bool IS_DEBUGGABLE_BUILD =
#if defined(__ANDROID__)
        android::base::GetBoolProperty("ro.debuggable", false);
#else
        true;
#endif

bool debugResampling() {
    if (!IS_DEBUGGABLE_BUILD) {
        static const bool DEBUG_TRANSPORT_RESAMPLING =
                __android_log_is_loggable(ANDROID_LOG_DEBUG, LOG_TAG "Resampling",
                                          ANDROID_LOG_INFO);
        return DEBUG_TRANSPORT_RESAMPLING;
    }
    return __android_log_is_loggable(ANDROID_LOG_DEBUG, LOG_TAG "Resampling", ANDROID_LOG_INFO);
}

constexpr std::chrono::milliseconds RESAMPLE_LATENCY{5};

constexpr std::chrono::milliseconds RESAMPLE_MIN_DELTA{2};

constexpr std::chrono::milliseconds RESAMPLE_MAX_DELTA{20};

constexpr std::chrono::milliseconds RESAMPLE_MAX_PREDICTION{8};

inline float lerp(float a, float b, float alpha) {
    return a + alpha * (b - a);
}

const PointerCoords calculateResampledCoords(const PointerCoords& a, const PointerCoords& b,
                                             const float alpha) {
    // We use the value of alpha to initialize resampledCoords with the latest sample information.
    PointerCoords resampledCoords = (alpha < 1.0f) ? a : b;
    resampledCoords.isResampled = true;
    resampledCoords.setAxisValue(AMOTION_EVENT_AXIS_X, lerp(a.getX(), b.getX(), alpha));
    resampledCoords.setAxisValue(AMOTION_EVENT_AXIS_Y, lerp(a.getY(), b.getY(), alpha));
    return resampledCoords;
}
} // namespace

void LegacyResampler::updateLatestSamples(const MotionEvent& motionEvent) {
    const size_t motionEventSampleSize = motionEvent.getHistorySize() + 1;
    for (size_t i = 0; i < motionEventSampleSize; ++i) {
        Sample sample{static_cast<nanoseconds>(motionEvent.getHistoricalEventTime(i)),
                      *motionEvent.getPointerProperties(0),
                      motionEvent.getSamplePointerCoords()[i]};
        mLatestSamples.pushBack(sample);
    }
}

void LegacyResampler::interpolate(const nanoseconds resampleTime, MotionEvent& motionEvent,
                                  const InputMessage& futureSample) const {
    const Sample pastSample = mLatestSamples.back();
    const nanoseconds delta =
            static_cast<nanoseconds>(futureSample.body.motion.eventTime) - pastSample.eventTime;
    if (delta < RESAMPLE_MIN_DELTA) {
        LOG_IF(INFO, debugResampling()) << "Not resampled. Delta is too small: " << delta << "ns.";
        return;
    }
    const float alpha =
            std::chrono::duration<float, std::milli>(resampleTime - pastSample.eventTime) / delta;

    const PointerCoords resampledCoords =
            calculateResampledCoords(pastSample.pointer.coords,
                                     futureSample.body.motion.pointers[0].coords, alpha);
    motionEvent.addSample(resampleTime.count(), &resampledCoords, motionEvent.getId());
}

void LegacyResampler::extrapolate(const nanoseconds resampleTime, MotionEvent& motionEvent) const {
    if (mLatestSamples.size() < 2) {
        return;
    }
    const Sample pastSample = *(mLatestSamples.end() - 2);
    const Sample presentSample = *(mLatestSamples.end() - 1);
    const nanoseconds delta =
            static_cast<nanoseconds>(presentSample.eventTime - pastSample.eventTime);
    if (delta < RESAMPLE_MIN_DELTA) {
        LOG_IF(INFO, debugResampling()) << "Not resampled. Delta is too small: " << delta << "ns.";
        return;
    } else if (delta > RESAMPLE_MAX_DELTA) {
        LOG_IF(INFO, debugResampling()) << "Not resampled. Delta is too large: " << delta << "ns.";
        return;
    }
    // The farthest future time to which we can extrapolate. If the given resampleTime exceeds this,
    // we use this value as the resample time target.
    const nanoseconds farthestPrediction = static_cast<nanoseconds>(presentSample.eventTime) +
            std::min<nanoseconds>(delta / 2, RESAMPLE_MAX_PREDICTION);
    const nanoseconds newResampleTime =
            (resampleTime > farthestPrediction) ? (farthestPrediction) : (resampleTime);
    LOG_IF(INFO, debugResampling() && newResampleTime == farthestPrediction)
            << "Resample time is too far in the future. Adjusting prediction from "
            << (resampleTime - presentSample.eventTime) << " to "
            << (farthestPrediction - presentSample.eventTime) << "ns.";
    const float alpha =
            std::chrono::duration<float, std::milli>(newResampleTime - pastSample.eventTime) /
            delta;

    const PointerCoords resampledCoords =
            calculateResampledCoords(pastSample.pointer.coords, presentSample.pointer.coords,
                                     alpha);
    motionEvent.addSample(newResampleTime.count(), &resampledCoords, motionEvent.getId());
}

void LegacyResampler::resampleMotionEvent(const nanoseconds resampleTime, MotionEvent& motionEvent,
                                          const InputMessage* futureSample) {
    if (mPreviousDeviceId && *mPreviousDeviceId != motionEvent.getDeviceId()) {
        mLatestSamples.clear();
    }
    mPreviousDeviceId = motionEvent.getDeviceId();
    updateLatestSamples(motionEvent);
    if (futureSample) {
        interpolate(resampleTime, motionEvent, *futureSample);
    } else {
        extrapolate(resampleTime, motionEvent);
    }
    LOG_IF(INFO, debugResampling()) << "Not resampled. Not enough data.";
}
} // namespace android
