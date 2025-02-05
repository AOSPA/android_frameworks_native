/*
 * Copyright 2025 The Android Open Source Project
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

#undef LOG_TAG
#define LOG_TAG "gpuservice_unittest"

#include <android-base/file.h>
#include <log/log.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <com_android_graphics_graphicsenv_flags.h>
#include <feature_override/FeatureOverrideParser.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace android {
namespace {

std::string getTestBinarypbPath(const std::string &filename) {
    std::string path = android::base::GetExecutableDirectory();
    path.append("/");
    path.append(filename);

    return path;
}

class FeatureOverrideParserMock : public FeatureOverrideParser {
public:
    MOCK_METHOD(std::string, getFeatureOverrideFilePath, (), (const, override));
};

class FeatureOverrideParserTest : public testing::Test {
public:
    FeatureOverrideParserTest() {
        const ::testing::TestInfo *const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
        ALOGD("**** Setting up for %s.%s\n", test_info->test_case_name(), test_info->name());
    }

    ~FeatureOverrideParserTest() {
        const ::testing::TestInfo *const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
        ALOGD("**** Tearing down after %s.%s\n", test_info->test_case_name(),
              test_info->name());
    }

    void SetUp() override {
        const std::string filename = "gpuservice_unittest_feature_config_vk.binarypb";

        EXPECT_CALL(mFeatureOverrideParser, getFeatureOverrideFilePath())
            .WillRepeatedly(Return(getTestBinarypbPath(filename)));
    }

    FeatureOverrideParserMock mFeatureOverrideParser;
};

testing::AssertionResult validateFeatureConfigTestTxtpbSizes(FeatureOverrides overrides) {
    size_t expectedGlobalFeaturesSize = 1;
    if (overrides.mGlobalFeatures.size() != expectedGlobalFeaturesSize) {
        return testing::AssertionFailure()
                << "overrides.mGlobalFeatures.size(): " << overrides.mGlobalFeatures.size()
                << ", expected: " << expectedGlobalFeaturesSize;
    }

    size_t expectedPackageFeaturesSize = 1;
    if (overrides.mPackageFeatures.size() != expectedPackageFeaturesSize) {
        return testing::AssertionFailure()
                << "overrides.mPackageFeatures.size(): " << overrides.mPackageFeatures.size()
                << ", expected: " << expectedPackageFeaturesSize;
    }

    return testing::AssertionSuccess();
}

testing::AssertionResult validateFeatureConfigTestForceReadTxtpbSizes(FeatureOverrides overrides) {
    size_t expectedGlobalFeaturesSize = 1;
    if (overrides.mGlobalFeatures.size() != expectedGlobalFeaturesSize) {
        return testing::AssertionFailure()
                << "overrides.mGlobalFeatures.size(): " << overrides.mGlobalFeatures.size()
                << ", expected: " << expectedGlobalFeaturesSize;
    }

    size_t expectedPackageFeaturesSize = 0;
    if (overrides.mPackageFeatures.size() != expectedPackageFeaturesSize) {
        return testing::AssertionFailure()
                << "overrides.mPackageFeatures.size(): " << overrides.mPackageFeatures.size()
                << ", expected: " << expectedPackageFeaturesSize;
    }

    return testing::AssertionSuccess();
}

testing::AssertionResult validateGlobalOverrides1(FeatureOverrides overrides) {
    const int kTestFeatureIndex = 0;
    const std::string expectedFeatureName = "globalOverrides1";
    const FeatureConfig &cfg = overrides.mGlobalFeatures[kTestFeatureIndex];

    if (cfg.mFeatureName != expectedFeatureName) {
        return testing::AssertionFailure()
                << "cfg.mFeatureName: " << cfg.mFeatureName
                << ", expected: " << expectedFeatureName;
    }

    bool expectedEnabled = false;
    if (cfg.mEnabled != expectedEnabled) {
        return testing::AssertionFailure()
                << "cfg.mEnabled: " << cfg.mEnabled
                << ", expected: " << expectedEnabled;
    }

    return testing::AssertionSuccess();
}

TEST_F(FeatureOverrideParserTest, globalOverrides1) {
    FeatureOverrides overrides = mFeatureOverrideParser.getFeatureOverrides();

    EXPECT_TRUE(validateFeatureConfigTestTxtpbSizes(overrides));
    EXPECT_TRUE(validateGlobalOverrides1(overrides));
}

testing::AssertionResult validatePackageOverrides1(FeatureOverrides overrides) {
    const std::string expectedTestPackageName = "com.gpuservice_unittest.packageOverrides1";

    if (!overrides.mPackageFeatures.count(expectedTestPackageName)) {
        return testing::AssertionFailure()
                << "overrides.mPackageFeatures missing expected package: "
                << expectedTestPackageName;
    }

    const std::vector<FeatureConfig>& features =
            overrides.mPackageFeatures[expectedTestPackageName];

    size_t expectedFeaturesSize = 1;
    if (features.size() != expectedFeaturesSize) {
        return testing::AssertionFailure()
                << "features.size(): " << features.size()
                << ", expectedFeaturesSize: " << expectedFeaturesSize;
    }

    const std::string expectedFeatureName = "packageOverrides1";
    const FeatureConfig &cfg = features[0];

    bool expectedEnabled = true;
    if (cfg.mEnabled != expectedEnabled) {
        return testing::AssertionFailure()
                << "cfg.mEnabled: " << cfg.mEnabled
                << ", expected: " << expectedEnabled;
    }

    return testing::AssertionSuccess();
}

TEST_F(FeatureOverrideParserTest, packageOverrides1) {
    FeatureOverrides overrides = mFeatureOverrideParser.getFeatureOverrides();

    EXPECT_TRUE(validateFeatureConfigTestTxtpbSizes(overrides));
    EXPECT_TRUE(validatePackageOverrides1(overrides));
}

testing::AssertionResult validateForceFileRead(FeatureOverrides overrides) {
    const int kTestFeatureIndex = 0;
    const std::string expectedFeatureName = "forceFileRead";

    const FeatureConfig &cfg = overrides.mGlobalFeatures[kTestFeatureIndex];
    if (cfg.mFeatureName != expectedFeatureName) {
        return testing::AssertionFailure()
                << "cfg.mFeatureName: " << cfg.mFeatureName
                << ", expected: " << expectedFeatureName;
    }

    bool expectedEnabled = false;
    if (cfg.mEnabled != expectedEnabled) {
        return testing::AssertionFailure()
                << "cfg.mEnabled: " << cfg.mEnabled
                << ", expected: " << expectedEnabled;
    }

    return testing::AssertionSuccess();
}

TEST_F(FeatureOverrideParserTest, forceFileRead) {
    FeatureOverrides overrides = mFeatureOverrideParser.getFeatureOverrides();

    // Validate the "original" contents are present.
    EXPECT_TRUE(validateFeatureConfigTestTxtpbSizes(overrides));
    EXPECT_TRUE(validateGlobalOverrides1(overrides));

    // "Update" the config file.
    const std::string filename = "gpuservice_unittest_feature_config_vk_force_read.binarypb";
    EXPECT_CALL(mFeatureOverrideParser, getFeatureOverrideFilePath())
        .WillRepeatedly(Return(getTestBinarypbPath(filename)));

    mFeatureOverrideParser.forceFileRead();

    overrides = mFeatureOverrideParser.getFeatureOverrides();

    // Validate the new file contents were read and parsed.
    EXPECT_TRUE(validateFeatureConfigTestForceReadTxtpbSizes(overrides));
    EXPECT_TRUE(validateForceFileRead(overrides));
}

} // namespace
} // namespace android
