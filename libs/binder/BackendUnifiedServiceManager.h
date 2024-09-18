/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <android/os/BnServiceManager.h>
#include <android/os/IServiceManager.h>
#include <binder/IPCThreadState.h>

namespace android {

class BackendUnifiedServiceManager : public android::os::BnServiceManager {
public:
    explicit BackendUnifiedServiceManager(const sp<os::IServiceManager>& impl);

    sp<os::IServiceManager> getImpl();
    binder::Status getService(const ::std::string& name, sp<IBinder>* _aidl_return) override;
    binder::Status getService2(const ::std::string& name, os::Service* out) override;
    binder::Status checkService(const ::std::string& name, os::Service* out) override;
    binder::Status addService(const ::std::string& name, const sp<IBinder>& service,
                              bool allowIsolated, int32_t dumpPriority) override;
    binder::Status listServices(int32_t dumpPriority,
                                ::std::vector<::std::string>* _aidl_return) override;
    binder::Status registerForNotifications(const ::std::string& name,
                                            const sp<os::IServiceCallback>& callback) override;
    binder::Status unregisterForNotifications(const ::std::string& name,
                                              const sp<os::IServiceCallback>& callback) override;
    binder::Status isDeclared(const ::std::string& name, bool* _aidl_return) override;
    binder::Status getDeclaredInstances(const ::std::string& iface,
                                        ::std::vector<::std::string>* _aidl_return) override;
    binder::Status updatableViaApex(const ::std::string& name,
                                    ::std::optional<::std::string>* _aidl_return) override;
    binder::Status getUpdatableNames(const ::std::string& apexName,
                                     ::std::vector<::std::string>* _aidl_return) override;
    binder::Status getConnectionInfo(const ::std::string& name,
                                     ::std::optional<os::ConnectionInfo>* _aidl_return) override;
    binder::Status registerClientCallback(const ::std::string& name, const sp<IBinder>& service,
                                          const sp<os::IClientCallback>& callback) override;
    binder::Status tryUnregisterService(const ::std::string& name,
                                        const sp<IBinder>& service) override;
    binder::Status getServiceDebugInfo(::std::vector<os::ServiceDebugInfo>* _aidl_return) override;

    // for legacy ABI
    const String16& getInterfaceDescriptor() const override {
        return mTheRealServiceManager->getInterfaceDescriptor();
    }

private:
    sp<os::IServiceManager> mTheRealServiceManager;
    void toBinderService(const os::Service& in, os::Service* _out);
};

sp<BackendUnifiedServiceManager> getBackendUnifiedServiceManager();

} // namespace android