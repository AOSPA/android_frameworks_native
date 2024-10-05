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

#include <utils/Looper.h>
#include <utils/StrongPointer.h>

namespace android {

/**
 * LooperInterface allows the use of TestLooper in InputConsumerNoResampling without reassigning to
 * Looper. LooperInterface is needed to control how InputConsumerNoResampling consumes and batches
 * InputMessages.
 */
class LooperInterface {
public:
    virtual ~LooperInterface() = default;

    virtual int addFd(int fd, int ident, int events, const sp<LooperCallback>& callback,
                      void* data) = 0;
    virtual int removeFd(int fd) = 0;

    virtual sp<Looper> getLooper() const = 0;
};
} // namespace android
