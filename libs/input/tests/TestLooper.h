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

#include <map>

#include <input/LooperInterface.h>

namespace android {
/**
 * TestLooper provides a mechanism to directly trigger Looper's callback.
 */
class TestLooper final : public LooperInterface {
public:
    TestLooper();

    /**
     * Adds a file descriptor to mCallbacks. Ident, events, and data parameters are ignored. If
     * addFd is called with an existent file descriptor and a different callback, the previous
     * callback is overwritten.
     */
    int addFd(int fd, int ident, int events, const sp<LooperCallback>& callback,
              void* data) override;

    /**
     * Removes a file descriptor from mCallbacks. If fd is not in mCallbacks, returns FAILURE.
     */
    int removeFd(int fd) override;

    /**
     * Calls handleEvent of the file descriptor. Fd must be in mCallbacks. Otherwise, invokeCallback
     * fatally logs.
     */
    void invokeCallback(int fd, int events);

    sp<Looper> getLooper() const override;

private:
    std::map<int /*fd*/, sp<LooperCallback>> mCallbacks;
    sp<Looper> mLooper;
};
} // namespace android