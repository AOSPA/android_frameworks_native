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

#include "SensorInputMapper.h"

#include <vector>

#include <EventHub.h>
#include <NotifyArgs.h>
#include <gtest/gtest.h>
#include <input/Input.h>
#include <input/InputDevice.h>
#include <linux/input-event-codes.h>

#include "InputMapperTest.h"

namespace android {

class SensorInputMapperTest : public InputMapperTest {
protected:
    static const int32_t ACCEL_RAW_MIN;
    static const int32_t ACCEL_RAW_MAX;
    static const int32_t ACCEL_RAW_FUZZ;
    static const int32_t ACCEL_RAW_FLAT;
    static const int32_t ACCEL_RAW_RESOLUTION;

    static const int32_t GYRO_RAW_MIN;
    static const int32_t GYRO_RAW_MAX;
    static const int32_t GYRO_RAW_FUZZ;
    static const int32_t GYRO_RAW_FLAT;
    static const int32_t GYRO_RAW_RESOLUTION;

    static const float GRAVITY_MS2_UNIT;
    static const float DEGREE_RADIAN_UNIT;

    void prepareAccelAxes();
    void prepareGyroAxes();
    void setAccelProperties();
    void setGyroProperties();
    void SetUp() override { InputMapperTest::SetUp(DEVICE_CLASSES | InputDeviceClass::SENSOR); }
};

const int32_t SensorInputMapperTest::ACCEL_RAW_MIN = -32768;
const int32_t SensorInputMapperTest::ACCEL_RAW_MAX = 32768;
const int32_t SensorInputMapperTest::ACCEL_RAW_FUZZ = 16;
const int32_t SensorInputMapperTest::ACCEL_RAW_FLAT = 0;
const int32_t SensorInputMapperTest::ACCEL_RAW_RESOLUTION = 8192;

const int32_t SensorInputMapperTest::GYRO_RAW_MIN = -2097152;
const int32_t SensorInputMapperTest::GYRO_RAW_MAX = 2097152;
const int32_t SensorInputMapperTest::GYRO_RAW_FUZZ = 16;
const int32_t SensorInputMapperTest::GYRO_RAW_FLAT = 0;
const int32_t SensorInputMapperTest::GYRO_RAW_RESOLUTION = 1024;

const float SensorInputMapperTest::GRAVITY_MS2_UNIT = 9.80665f;
const float SensorInputMapperTest::DEGREE_RADIAN_UNIT = 0.0174533f;

void SensorInputMapperTest::prepareAccelAxes() {
    mFakeEventHub->addAbsoluteAxis(EVENTHUB_ID, ABS_X, ACCEL_RAW_MIN, ACCEL_RAW_MAX, ACCEL_RAW_FUZZ,
                                   ACCEL_RAW_FLAT, ACCEL_RAW_RESOLUTION);
    mFakeEventHub->addAbsoluteAxis(EVENTHUB_ID, ABS_Y, ACCEL_RAW_MIN, ACCEL_RAW_MAX, ACCEL_RAW_FUZZ,
                                   ACCEL_RAW_FLAT, ACCEL_RAW_RESOLUTION);
    mFakeEventHub->addAbsoluteAxis(EVENTHUB_ID, ABS_Z, ACCEL_RAW_MIN, ACCEL_RAW_MAX, ACCEL_RAW_FUZZ,
                                   ACCEL_RAW_FLAT, ACCEL_RAW_RESOLUTION);
}

void SensorInputMapperTest::prepareGyroAxes() {
    mFakeEventHub->addAbsoluteAxis(EVENTHUB_ID, ABS_RX, GYRO_RAW_MIN, GYRO_RAW_MAX, GYRO_RAW_FUZZ,
                                   GYRO_RAW_FLAT, GYRO_RAW_RESOLUTION);
    mFakeEventHub->addAbsoluteAxis(EVENTHUB_ID, ABS_RY, GYRO_RAW_MIN, GYRO_RAW_MAX, GYRO_RAW_FUZZ,
                                   GYRO_RAW_FLAT, GYRO_RAW_RESOLUTION);
    mFakeEventHub->addAbsoluteAxis(EVENTHUB_ID, ABS_RZ, GYRO_RAW_MIN, GYRO_RAW_MAX, GYRO_RAW_FUZZ,
                                   GYRO_RAW_FLAT, GYRO_RAW_RESOLUTION);
}

void SensorInputMapperTest::setAccelProperties() {
    mFakeEventHub->addSensorAxis(EVENTHUB_ID, /* absCode */ 0, InputDeviceSensorType::ACCELEROMETER,
                                 /* sensorDataIndex */ 0);
    mFakeEventHub->addSensorAxis(EVENTHUB_ID, /* absCode */ 1, InputDeviceSensorType::ACCELEROMETER,
                                 /* sensorDataIndex */ 1);
    mFakeEventHub->addSensorAxis(EVENTHUB_ID, /* absCode */ 2, InputDeviceSensorType::ACCELEROMETER,
                                 /* sensorDataIndex */ 2);
    mFakeEventHub->setMscEvent(EVENTHUB_ID, MSC_TIMESTAMP);
    addConfigurationProperty("sensor.accelerometer.reportingMode", "0");
    addConfigurationProperty("sensor.accelerometer.maxDelay", "100000");
    addConfigurationProperty("sensor.accelerometer.minDelay", "5000");
    addConfigurationProperty("sensor.accelerometer.power", "1.5");
}

void SensorInputMapperTest::setGyroProperties() {
    mFakeEventHub->addSensorAxis(EVENTHUB_ID, /* absCode */ 3, InputDeviceSensorType::GYROSCOPE,
                                 /* sensorDataIndex */ 0);
    mFakeEventHub->addSensorAxis(EVENTHUB_ID, /* absCode */ 4, InputDeviceSensorType::GYROSCOPE,
                                 /* sensorDataIndex */ 1);
    mFakeEventHub->addSensorAxis(EVENTHUB_ID, /* absCode */ 5, InputDeviceSensorType::GYROSCOPE,
                                 /* sensorDataIndex */ 2);
    mFakeEventHub->setMscEvent(EVENTHUB_ID, MSC_TIMESTAMP);
    addConfigurationProperty("sensor.gyroscope.reportingMode", "0");
    addConfigurationProperty("sensor.gyroscope.maxDelay", "100000");
    addConfigurationProperty("sensor.gyroscope.minDelay", "5000");
    addConfigurationProperty("sensor.gyroscope.power", "0.8");
}

TEST_F(SensorInputMapperTest, GetSources) {
    SensorInputMapper& mapper = constructAndAddMapper<SensorInputMapper>();

    ASSERT_EQ(static_cast<uint32_t>(AINPUT_SOURCE_SENSOR), mapper.getSources());
}

TEST_F(SensorInputMapperTest, ProcessAccelerometerSensor) {
    setAccelProperties();
    prepareAccelAxes();
    SensorInputMapper& mapper = constructAndAddMapper<SensorInputMapper>();

    ASSERT_TRUE(mapper.enableSensor(InputDeviceSensorType::ACCELEROMETER,
                                    std::chrono::microseconds(10000),
                                    std::chrono::microseconds(0)));
    ASSERT_TRUE(mFakeEventHub->isDeviceEnabled(EVENTHUB_ID));
    process(mapper, ARBITRARY_TIME, READ_TIME, EV_ABS, ABS_X, 20000);
    process(mapper, ARBITRARY_TIME, READ_TIME, EV_ABS, ABS_Y, -20000);
    process(mapper, ARBITRARY_TIME, READ_TIME, EV_ABS, ABS_Z, 40000);
    process(mapper, ARBITRARY_TIME, READ_TIME, EV_MSC, MSC_TIMESTAMP, 1000);
    process(mapper, ARBITRARY_TIME, READ_TIME, EV_SYN, SYN_REPORT, 0);

    NotifySensorArgs args;
    std::vector<float> values = {20000.0f / ACCEL_RAW_RESOLUTION * GRAVITY_MS2_UNIT,
                                 -20000.0f / ACCEL_RAW_RESOLUTION * GRAVITY_MS2_UNIT,
                                 40000.0f / ACCEL_RAW_RESOLUTION * GRAVITY_MS2_UNIT};

    ASSERT_NO_FATAL_FAILURE(mFakeListener->assertNotifySensorWasCalled(&args));
    ASSERT_EQ(args.source, AINPUT_SOURCE_SENSOR);
    ASSERT_EQ(args.deviceId, DEVICE_ID);
    ASSERT_EQ(args.sensorType, InputDeviceSensorType::ACCELEROMETER);
    ASSERT_EQ(args.accuracy, InputDeviceSensorAccuracy::ACCURACY_HIGH);
    ASSERT_EQ(args.hwTimestamp, ARBITRARY_TIME);
    ASSERT_EQ(args.values, values);
    mapper.flushSensor(InputDeviceSensorType::ACCELEROMETER);
}

TEST_F(SensorInputMapperTest, ProcessGyroscopeSensor) {
    setGyroProperties();
    prepareGyroAxes();
    SensorInputMapper& mapper = constructAndAddMapper<SensorInputMapper>();

    ASSERT_TRUE(mapper.enableSensor(InputDeviceSensorType::GYROSCOPE,
                                    std::chrono::microseconds(10000),
                                    std::chrono::microseconds(0)));
    ASSERT_TRUE(mFakeEventHub->isDeviceEnabled(EVENTHUB_ID));
    process(mapper, ARBITRARY_TIME, READ_TIME, EV_ABS, ABS_RX, 20000);
    process(mapper, ARBITRARY_TIME, READ_TIME, EV_ABS, ABS_RY, -20000);
    process(mapper, ARBITRARY_TIME, READ_TIME, EV_ABS, ABS_RZ, 40000);
    process(mapper, ARBITRARY_TIME, READ_TIME, EV_MSC, MSC_TIMESTAMP, 1000);
    process(mapper, ARBITRARY_TIME, READ_TIME, EV_SYN, SYN_REPORT, 0);

    NotifySensorArgs args;
    std::vector<float> values = {20000.0f / GYRO_RAW_RESOLUTION * DEGREE_RADIAN_UNIT,
                                 -20000.0f / GYRO_RAW_RESOLUTION * DEGREE_RADIAN_UNIT,
                                 40000.0f / GYRO_RAW_RESOLUTION * DEGREE_RADIAN_UNIT};

    ASSERT_NO_FATAL_FAILURE(mFakeListener->assertNotifySensorWasCalled(&args));
    ASSERT_EQ(args.source, AINPUT_SOURCE_SENSOR);
    ASSERT_EQ(args.deviceId, DEVICE_ID);
    ASSERT_EQ(args.sensorType, InputDeviceSensorType::GYROSCOPE);
    ASSERT_EQ(args.accuracy, InputDeviceSensorAccuracy::ACCURACY_HIGH);
    ASSERT_EQ(args.hwTimestamp, ARBITRARY_TIME);
    ASSERT_EQ(args.values, values);
    mapper.flushSensor(InputDeviceSensorType::GYROSCOPE);
}

} // namespace android