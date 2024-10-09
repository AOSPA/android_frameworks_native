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

#include <input/InputConsumerNoResampling.h>

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <optional>

#include <TestEventMatchers.h>
#include <TestInputChannel.h>
#include <android-base/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <input/BlockingQueue.h>
#include <input/Input.h>
#include <input/InputEventBuilders.h>
#include <input/Resampler.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>

namespace android {

namespace {

using std::chrono::nanoseconds;

using ::testing::AllOf;
using ::testing::Matcher;

struct Pointer {
    int32_t id{0};
    ToolType toolType{ToolType::FINGER};
    float x{0.0f};
    float y{0.0f};
    bool isResampled{false};

    PointerBuilder asPointerBuilder() const {
        return PointerBuilder{id, toolType}.x(x).y(y).isResampled(isResampled);
    }
};
} // namespace

class InputConsumerTest : public testing::Test, public InputConsumerCallbacks {
protected:
    InputConsumerTest()
          : mClientTestChannel{std::make_shared<TestInputChannel>("TestChannel")},
            mLooper{sp<Looper>::make(/*allowNonCallbacks=*/false)} {
        Looper::setForThread(mLooper);
        mConsumer = std::make_unique<
                InputConsumerNoResampling>(mClientTestChannel, mLooper, *this,
                                           []() { return std::make_unique<LegacyResampler>(); });
    }

    void invokeLooperCallback() const {
        sp<LooperCallback> callback;
        ASSERT_TRUE(mLooper->getFdStateDebug(mClientTestChannel->getFd(), /*ident=*/nullptr,
                                             /*events=*/nullptr, &callback, /*data=*/nullptr));
        callback->handleEvent(mClientTestChannel->getFd(), ALOOPER_EVENT_INPUT, /*data=*/nullptr);
    }

    void assertOnBatchedInputEventPendingWasCalled() {
        ASSERT_GT(mOnBatchedInputEventPendingInvocationCount, 0UL)
                << "onBatchedInputEventPending has not been called.";
        --mOnBatchedInputEventPendingInvocationCount;
    }

    void assertReceivedMotionEvent(const Matcher<MotionEvent>& matcher) {
        std::unique_ptr<MotionEvent> motionEvent = mMotionEvents.pop();
        ASSERT_NE(motionEvent, nullptr);
        EXPECT_THAT(*motionEvent, matcher);
    }

    InputMessage nextPointerMessage(std::chrono::nanoseconds eventTime, DeviceId deviceId,
                                    int32_t action, const Pointer& pointer);

    std::shared_ptr<TestInputChannel> mClientTestChannel;
    sp<Looper> mLooper;
    std::unique_ptr<InputConsumerNoResampling> mConsumer;

    BlockingQueue<std::unique_ptr<KeyEvent>> mKeyEvents;
    BlockingQueue<std::unique_ptr<MotionEvent>> mMotionEvents;
    BlockingQueue<std::unique_ptr<FocusEvent>> mFocusEvents;
    BlockingQueue<std::unique_ptr<CaptureEvent>> mCaptureEvents;
    BlockingQueue<std::unique_ptr<DragEvent>> mDragEvents;
    BlockingQueue<std::unique_ptr<TouchModeEvent>> mTouchModeEvents;

private:
    uint32_t mLastSeq{0};
    size_t mOnBatchedInputEventPendingInvocationCount{0};

    // InputConsumerCallbacks interface
    void onKeyEvent(std::unique_ptr<KeyEvent> event, uint32_t seq) override {
        mKeyEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, true);
    }
    void onMotionEvent(std::unique_ptr<MotionEvent> event, uint32_t seq) override {
        mMotionEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, true);
    }
    void onBatchedInputEventPending(int32_t pendingBatchSource) override {
        if (!mConsumer->probablyHasInput()) {
            ADD_FAILURE() << "should deterministically have input because there is a batch";
        }
        ++mOnBatchedInputEventPendingInvocationCount;
    };
    void onFocusEvent(std::unique_ptr<FocusEvent> event, uint32_t seq) override {
        mFocusEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, true);
    };
    void onCaptureEvent(std::unique_ptr<CaptureEvent> event, uint32_t seq) override {
        mCaptureEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, true);
    };
    void onDragEvent(std::unique_ptr<DragEvent> event, uint32_t seq) override {
        mDragEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, true);
    }
    void onTouchModeEvent(std::unique_ptr<TouchModeEvent> event, uint32_t seq) override {
        mTouchModeEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, true);
    };
};

InputMessage InputConsumerTest::nextPointerMessage(std::chrono::nanoseconds eventTime,
                                                   DeviceId deviceId, int32_t action,
                                                   const Pointer& pointer) {
    ++mLastSeq;
    return InputMessageBuilder{InputMessage::Type::MOTION, mLastSeq}
            .eventTime(eventTime.count())
            .deviceId(deviceId)
            .source(AINPUT_SOURCE_TOUCHSCREEN)
            .action(action)
            .pointer(pointer.asPointerBuilder())
            .build();
}

TEST_F(InputConsumerTest, MessageStreamBatchedInMotionEvent) {
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/0}
                                               .eventTime(nanoseconds{0ms}.count())
                                               .action(AMOTION_EVENT_ACTION_DOWN)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/1}
                                               .eventTime(nanoseconds{5ms}.count())
                                               .action(AMOTION_EVENT_ACTION_MOVE)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/2}
                                               .eventTime(nanoseconds{10ms}.count())
                                               .action(AMOTION_EVENT_ACTION_MOVE)
                                               .build());

    mClientTestChannel->assertNoSentMessages();

    invokeLooperCallback();

    assertOnBatchedInputEventPendingWasCalled();

    mConsumer->consumeBatchedInputEvents(/*frameTime=*/std::nullopt);

    std::unique_ptr<MotionEvent> downMotionEvent = mMotionEvents.pop();
    ASSERT_NE(downMotionEvent, nullptr);

    std::unique_ptr<MotionEvent> moveMotionEvent = mMotionEvents.pop();
    ASSERT_NE(moveMotionEvent, nullptr);
    EXPECT_EQ(moveMotionEvent->getHistorySize() + 1, 3UL);

    mClientTestChannel->assertFinishMessage(/*seq=*/0, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
}

TEST_F(InputConsumerTest, LastBatchedSampleIsLessThanResampleTime) {
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/0}
                                               .eventTime(nanoseconds{0ms}.count())
                                               .action(AMOTION_EVENT_ACTION_DOWN)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/1}
                                               .eventTime(nanoseconds{5ms}.count())
                                               .action(AMOTION_EVENT_ACTION_MOVE)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/2}
                                               .eventTime(nanoseconds{10ms}.count())
                                               .action(AMOTION_EVENT_ACTION_MOVE)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/3}
                                               .eventTime(nanoseconds{15ms}.count())
                                               .action(AMOTION_EVENT_ACTION_MOVE)
                                               .build());

    mClientTestChannel->assertNoSentMessages();

    invokeLooperCallback();

    assertOnBatchedInputEventPendingWasCalled();

    mConsumer->consumeBatchedInputEvents(16'000'000 /*ns*/);

    std::unique_ptr<MotionEvent> downMotionEvent = mMotionEvents.pop();
    ASSERT_NE(downMotionEvent, nullptr);

    std::unique_ptr<MotionEvent> moveMotionEvent = mMotionEvents.pop();
    ASSERT_NE(moveMotionEvent, nullptr);
    const size_t numSamples = moveMotionEvent->getHistorySize() + 1;
    EXPECT_LT(moveMotionEvent->getHistoricalEventTime(numSamples - 2),
              moveMotionEvent->getEventTime());

    // Consume all remaining events before ending the test. Otherwise, the smart pointer that owns
    // consumer is set to null before destroying consumer. This leads to a member function call on a
    // null object.
    // TODO(b/332613662): Remove this workaround.
    mConsumer->consumeBatchedInputEvents(std::nullopt);

    mClientTestChannel->assertFinishMessage(/*seq=*/0, true);
    mClientTestChannel->assertFinishMessage(/*seq=*/1, true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, true);
}

TEST_F(InputConsumerTest, BatchedEventsMultiDeviceConsumption) {
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/0}
                                               .deviceId(0)
                                               .action(AMOTION_EVENT_ACTION_DOWN)
                                               .build());

    invokeLooperCallback();
    assertReceivedMotionEvent(AllOf(WithDeviceId(0), WithMotionAction(AMOTION_EVENT_ACTION_DOWN)));

    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/1}
                                               .deviceId(0)
                                               .action(AMOTION_EVENT_ACTION_MOVE)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/2}
                                               .deviceId(0)
                                               .action(AMOTION_EVENT_ACTION_MOVE)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/3}
                                               .deviceId(0)
                                               .action(AMOTION_EVENT_ACTION_MOVE)
                                               .build());

    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/4}
                                               .deviceId(1)
                                               .action(AMOTION_EVENT_ACTION_DOWN)
                                               .build());

    invokeLooperCallback();
    assertReceivedMotionEvent(AllOf(WithDeviceId(1), WithMotionAction(AMOTION_EVENT_ACTION_DOWN)));

    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/5}
                                               .deviceId(0)
                                               .action(AMOTION_EVENT_ACTION_UP)
                                               .build());

    invokeLooperCallback();
    assertReceivedMotionEvent(AllOf(WithDeviceId(0), WithMotionAction(AMOTION_EVENT_ACTION_MOVE)));

    mClientTestChannel->assertFinishMessage(/*seq=*/0, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/4, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
}

/**
 * The test supposes a 60Hz Vsync rate and a 200Hz input rate. The InputMessages are intertwined as
 * in a real use cases. The test's two devices should be resampled independently. Moreover, the
 * InputMessage stream layout for the test is:
 *
 * DOWN(0, 0ms)
 * MOVE(0, 5ms)
 * MOVE(0, 10ms)
 * DOWN(1, 15ms)
 *
 * CONSUME(16ms)
 *
 * MOVE(1, 20ms)
 * MOVE(1, 25ms)
 * MOVE(0, 30ms)
 *
 * CONSUME(32ms)
 *
 * MOVE(0, 35ms)
 * UP(1, 40ms)
 * UP(0, 45ms)
 *
 * CONSUME(48ms)
 *
 * The first field is device ID, and the second field is event time.
 */
TEST_F(InputConsumerTest, MultiDeviceResampling) {
    mClientTestChannel->enqueueMessage(nextPointerMessage(0ms, /*deviceId=*/0,
                                                          AMOTION_EVENT_ACTION_DOWN,
                                                          Pointer{.x = 0, .y = 0}));

    mClientTestChannel->assertNoSentMessages();

    invokeLooperCallback();
    assertReceivedMotionEvent(AllOf(WithDeviceId(0), WithMotionAction(AMOTION_EVENT_ACTION_DOWN),
                                    WithSampleCount(1)));

    mClientTestChannel->enqueueMessage(nextPointerMessage(5ms, /*deviceId=*/0,
                                                          AMOTION_EVENT_ACTION_MOVE,
                                                          Pointer{.x = 1.0f, .y = 2.0f}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(10ms, /*deviceId=*/0,
                                                          AMOTION_EVENT_ACTION_MOVE,
                                                          Pointer{.x = 2.0f, .y = 4.0f}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(15ms, /*deviceId=*/1,
                                                          AMOTION_EVENT_ACTION_DOWN,
                                                          Pointer{.x = 10.0f, .y = 10.0f}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(16'000'000 /*ns*/);

    assertReceivedMotionEvent(AllOf(WithDeviceId(1), WithMotionAction(AMOTION_EVENT_ACTION_DOWN),
                                    WithSampleCount(1)));
    assertReceivedMotionEvent(
            AllOf(WithDeviceId(0), WithMotionAction(AMOTION_EVENT_ACTION_MOVE), WithSampleCount(3),
                  WithSample(/*sampleIndex=*/2,
                             Sample{11ms,
                                    {PointerArgs{.x = 2.2f, .y = 4.4f, .isResampled = true}}})));

    mClientTestChannel->enqueueMessage(nextPointerMessage(20ms, /*deviceId=*/1,
                                                          AMOTION_EVENT_ACTION_MOVE,
                                                          Pointer{.x = 11.0f, .y = 12.0f}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(25ms, /*deviceId=*/1,
                                                          AMOTION_EVENT_ACTION_MOVE,
                                                          Pointer{.x = 12.0f, .y = 14.0f}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(30ms, /*deviceId=*/0,
                                                          AMOTION_EVENT_ACTION_MOVE,
                                                          Pointer{.x = 5.0f, .y = 6.0f}));

    invokeLooperCallback();
    assertOnBatchedInputEventPendingWasCalled();
    mConsumer->consumeBatchedInputEvents(32'000'000 /*ns*/);

    assertReceivedMotionEvent(
            AllOf(WithDeviceId(1), WithMotionAction(AMOTION_EVENT_ACTION_MOVE), WithSampleCount(3),
                  WithSample(/*sampleIndex=*/2,
                             Sample{27ms,
                                    {PointerArgs{.x = 12.4f, .y = 14.8f, .isResampled = true}}})));

    mClientTestChannel->enqueueMessage(nextPointerMessage(35ms, /*deviceId=*/0,
                                                          AMOTION_EVENT_ACTION_MOVE,
                                                          Pointer{.x = 8.0f, .y = 9.0f}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(40ms, /*deviceId=*/1,
                                                          AMOTION_EVENT_ACTION_UP,
                                                          Pointer{.x = 12.0f, .y = 14.0f}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(45ms, /*deviceId=*/0,
                                                          AMOTION_EVENT_ACTION_UP,
                                                          Pointer{.x = 8.0f, .y = 9.0f}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(48'000'000 /*ns*/);

    assertReceivedMotionEvent(
            AllOf(WithDeviceId(1), WithMotionAction(AMOTION_EVENT_ACTION_UP), WithSampleCount(1)));

    assertReceivedMotionEvent(
            AllOf(WithDeviceId(0), WithMotionAction(AMOTION_EVENT_ACTION_MOVE), WithSampleCount(3),
                  WithSample(/*sampleIndex=*/2,
                             Sample{37'500'000ns,
                                    {PointerArgs{.x = 9.5f, .y = 10.5f, .isResampled = true}}})));

    assertReceivedMotionEvent(
            AllOf(WithDeviceId(0), WithMotionAction(AMOTION_EVENT_ACTION_UP), WithSampleCount(1)));

    // The sequence order is based on the expected consumption. Each sequence number corresponds to
    // one of the previously enqueued messages.
    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/4, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/5, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/6, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/9, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/7, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/8, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/10, /*handled=*/true);
}
} // namespace android
