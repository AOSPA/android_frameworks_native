/*
 * Copyright (C) 2025 The Android Open Source Project
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

use crate::{
    binder::AsNative,
    error::{status_result, StatusCode},
    impl_deserialize_for_unstructured_parcelable, impl_serialize_for_unstructured_parcelable,
    parcel::{BorrowedParcel, UnstructuredParcelable},
};
use binder_ndk_sys::{
    APersistableBundle, APersistableBundle_delete, APersistableBundle_dup,
    APersistableBundle_erase, APersistableBundle_getBoolean, APersistableBundle_getBooleanVector,
    APersistableBundle_getDouble, APersistableBundle_getDoubleVector, APersistableBundle_getInt,
    APersistableBundle_getIntVector, APersistableBundle_getLong, APersistableBundle_getLongVector,
    APersistableBundle_getPersistableBundle, APersistableBundle_isEqual, APersistableBundle_new,
    APersistableBundle_putBoolean, APersistableBundle_putBooleanVector,
    APersistableBundle_putDouble, APersistableBundle_putDoubleVector, APersistableBundle_putInt,
    APersistableBundle_putIntVector, APersistableBundle_putLong, APersistableBundle_putLongVector,
    APersistableBundle_putPersistableBundle, APersistableBundle_putString,
    APersistableBundle_putStringVector, APersistableBundle_readFromParcel, APersistableBundle_size,
    APersistableBundle_writeToParcel, APERSISTABLEBUNDLE_KEY_NOT_FOUND,
};
use std::ffi::{c_char, CString, NulError};
use std::ptr::{null_mut, NonNull};

/// A mapping from string keys to values of various types.
#[derive(Debug)]
pub struct PersistableBundle(NonNull<APersistableBundle>);

impl PersistableBundle {
    /// Creates a new `PersistableBundle`.
    pub fn new() -> Self {
        // SAFETY: APersistableBundle_new doesn't actually have any safety requirements.
        let bundle = unsafe { APersistableBundle_new() };
        Self(NonNull::new(bundle).expect("Allocated APersistableBundle was null"))
    }

    /// Returns the number of mappings in the bundle.
    pub fn size(&self) -> usize {
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`.
        unsafe { APersistableBundle_size(self.0.as_ptr()) }
            .try_into()
            .expect("APersistableBundle_size returned a negative size")
    }

    /// Removes any entry with the given key.
    ///
    /// Returns an error if the given key contains a NUL character, otherwise returns whether there
    /// was any entry to remove.
    pub fn remove(&mut self, key: &str) -> Result<bool, NulError> {
        let key = CString::new(key)?;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call.
        Ok(unsafe { APersistableBundle_erase(self.0.as_ptr(), key.as_ptr()) != 0 })
    }

    /// Inserts a key-value pair into the bundle.
    ///
    /// If the key is already present then its value will be overwritten by the given value.
    ///
    /// Returns an error if the key contains a NUL character.
    pub fn insert_bool(&mut self, key: &str, value: bool) -> Result<(), NulError> {
        let key = CString::new(key)?;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call.
        unsafe {
            APersistableBundle_putBoolean(self.0.as_ptr(), key.as_ptr(), value);
        }
        Ok(())
    }

    /// Inserts a key-value pair into the bundle.
    ///
    /// If the key is already present then its value will be overwritten by the given value.
    ///
    /// Returns an error if the key contains a NUL character.
    pub fn insert_int(&mut self, key: &str, value: i32) -> Result<(), NulError> {
        let key = CString::new(key)?;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call.
        unsafe {
            APersistableBundle_putInt(self.0.as_ptr(), key.as_ptr(), value);
        }
        Ok(())
    }

    /// Inserts a key-value pair into the bundle.
    ///
    /// If the key is already present then its value will be overwritten by the given value.
    ///
    /// Returns an error if the key contains a NUL character.
    pub fn insert_long(&mut self, key: &str, value: i64) -> Result<(), NulError> {
        let key = CString::new(key)?;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call.
        unsafe {
            APersistableBundle_putLong(self.0.as_ptr(), key.as_ptr(), value);
        }
        Ok(())
    }

    /// Inserts a key-value pair into the bundle.
    ///
    /// If the key is already present then its value will be overwritten by the given value.
    ///
    /// Returns an error if the key contains a NUL character.
    pub fn insert_double(&mut self, key: &str, value: f64) -> Result<(), NulError> {
        let key = CString::new(key)?;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call.
        unsafe {
            APersistableBundle_putDouble(self.0.as_ptr(), key.as_ptr(), value);
        }
        Ok(())
    }

    /// Inserts a key-value pair into the bundle.
    ///
    /// If the key is already present then its value will be overwritten by the given value.
    ///
    /// Returns an error if the key or value contains a NUL character.
    pub fn insert_string(&mut self, key: &str, value: &str) -> Result<(), NulError> {
        let key = CString::new(key)?;
        let value = CString::new(value)?;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `CStr::as_ptr` is guaranteed
        // to be valid for the duration of this call.
        unsafe {
            APersistableBundle_putString(self.0.as_ptr(), key.as_ptr(), value.as_ptr());
        }
        Ok(())
    }

    /// Inserts a key-value pair into the bundle.
    ///
    /// If the key is already present then its value will be overwritten by the given value.
    ///
    /// Returns an error if the key contains a NUL character.
    pub fn insert_bool_vec(&mut self, key: &str, value: &[bool]) -> Result<(), NulError> {
        let key = CString::new(key)?;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call, and likewise the pointer returned by
        // `value.as_ptr()` is guaranteed to be valid for at least `value.len()` values for the
        // duration of the call.
        unsafe {
            APersistableBundle_putBooleanVector(
                self.0.as_ptr(),
                key.as_ptr(),
                value.as_ptr(),
                value.len().try_into().unwrap(),
            );
        }
        Ok(())
    }

    /// Inserts a key-value pair into the bundle.
    ///
    /// If the key is already present then its value will be overwritten by the given value.
    ///
    /// Returns an error if the key contains a NUL character.
    pub fn insert_int_vec(&mut self, key: &str, value: &[i32]) -> Result<(), NulError> {
        let key = CString::new(key)?;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call, and likewise the pointer returned by
        // `value.as_ptr()` is guaranteed to be valid for at least `value.len()` values for the
        // duration of the call.
        unsafe {
            APersistableBundle_putIntVector(
                self.0.as_ptr(),
                key.as_ptr(),
                value.as_ptr(),
                value.len().try_into().unwrap(),
            );
        }
        Ok(())
    }

    /// Inserts a key-value pair into the bundle.
    ///
    /// If the key is already present then its value will be overwritten by the given value.
    ///
    /// Returns an error if the key contains a NUL character.
    pub fn insert_long_vec(&mut self, key: &str, value: &[i64]) -> Result<(), NulError> {
        let key = CString::new(key)?;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call, and likewise the pointer returned by
        // `value.as_ptr()` is guaranteed to be valid for at least `value.len()` values for the
        // duration of the call.
        unsafe {
            APersistableBundle_putLongVector(
                self.0.as_ptr(),
                key.as_ptr(),
                value.as_ptr(),
                value.len().try_into().unwrap(),
            );
        }
        Ok(())
    }

    /// Inserts a key-value pair into the bundle.
    ///
    /// If the key is already present then its value will be overwritten by the given value.
    ///
    /// Returns an error if the key contains a NUL character.
    pub fn insert_double_vec(&mut self, key: &str, value: &[f64]) -> Result<(), NulError> {
        let key = CString::new(key)?;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call, and likewise the pointer returned by
        // `value.as_ptr()` is guaranteed to be valid for at least `value.len()` values for the
        // duration of the call.
        unsafe {
            APersistableBundle_putDoubleVector(
                self.0.as_ptr(),
                key.as_ptr(),
                value.as_ptr(),
                value.len().try_into().unwrap(),
            );
        }
        Ok(())
    }

    /// Inserts a key-value pair into the bundle.
    ///
    /// If the key is already present then its value will be overwritten by the given value.
    ///
    /// Returns an error if the key contains a NUL character.
    pub fn insert_string_vec<'a, T: ToString + 'a>(
        &mut self,
        key: &str,
        value: impl IntoIterator<Item = &'a T>,
    ) -> Result<(), NulError> {
        let key = CString::new(key)?;
        // We need to collect the new `CString`s into something first so that they live long enough
        // for their pointers to be valid for the `APersistableBundle_putStringVector` call below.
        let c_strings = value
            .into_iter()
            .map(|s| CString::new(s.to_string()))
            .collect::<Result<Vec<_>, NulError>>()?;
        let char_pointers = c_strings.iter().map(|s| s.as_ptr()).collect::<Vec<_>>();
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call, and likewise the pointer returned by
        // `value.as_ptr()` is guaranteed to be valid for at least `value.len()` values for the
        // duration of the call.
        unsafe {
            APersistableBundle_putStringVector(
                self.0.as_ptr(),
                key.as_ptr(),
                char_pointers.as_ptr(),
                char_pointers.len().try_into().unwrap(),
            );
        }
        Ok(())
    }

    /// Inserts a key-value pair into the bundle.
    ///
    /// If the key is already present then its value will be overwritten by the given value.
    ///
    /// Returns an error if the key contains a NUL character.
    pub fn insert_persistable_bundle(
        &mut self,
        key: &str,
        value: &PersistableBundle,
    ) -> Result<(), NulError> {
        let key = CString::new(key)?;
        // SAFETY: The wrapped `APersistableBundle` pointers are guaranteed to be valid for the
        // lifetime of the `PersistableBundle`s. The pointer returned by `CStr::as_ptr` is
        // guaranteed to be valid for the duration of this call, and
        // `APersistableBundle_putPersistableBundle` does a deep copy so that is all that is
        // required.
        unsafe {
            APersistableBundle_putPersistableBundle(
                self.0.as_ptr(),
                key.as_ptr(),
                value.0.as_ptr(),
            );
        }
        Ok(())
    }

    /// Gets the boolean value associated with the given key.
    ///
    /// Returns an error if the key contains a NUL character, or `Ok(None)` if the key doesn't exist
    /// in the bundle.
    pub fn get_bool(&self, key: &str) -> Result<Option<bool>, NulError> {
        let key = CString::new(key)?;
        let mut value = false;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call. The value pointer must be valid because it
        // comes from a reference.
        if unsafe { APersistableBundle_getBoolean(self.0.as_ptr(), key.as_ptr(), &mut value) } {
            Ok(Some(value))
        } else {
            Ok(None)
        }
    }

    /// Gets the i32 value associated with the given key.
    ///
    /// Returns an error if the key contains a NUL character, or `Ok(None)` if the key doesn't exist
    /// in the bundle.
    pub fn get_int(&self, key: &str) -> Result<Option<i32>, NulError> {
        let key = CString::new(key)?;
        let mut value = 0;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call. The value pointer must be valid because it
        // comes from a reference.
        if unsafe { APersistableBundle_getInt(self.0.as_ptr(), key.as_ptr(), &mut value) } {
            Ok(Some(value))
        } else {
            Ok(None)
        }
    }

    /// Gets the i64 value associated with the given key.
    ///
    /// Returns an error if the key contains a NUL character, or `Ok(None)` if the key doesn't exist
    /// in the bundle.
    pub fn get_long(&self, key: &str) -> Result<Option<i64>, NulError> {
        let key = CString::new(key)?;
        let mut value = 0;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call. The value pointer must be valid because it
        // comes from a reference.
        if unsafe { APersistableBundle_getLong(self.0.as_ptr(), key.as_ptr(), &mut value) } {
            Ok(Some(value))
        } else {
            Ok(None)
        }
    }

    /// Gets the f64 value associated with the given key.
    ///
    /// Returns an error if the key contains a NUL character, or `Ok(None)` if the key doesn't exist
    /// in the bundle.
    pub fn get_double(&self, key: &str) -> Result<Option<f64>, NulError> {
        let key = CString::new(key)?;
        let mut value = 0.0;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the duration of this call. The value pointer must be valid because it
        // comes from a reference.
        if unsafe { APersistableBundle_getDouble(self.0.as_ptr(), key.as_ptr(), &mut value) } {
            Ok(Some(value))
        } else {
            Ok(None)
        }
    }

    /// Gets the vector of `T` associated with the given key.
    ///
    /// Returns an error if the key contains a NUL character, or `Ok(None)` if the key doesn't exist
    /// in the bundle.
    ///
    /// `get_func` should be one of the `APersistableBundle_get*Vector` functions from
    /// `binder_ndk_sys`.
    ///
    /// # Safety
    ///
    /// `get_func` must only require that the pointers it takes are valid for the duration of the
    /// call. It must allow a null pointer for the buffer, and must return the size in bytes of
    /// buffer it requires. If it is given a non-null buffer pointer it must write that number of
    /// bytes to the buffer, which must be a whole number of valid `T` values.
    unsafe fn get_vec<T: Clone + Default>(
        &self,
        key: &str,
        get_func: unsafe extern "C" fn(
            *const APersistableBundle,
            *const c_char,
            *mut T,
            i32,
        ) -> i32,
    ) -> Result<Option<Vec<T>>, NulError> {
        let key = CString::new(key)?;
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the lifetime of `key`. A null pointer is allowed for the buffer.
        match unsafe { get_func(self.0.as_ptr(), key.as_ptr(), null_mut(), 0) } {
            APERSISTABLEBUNDLE_KEY_NOT_FOUND => Ok(None),
            required_buffer_size => {
                let mut value = vec![
                    T::default();
                    usize::try_from(required_buffer_size).expect(
                        "APersistableBundle_get*Vector returned invalid size"
                    ) / size_of::<T>()
                ];
                // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for
                // the lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()`
                // is guaranteed to be valid for the lifetime of `key`. The value buffer pointer is
                // valid as it comes from the Vec we just allocated.
                match unsafe {
                    get_func(
                        self.0.as_ptr(),
                        key.as_ptr(),
                        value.as_mut_ptr(),
                        (value.len() * size_of::<T>()).try_into().unwrap(),
                    )
                } {
                    APERSISTABLEBUNDLE_KEY_NOT_FOUND => {
                        panic!("APersistableBundle_get*Vector failed to find key after first finding it");
                    }
                    _ => Ok(Some(value)),
                }
            }
        }
    }

    /// Gets the boolean vector value associated with the given key.
    ///
    /// Returns an error if the key contains a NUL character, or `Ok(None)` if the key doesn't exist
    /// in the bundle.
    pub fn get_bool_vec(&self, key: &str) -> Result<Option<Vec<bool>>, NulError> {
        // SAFETY: APersistableBundle_getBooleanVector fulfils all the safety requirements of
        // `get_vec`.
        unsafe { self.get_vec(key, APersistableBundle_getBooleanVector) }
    }

    /// Gets the i32 vector value associated with the given key.
    ///
    /// Returns an error if the key contains a NUL character, or `Ok(None)` if the key doesn't exist
    /// in the bundle.
    pub fn get_int_vec(&self, key: &str) -> Result<Option<Vec<i32>>, NulError> {
        // SAFETY: APersistableBundle_getIntVector fulfils all the safety requirements of
        // `get_vec`.
        unsafe { self.get_vec(key, APersistableBundle_getIntVector) }
    }

    /// Gets the i64 vector value associated with the given key.
    ///
    /// Returns an error if the key contains a NUL character, or `Ok(None)` if the key doesn't exist
    /// in the bundle.
    pub fn get_long_vec(&self, key: &str) -> Result<Option<Vec<i64>>, NulError> {
        // SAFETY: APersistableBundle_getLongVector fulfils all the safety requirements of
        // `get_vec`.
        unsafe { self.get_vec(key, APersistableBundle_getLongVector) }
    }

    /// Gets the f64 vector value associated with the given key.
    ///
    /// Returns an error if the key contains a NUL character, or `Ok(None)` if the key doesn't exist
    /// in the bundle.
    pub fn get_double_vec(&self, key: &str) -> Result<Option<Vec<f64>>, NulError> {
        // SAFETY: APersistableBundle_getDoubleVector fulfils all the safety requirements of
        // `get_vec`.
        unsafe { self.get_vec(key, APersistableBundle_getDoubleVector) }
    }

    /// Gets the `PersistableBundle` value associated with the given key.
    ///
    /// Returns an error if the key contains a NUL character, or `Ok(None)` if the key doesn't exist
    /// in the bundle.
    pub fn get_persistable_bundle(&self, key: &str) -> Result<Option<Self>, NulError> {
        let key = CString::new(key)?;
        let mut value = null_mut();
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. The pointer returned by `key.as_ptr()` is guaranteed
        // to be valid for the lifetime of `key`. The value pointer must be valid because it comes
        // from a reference.
        if unsafe {
            APersistableBundle_getPersistableBundle(self.0.as_ptr(), key.as_ptr(), &mut value)
        } {
            Ok(Some(Self(NonNull::new(value).expect(
                "APersistableBundle_getPersistableBundle returned true but didn't set outBundle",
            ))))
        } else {
            Ok(None)
        }
    }
}

// SAFETY: The underlying *APersistableBundle can be moved between threads.
unsafe impl Send for PersistableBundle {}

// SAFETY: The underlying *APersistableBundle can be read from multiple threads, and we require
// `&mut PersistableBundle` for any operations which mutate it.
unsafe impl Sync for PersistableBundle {}

impl Default for PersistableBundle {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for PersistableBundle {
    fn drop(&mut self) {
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of this `PersistableBundle`.
        unsafe { APersistableBundle_delete(self.0.as_ptr()) };
    }
}

impl Clone for PersistableBundle {
    fn clone(&self) -> Self {
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`.
        let duplicate = unsafe { APersistableBundle_dup(self.0.as_ptr()) };
        Self(NonNull::new(duplicate).expect("Duplicated APersistableBundle was null"))
    }
}

impl PartialEq for PersistableBundle {
    fn eq(&self, other: &Self) -> bool {
        // SAFETY: The wrapped `APersistableBundle` pointers are guaranteed to be valid for the
        // lifetime of the `PersistableBundle`s.
        unsafe { APersistableBundle_isEqual(self.0.as_ptr(), other.0.as_ptr()) }
    }
}

impl UnstructuredParcelable for PersistableBundle {
    fn write_to_parcel(&self, parcel: &mut BorrowedParcel) -> Result<(), StatusCode> {
        let status =
        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. `parcel.as_native_mut()` always returns a valid
        // parcel pointer.
            unsafe { APersistableBundle_writeToParcel(self.0.as_ptr(), parcel.as_native_mut()) };
        status_result(status)
    }

    fn from_parcel(parcel: &BorrowedParcel) -> Result<Self, StatusCode> {
        let mut bundle = null_mut();

        // SAFETY: The wrapped `APersistableBundle` pointer is guaranteed to be valid for the
        // lifetime of the `PersistableBundle`. `parcel.as_native()` always returns a valid parcel
        // pointer.
        let status = unsafe { APersistableBundle_readFromParcel(parcel.as_native(), &mut bundle) };
        status_result(status)?;

        Ok(Self(NonNull::new(bundle).expect(
            "APersistableBundle_readFromParcel returned success but didn't allocate bundle",
        )))
    }
}

impl_deserialize_for_unstructured_parcelable!(PersistableBundle);
impl_serialize_for_unstructured_parcelable!(PersistableBundle);

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn create_delete() {
        let bundle = PersistableBundle::new();
        drop(bundle);
    }

    #[test]
    fn duplicate_equal() {
        let bundle = PersistableBundle::new();
        let duplicate = bundle.clone();
        assert_eq!(bundle, duplicate);
    }

    #[test]
    fn get_empty() {
        let bundle = PersistableBundle::new();
        assert_eq!(bundle.get_bool("foo"), Ok(None));
        assert_eq!(bundle.get_int("foo"), Ok(None));
        assert_eq!(bundle.get_long("foo"), Ok(None));
        assert_eq!(bundle.get_double("foo"), Ok(None));
        assert_eq!(bundle.get_bool_vec("foo"), Ok(None));
        assert_eq!(bundle.get_int_vec("foo"), Ok(None));
        assert_eq!(bundle.get_long_vec("foo"), Ok(None));
        assert_eq!(bundle.get_double_vec("foo"), Ok(None));
    }

    #[test]
    fn remove_empty() {
        let mut bundle = PersistableBundle::new();
        assert_eq!(bundle.remove("foo"), Ok(false));
    }

    #[test]
    fn insert_get_primitives() {
        let mut bundle = PersistableBundle::new();

        assert_eq!(bundle.insert_bool("bool", true), Ok(()));
        assert_eq!(bundle.insert_int("int", 42), Ok(()));
        assert_eq!(bundle.insert_long("long", 66), Ok(()));
        assert_eq!(bundle.insert_double("double", 123.4), Ok(()));

        assert_eq!(bundle.get_bool("bool"), Ok(Some(true)));
        assert_eq!(bundle.get_int("int"), Ok(Some(42)));
        assert_eq!(bundle.get_long("long"), Ok(Some(66)));
        assert_eq!(bundle.get_double("double"), Ok(Some(123.4)));
        assert_eq!(bundle.size(), 4);

        // Getting the wrong type should return nothing.
        assert_eq!(bundle.get_int("bool"), Ok(None));
        assert_eq!(bundle.get_long("bool"), Ok(None));
        assert_eq!(bundle.get_double("bool"), Ok(None));
        assert_eq!(bundle.get_bool("int"), Ok(None));
        assert_eq!(bundle.get_long("int"), Ok(None));
        assert_eq!(bundle.get_double("int"), Ok(None));
        assert_eq!(bundle.get_bool("long"), Ok(None));
        assert_eq!(bundle.get_int("long"), Ok(None));
        assert_eq!(bundle.get_double("long"), Ok(None));
        assert_eq!(bundle.get_bool("double"), Ok(None));
        assert_eq!(bundle.get_int("double"), Ok(None));
        assert_eq!(bundle.get_long("double"), Ok(None));

        // If they are removed they should no longer be present.
        assert_eq!(bundle.remove("bool"), Ok(true));
        assert_eq!(bundle.remove("int"), Ok(true));
        assert_eq!(bundle.remove("long"), Ok(true));
        assert_eq!(bundle.remove("double"), Ok(true));
        assert_eq!(bundle.get_bool("bool"), Ok(None));
        assert_eq!(bundle.get_int("int"), Ok(None));
        assert_eq!(bundle.get_long("long"), Ok(None));
        assert_eq!(bundle.get_double("double"), Ok(None));
        assert_eq!(bundle.size(), 0);
    }

    #[test]
    fn insert_string() {
        let mut bundle = PersistableBundle::new();
        assert_eq!(bundle.insert_string("string", "foo"), Ok(()));
        assert_eq!(bundle.size(), 1);
    }

    #[test]
    fn insert_get_vec() {
        let mut bundle = PersistableBundle::new();

        assert_eq!(bundle.insert_bool_vec("bool", &[]), Ok(()));
        assert_eq!(bundle.insert_int_vec("int", &[42]), Ok(()));
        assert_eq!(bundle.insert_long_vec("long", &[66, 67, 68]), Ok(()));
        assert_eq!(bundle.insert_double_vec("double", &[123.4]), Ok(()));
        assert_eq!(bundle.insert_string_vec("string", &["foo", "bar", "baz"]), Ok(()));
        assert_eq!(
            bundle.insert_string_vec(
                "string",
                &[&"foo".to_string(), &"bar".to_string(), &"baz".to_string()]
            ),
            Ok(())
        );
        assert_eq!(
            bundle.insert_string_vec(
                "string",
                &["foo".to_string(), "bar".to_string(), "baz".to_string()]
            ),
            Ok(())
        );

        assert_eq!(bundle.size(), 5);

        assert_eq!(bundle.get_bool_vec("bool"), Ok(Some(vec![])));
        assert_eq!(bundle.get_int_vec("int"), Ok(Some(vec![42])));
        assert_eq!(bundle.get_long_vec("long"), Ok(Some(vec![66, 67, 68])));
        assert_eq!(bundle.get_double_vec("double"), Ok(Some(vec![123.4])));
    }

    #[test]
    fn insert_get_bundle() {
        let mut bundle = PersistableBundle::new();

        let mut sub_bundle = PersistableBundle::new();
        assert_eq!(sub_bundle.insert_int("int", 42), Ok(()));
        assert_eq!(sub_bundle.size(), 1);
        assert_eq!(bundle.insert_persistable_bundle("bundle", &sub_bundle), Ok(()));

        assert_eq!(bundle.get_persistable_bundle("bundle"), Ok(Some(sub_bundle)));
    }
}
