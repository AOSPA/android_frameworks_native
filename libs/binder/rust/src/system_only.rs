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

use crate::proxy::SpIBinder;
use crate::sys;

use std::ffi::{c_void, CStr, CString};
use std::os::raw::c_char;

use libc::sockaddr;
use nix::sys::socket::{SockaddrLike, UnixAddr, VsockAddr};
use std::sync::Arc;
use std::{fmt, ptr};

/// Rust wrapper around ABinderRpc_Accessor objects for RPC binder service management.
///
/// Dropping the `Accessor` will drop the underlying object and the binder it owns.
pub struct Accessor {
    accessor: *mut sys::ABinderRpc_Accessor,
}

impl fmt::Debug for Accessor {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "ABinderRpc_Accessor({:p})", self.accessor)
    }
}

/// Socket connection info required for libbinder to connect to a service.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ConnectionInfo {
    /// For vsock connection
    Vsock(VsockAddr),
    /// For unix domain socket connection
    Unix(UnixAddr),
}

/// Safety: A `Accessor` is a wrapper around `ABinderRpc_Accessor` which is
/// `Sync` and `Send`. As
/// `ABinderRpc_Accessor` is threadsafe, this structure is too.
/// The Fn owned the Accessor has `Sync` and `Send` properties
unsafe impl Send for Accessor {}

/// Safety: A `Accessor` is a wrapper around `ABinderRpc_Accessor` which is
/// `Sync` and `Send`. As `ABinderRpc_Accessor` is threadsafe, this structure is too.
/// The Fn owned the Accessor has `Sync` and `Send` properties
unsafe impl Sync for Accessor {}

impl Accessor {
    /// Create a new accessor that will call the given callback when its
    /// connection info is required.
    /// The callback object and all objects it captures are owned by the Accessor
    /// and will be deleted some time after the Accessor is Dropped. If the callback
    /// is being called when the Accessor is Dropped, the callback will not be deleted
    /// immediately.
    pub fn new<F>(instance: &str, callback: F) -> Accessor
    where
        F: Fn(&str) -> Option<ConnectionInfo> + Send + Sync + 'static,
    {
        let callback: *mut c_void = Arc::into_raw(Arc::new(callback)) as *mut c_void;
        let inst = CString::new(instance).unwrap();

        // Safety: The function pointer is a valid connection_info callback.
        // This call returns an owned `ABinderRpc_Accessor` pointer which
        // must be destroyed via `ABinderRpc_Accessor_delete` when no longer
        // needed.
        // When the underlying ABinderRpc_Accessor is deleted, it will call
        // the cookie_decr_refcount callback to release its strong ref.
        let accessor = unsafe {
            sys::ABinderRpc_Accessor_new(
                inst.as_ptr(),
                Some(Self::connection_info::<F>),
                callback,
                Some(Self::cookie_decr_refcount::<F>),
            )
        };

        Accessor { accessor }
    }

    /// Get the underlying binder for this Accessor for when it needs to be either
    /// registered with service manager or sent to another process.
    pub fn as_binder(&self) -> Option<SpIBinder> {
        // Safety: `ABinderRpc_Accessor_asBinder` returns either a null pointer or a
        // valid pointer to an owned `AIBinder`. Either of these values is safe to
        // pass to `SpIBinder::from_raw`.
        unsafe { SpIBinder::from_raw(sys::ABinderRpc_Accessor_asBinder(self.accessor)) }
    }

    /// Callback invoked from C++ when the connection info is needed.
    ///
    /// # Safety
    ///
    /// The `instance` parameter must be a non-null pointer to a valid C string for
    /// CStr::from_ptr. The memory must contain a valid null terminator at the end of
    /// the string within isize::MAX from the pointer. The memory must not be mutated for
    /// the duration of this function  call and must be valid for reads from the pointer
    /// to the null terminator.
    /// The `cookie` parameter must be the cookie for an `Arc<F>` and
    /// the caller must hold a ref-count to it.
    unsafe extern "C" fn connection_info<F>(
        instance: *const c_char,
        cookie: *mut c_void,
    ) -> *mut binder_ndk_sys::ABinderRpc_ConnectionInfo
    where
        F: Fn(&str) -> Option<ConnectionInfo> + Send + Sync + 'static,
    {
        if cookie.is_null() || instance.is_null() {
            log::error!("Cookie({cookie:p}) or instance({instance:p}) is null!");
            return ptr::null_mut();
        }
        // Safety: The caller promises that `cookie` is for an Arc<F>.
        let callback = unsafe { (cookie as *const F).as_ref().unwrap() };

        // Safety: The caller in libbinder_ndk will have already verified this is a valid
        // C string
        let inst = unsafe {
            match CStr::from_ptr(instance).to_str() {
                Ok(s) => s,
                Err(err) => {
                    log::error!("Failed to get a valid C string! {err:?}");
                    return ptr::null_mut();
                }
            }
        };

        let connection = match callback(inst) {
            Some(con) => con,
            None => {
                return ptr::null_mut();
            }
        };

        match connection {
            ConnectionInfo::Vsock(addr) => {
                // Safety: The sockaddr is being copied in the NDK API
                unsafe { sys::ABinderRpc_ConnectionInfo_new(addr.as_ptr(), addr.len()) }
            }
            ConnectionInfo::Unix(addr) => {
                // Safety: The sockaddr is being copied in the NDK API
                // The cast is from sockaddr_un* to sockaddr*.
                unsafe {
                    sys::ABinderRpc_ConnectionInfo_new(addr.as_ptr() as *const sockaddr, addr.len())
                }
            }
        }
    }

    /// Callback that decrements the ref-count.
    /// This is invoked from C++ when a binder is unlinked.
    ///
    /// # Safety
    ///
    /// The `cookie` parameter must be the cookie for an `Arc<F>` and
    /// the owner must give up a ref-count to it.
    unsafe extern "C" fn cookie_decr_refcount<F>(cookie: *mut c_void)
    where
        F: Fn(&str) -> Option<ConnectionInfo> + Send + Sync + 'static,
    {
        // Safety: The caller promises that `cookie` is for an Arc<F>.
        unsafe { Arc::decrement_strong_count(cookie as *const F) };
    }
}

impl Drop for Accessor {
    fn drop(&mut self) {
        // Safety: `self.accessor` is always a valid, owned
        // `ABinderRpc_Accessor` pointer returned by
        // `ABinderRpc_Accessor_new` when `self` was created. This delete
        // method can only be called once when `self` is dropped.
        unsafe {
            sys::ABinderRpc_Accessor_delete(self.accessor);
        }
    }
}
