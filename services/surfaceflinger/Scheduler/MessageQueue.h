/*
 * Copyright (C) 2009 The Android Open Source Project
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

#include <cstdint>
#include <future>
#include <type_traits>
#include <utility>

#include <android-base/thread_annotations.h>
#include <android/gui/IDisplayEventConnection.h>
#include <private/gui/BitTube.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>
#include <utils/Timers.h>

#include <scheduler/Time.h>
#include <scheduler/VsyncId.h>

#include "EventThread.h"
#include "TracedOrdinal.h"
#include "VSyncDispatch.h"

namespace android {

struct ICompositor {
    virtual void configure() = 0;
    virtual bool commit(TimePoint frameTime, VsyncId, TimePoint expectedVsyncTime) = 0;
    virtual void composite(TimePoint frameTime, VsyncId) = 0;
    virtual void sample() = 0;

protected:
    ~ICompositor() = default;
};

template <typename F>
class Task : public MessageHandler {
    template <typename G>
    friend auto makeTask(G&&);

    template <typename... Args>
    friend sp<Task<F>> sp<Task<F>>::make(Args&&... args);

    explicit Task(F&& f) : mTask(std::move(f)) {}

    void handleMessage(const Message&) override { mTask(); }

    using T = std::invoke_result_t<F>;
    std::packaged_task<T()> mTask;
};

template <typename F>
inline auto makeTask(F&& f) {
    sp<Task<F>> task = sp<Task<F>>::make(std::forward<F>(f));
    return std::make_pair(task, task->mTask.get_future());
}

class MessageQueue {
public:
    virtual ~MessageQueue() = default;

    virtual void initVsync(scheduler::VSyncDispatch&, frametimeline::TokenManager&,
                           std::chrono::nanoseconds workDuration) = 0;
    virtual void setDuration(std::chrono::nanoseconds workDuration) = 0;
    virtual void waitMessage() = 0;
    virtual void postMessage(sp<MessageHandler>&&) = 0;
    virtual void scheduleConfigure() = 0;
    virtual void scheduleFrame() = 0;

    using Clock = std::chrono::steady_clock;
    virtual std::optional<Clock::time_point> getScheduledFrameTime() const = 0;
};

namespace impl {

class MessageQueue : public android::MessageQueue {
protected:
    class Handler : public MessageHandler {
        MessageQueue& mQueue;
        std::atomic_bool mFramePending = false;

        std::atomic<VsyncId> mVsyncId;
        std::atomic<TimePoint> mExpectedVsyncTime;

    public:
        explicit Handler(MessageQueue& queue) : mQueue(queue) {}
        void handleMessage(const Message& message) override;

        bool isFramePending() const;

        virtual void dispatchFrame(VsyncId, TimePoint expectedVsyncTime);
    };

    friend class Handler;

    // For tests.
    MessageQueue(ICompositor&, sp<Handler>);

    void vsyncCallback(nsecs_t vsyncTime, nsecs_t targetWakeupTime, nsecs_t readyTime);

private:
    virtual void onFrameSignal(ICompositor&, VsyncId, TimePoint expectedVsyncTime) = 0;

    ICompositor& mCompositor;
    const sp<Looper> mLooper;
    const sp<Handler> mHandler;

    struct Vsync {
        frametimeline::TokenManager* tokenManager = nullptr;
        std::unique_ptr<scheduler::VSyncCallbackRegistration> registration;

        mutable std::mutex mutex;
        TracedOrdinal<std::chrono::nanoseconds> workDuration
                GUARDED_BY(mutex) = {"VsyncWorkDuration-sf", std::chrono::nanoseconds(0)};
        TimePoint lastCallbackTime GUARDED_BY(mutex);
        std::optional<nsecs_t> scheduledFrameTime GUARDED_BY(mutex);
        TracedOrdinal<int> value = {"VSYNC-sf", 0};
    };

    Vsync mVsync;

public:
    explicit MessageQueue(ICompositor&);

    void initVsync(scheduler::VSyncDispatch&, frametimeline::TokenManager&,
                   std::chrono::nanoseconds workDuration) override;
    void setDuration(std::chrono::nanoseconds workDuration) override;

    void waitMessage() override;
    void postMessage(sp<MessageHandler>&&) override;

    void scheduleConfigure() override;
    void scheduleFrame() override;

    std::optional<Clock::time_point> getScheduledFrameTime() const override;
};

} // namespace impl
} // namespace android
