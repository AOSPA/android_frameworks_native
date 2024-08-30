/*
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

#include "JoystickInputMapper.h"

#include <optional>

#include <EventHub.h>
#include <NotifyArgs.h>
#include <ftl/flags.h>
#include <gtest/gtest.h>
#include <input/DisplayViewport.h>
#include <linux/input-event-codes.h>
#include <ui/LogicalDisplayId.h>

#include "InputMapperTest.h"
#include "TestConstants.h"

namespace android {

namespace {

using namespace ftl::flag_operators;

} // namespace

class JoystickInputMapperTest : public InputMapperTest {
protected:
    static const int32_t RAW_X_MIN;
    static const int32_t RAW_X_MAX;
    static const int32_t RAW_Y_MIN;
    static const int32_t RAW_Y_MAX;

    static constexpr ui::LogicalDisplayId VIRTUAL_DISPLAY_ID = ui::LogicalDisplayId{1};
    static const char* const VIRTUAL_DISPLAY_UNIQUE_ID;

    void SetUp() override {
        InputMapperTest::SetUp(InputDeviceClass::JOYSTICK | InputDeviceClass::EXTERNAL);
    }
    void prepareAxes() {
        mFakeEventHub->addAbsoluteAxis(EVENTHUB_ID, ABS_X, RAW_X_MIN, RAW_X_MAX, 0, 0);
        mFakeEventHub->addAbsoluteAxis(EVENTHUB_ID, ABS_Y, RAW_Y_MIN, RAW_Y_MAX, 0, 0);
    }

    void processAxis(JoystickInputMapper& mapper, int32_t axis, int32_t value) {
        process(mapper, ARBITRARY_TIME, READ_TIME, EV_ABS, axis, value);
    }

    void processSync(JoystickInputMapper& mapper) {
        process(mapper, ARBITRARY_TIME, READ_TIME, EV_SYN, SYN_REPORT, 0);
    }

    void prepareVirtualDisplay(ui::Rotation orientation) {
        setDisplayInfoAndReconfigure(VIRTUAL_DISPLAY_ID, /*width=*/400, /*height=*/500, orientation,
                                     VIRTUAL_DISPLAY_UNIQUE_ID, /*physicalPort=*/std::nullopt,
                                     ViewportType::VIRTUAL);
    }
};

const int32_t JoystickInputMapperTest::RAW_X_MIN = -32767;
const int32_t JoystickInputMapperTest::RAW_X_MAX = 32767;
const int32_t JoystickInputMapperTest::RAW_Y_MIN = -32767;
const int32_t JoystickInputMapperTest::RAW_Y_MAX = 32767;
const char* const JoystickInputMapperTest::VIRTUAL_DISPLAY_UNIQUE_ID = "virtual:1";

TEST_F(JoystickInputMapperTest, Configure_AssignsDisplayUniqueId) {
    prepareAxes();
    JoystickInputMapper& mapper = constructAndAddMapper<JoystickInputMapper>();

    mFakePolicy->addInputUniqueIdAssociation(DEVICE_LOCATION, VIRTUAL_DISPLAY_UNIQUE_ID);

    prepareVirtualDisplay(ui::ROTATION_0);

    // Send an axis event
    processAxis(mapper, ABS_X, 100);
    processSync(mapper);

    NotifyMotionArgs args;
    ASSERT_NO_FATAL_FAILURE(mFakeListener->assertNotifyMotionWasCalled(&args));
    ASSERT_EQ(VIRTUAL_DISPLAY_ID, args.displayId);

    // Send another axis event
    processAxis(mapper, ABS_Y, 100);
    processSync(mapper);

    ASSERT_NO_FATAL_FAILURE(mFakeListener->assertNotifyMotionWasCalled(&args));
    ASSERT_EQ(VIRTUAL_DISPLAY_ID, args.displayId);
}

} // namespace android
