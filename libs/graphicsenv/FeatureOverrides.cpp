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

#include <graphicsenv/FeatureOverrides.h>

#include <android-base/stringprintf.h>

namespace android {

using base::StringAppendF;

std::string FeatureConfig::toString() const {
    std::string result;
    StringAppendF(&result, "Feature: %s\n", mFeatureName.c_str());
    StringAppendF(&result, "      Status: %s\n", mEnabled ? "enabled" : "disabled");

    return result;
}

std::string FeatureOverrides::toString() const {
    std::string result;
    result.append("Global Features:\n");
    for (auto& cfg : mGlobalFeatures) {
        result.append("  " + cfg.toString());
    }
    result.append("\n");
    result.append("Package Features:\n");
    for (const auto& packageFeature : mPackageFeatures) {
        result.append("  Package:");
        StringAppendF(&result, " %s\n", packageFeature.first.c_str());
        for (auto& cfg : packageFeature.second) {
            result.append("    " + cfg.toString());
        }
    }

    return result;
}

} // namespace android
