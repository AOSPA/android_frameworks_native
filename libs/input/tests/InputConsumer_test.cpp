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

#include <memory>
#include <optional>
#include <utility>

#include <TestInputChannel.h>
#include <TestLooper.h>
#include <android-base/logging.h>
#include <gtest/gtest.h>
#include <input/BlockingQueue.h>
#include <input/InputEventBuilders.h>
#include <utils/StrongPointer.h>

namespace android {

namespace {

using std::chrono::nanoseconds;

} // namespace

class InputConsumerTest : public testing::Test, public InputConsumerCallbacks {
protected:
    InputConsumerTest()
          : mClientTestChannel{std::make_shared<TestInputChannel>("TestChannel")},
            mTestLooper{std::make_shared<TestLooper>()} {
        Looper::setForThread(mTestLooper->getLooper());
        mConsumer =
                std::make_unique<InputConsumerNoResampling>(mClientTestChannel, mTestLooper, *this,
                                                            std::make_unique<LegacyResampler>());
    }

    void assertOnBatchedInputEventPendingWasCalled();

    std::shared_ptr<TestInputChannel> mClientTestChannel;
    std::shared_ptr<TestLooper> mTestLooper;
    std::unique_ptr<InputConsumerNoResampling> mConsumer;

    BlockingQueue<std::unique_ptr<KeyEvent>> mKeyEvents;
    BlockingQueue<std::unique_ptr<MotionEvent>> mMotionEvents;
    BlockingQueue<std::unique_ptr<FocusEvent>> mFocusEvents;
    BlockingQueue<std::unique_ptr<CaptureEvent>> mCaptureEvents;
    BlockingQueue<std::unique_ptr<DragEvent>> mDragEvents;
    BlockingQueue<std::unique_ptr<TouchModeEvent>> mTouchModeEvents;

private:
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

void InputConsumerTest::assertOnBatchedInputEventPendingWasCalled() {
    ASSERT_GT(mOnBatchedInputEventPendingInvocationCount, 0UL)
            << "onBatchedInputEventPending has not been called.";
    --mOnBatchedInputEventPendingInvocationCount;
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

    mTestLooper->invokeCallback(mClientTestChannel->getFd(), ALOOPER_EVENT_INPUT);

    assertOnBatchedInputEventPendingWasCalled();

    mConsumer->consumeBatchedInputEvents(std::nullopt);

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

    mTestLooper->invokeCallback(mClientTestChannel->getFd(), ALOOPER_EVENT_INPUT);

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
} // namespace android
