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

#pragma once

#include <chrono>
#include <optional>

#include <input/Input.h>
#include <input/InputTransport.h>
#include <input/RingBuffer.h>
#include <utils/Timers.h>

namespace android {

/**
 * Resampler is an interface for resampling MotionEvents. Every resampling implementation
 * must use this interface to enable resampling inside InputConsumer's logic.
 */
struct Resampler {
    virtual ~Resampler() = default;

    /**
     * Tries to resample motionEvent at resampleTime. The provided resampleTime must be greater than
     * the latest sample time of motionEvent. It is not guaranteed that resampling occurs at
     * resampleTime. Interpolation may occur is futureSample is available. Otherwise, motionEvent
     * may be resampled by another method, or not resampled at all. Furthermore, it is the
     * implementer's responsibility to guarantee the following:
     * - If resampling occurs, a single additional sample should be added to motionEvent. That is,
     * if motionEvent had N samples before being passed to Resampler, then it will have N + 1
     * samples by the end of the resampling. No other field of motionEvent should be modified.
     * - If resampling does not occur, then motionEvent must not be modified in any way.
     */
    virtual void resampleMotionEvent(const std::chrono::nanoseconds resampleTime,
                                     MotionEvent& motionEvent,
                                     const InputMessage* futureSample) = 0;
};

class LegacyResampler final : public Resampler {
public:
    /**
     * Tries to resample `motionEvent` at `resampleTime` by adding a resampled sample at the end of
     * `motionEvent` with eventTime equal to `resampleTime` and pointer coordinates determined by
     * linear interpolation or linear extrapolation. An earlier `resampleTime` will be used if
     * extrapolation takes place and `resampleTime` is too far in the future. If `futureSample` is
     * not null, interpolation will occur. If `futureSample` is null and there is enough historical
     * data, LegacyResampler will extrapolate. Otherwise, no resampling takes place and
     * `motionEvent` is unmodified.
     */
    void resampleMotionEvent(const std::chrono::nanoseconds resampleTime, MotionEvent& motionEvent,
                             const InputMessage* futureSample) override;

private:
    struct Pointer {
        PointerProperties properties;
        PointerCoords coords;
    };

    struct Sample {
        std::chrono::nanoseconds eventTime;
        Pointer pointer;

        Sample(const std::chrono::nanoseconds eventTime, const PointerProperties& properties,
               const PointerCoords& coords)
              : eventTime{eventTime}, pointer{properties, coords} {}
    };

    /**
     * Keeps track of the previous MotionEvent deviceId to enable comparison between the previous
     * and the current deviceId.
     */
    std::optional<DeviceId> mPreviousDeviceId;

    /**
     * Up to two latest samples from MotionEvent. Updated every time resampleMotionEvent is called.
     * Note: We store up to two samples in order to simplify the implementation. Although,
     * calculations are possible with only one previous sample.
     */
    RingBuffer<Sample> mLatestSamples{/*capacity=*/2};

    /**
     * Adds up to mLatestSamples.capacity() of motionEvent's latest samples to mLatestSamples. (If
     * motionEvent has fewer samples than mLatestSamples.capacity(), then the available samples are
     * added to mLatestSamples.)
     */
    void updateLatestSamples(const MotionEvent& motionEvent);

    /**
     * May add a sample at the end of motionEvent with eventTime equal to resampleTime, and
     * interpolated coordinates between the latest motionEvent sample and futureSample.
     */
    void interpolate(const std::chrono::nanoseconds resampleTime, MotionEvent& motionEvent,
                     const InputMessage& futureSample) const;

    /**
     * May add a sample at the end of motionEvent by extrapolating from the latest two samples. The
     * added sample either has eventTime equal to resampleTime, or an earlier time if resampleTime
     * is too far in the future.
     */
    void extrapolate(const std::chrono::nanoseconds resampleTime, MotionEvent& motionEvent) const;
};
} // namespace android