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

#include <TestLooper.h>

#include <android-base/logging.h>

namespace android {

TestLooper::TestLooper() : mLooper(sp<Looper>::make(/*allowNonCallbacks=*/false)) {}

int TestLooper::addFd(int fd, int ident, int events, const sp<LooperCallback>& callback,
                      void* data) {
    mCallbacks[fd] = callback;
    constexpr int SUCCESS{1};
    return SUCCESS;
}

int TestLooper::removeFd(int fd) {
    if (auto it = mCallbacks.find(fd); it != mCallbacks.cend()) {
        mCallbacks.erase(fd);
        constexpr int SUCCESS{1};
        return SUCCESS;
    }
    constexpr int FAILURE{0};
    return FAILURE;
}

void TestLooper::invokeCallback(int fd, int events) {
    auto it = mCallbacks.find(fd);
    LOG_IF(FATAL, it == mCallbacks.cend()) << "Fd does not exist in mCallbacks.";
    mCallbacks[fd]->handleEvent(fd, events, /*data=*/nullptr);
}

sp<Looper> TestLooper::getLooper() const {
    return mLooper;
}
} // namespace android