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

package android.os;

import android.os.ParcelFileDescriptor;

/**
 * Interface for accessing the RPC server of a service.
 *
 * @hide
 */
interface IAccessor {
    /**
     * Adds a connection to the RPC server of the service managed by the IAccessor.
     *
     * This method can be called multiple times to establish multiple distinct
     * connections to the same RPC server.
     *
     * @return A file descriptor connected to the RPC session of the service managed
     *         by IAccessor.
     */
    ParcelFileDescriptor addConnection();

    // TODO(b/350941051): Add API for debugging.
}
