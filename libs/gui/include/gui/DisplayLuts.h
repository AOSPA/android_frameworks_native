/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <android-base/unique_fd.h>
#include <vector>

namespace android::gui {

struct DisplayLuts {
public:
    struct Entry {
        int32_t dimension;
        int32_t size;
        int32_t samplingKey;
    };

    DisplayLuts() {}

    DisplayLuts(base::unique_fd lutfd, std::vector<int32_t> lutoffsets,
                std::vector<int32_t> lutdimensions, std::vector<int32_t> lutsizes,
                std::vector<int32_t> lutsamplingKeys) {
        fd = std::move(lutfd);
        offsets = lutoffsets;
        lutProperties.reserve(offsets.size());
        for (size_t i = 0; i < lutoffsets.size(); i++) {
            Entry entry{lutdimensions[i], lutsizes[i], lutsamplingKeys[i]};
            lutProperties.emplace_back(entry);
        }
    }

    base::unique_fd& getLutFileDescriptor() { return fd; }

    std::vector<Entry> lutProperties;
    std::vector<int32_t> offsets;

private:
    base::unique_fd fd;
}; // struct DisplayLuts

} // namespace android::gui