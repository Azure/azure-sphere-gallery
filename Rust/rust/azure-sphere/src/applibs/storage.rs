//! The Applibs storage module contains functions that interact with on-device storage, which includes read-only storage and mutable storage.

use azure_sphere_sys::applibs::storage;
use std::ffi::{CStr, CString, OsString};
use std::fs::File;
use std::io::Error;
use std::os::unix::ffi::OsStrExt;
use std::os::unix::io::FromRawFd;
use std::path::Path;

/// Delete the mutable file
///
/// Returns Ok with whether the file previously existed or not or an Err.
///
/// # Safety
/// All the file handles must be closed (dropped) before this is safe to call
pub unsafe fn delete() -> Result<bool, std::io::Error> {
    let ret = storage::Storage_DeleteMutableFile();
    if ret == -1 {
        let e = Error::last_os_error();
        if let Some(raw_os_err) = e.raw_os_error() {
            if raw_os_err == libc::ENOENT {
                return Ok(false);
            }
        }
        return Err(e);
    }
    return Ok(true);
}

/// Gets a string that contains the absolute path to a location within the image package of the running application, given a relative path inside the image package.
///
/// The location of the image package and the path returned by this function will not change while an application is running. However, the location may change between executions of an application.
///
///This function does not check whether the path exists in the image package. The path cannot not begin with '/' or '.', and cannot not contain '..'.
pub fn absolute_path_in_image_package<P: Into<CString>>(
    relative_path: P,
) -> Result<OsString, std::io::Error> {
    // TODO: the incoming string may not contain '..' or begin with '/' or '.'
    let relative_path = relative_path.into();
    let ptr = unsafe {
        storage::Storage_GetAbsolutePathInImagePackage(relative_path.as_ptr() as *const libc::c_char)
    };
    if ptr as isize == -1 {
        return Err(Error::last_os_error());
    }
    debug_assert!(!ptr.is_null());
    let c = unsafe { CStr::from_ptr(ptr).to_string_lossy().into_owned() };
    Ok(OsString::from(c))
}

/// Retrieves a file descriptor to the mutable storage file for the application. If the application doesn't already have a mutable storage file, this function creates a file and then returns the [`std::fs::File`]
///
/// A mutable storage file stores and persists data over a device reboot or system update.
/// The application manifest must include the MutableStorage capability.
pub fn open_mutable_file() -> Result<std::fs::File, std::io::Error> {
    let fd = unsafe { storage::Storage_OpenMutableFile() };
    if fd == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(unsafe { File::from_raw_fd(fd) })
    }
}

/// Takes a relative path inside the image package and returns an opened read-only file. This function should only be used to open regular files inside the image package.
pub fn open_in_image_package<P: AsRef<Path>>(
    relative_path: P,
) -> Result<std::fs::File, std::io::Error> {
    let t = relative_path.as_ref();
    let t = t.as_os_str();
    let t = std::ffi::CString::new(t.as_bytes()).unwrap();
    let t = t.as_ptr();
    let fd = unsafe { storage::Storage_OpenFileInImagePackage(t as *const libc::c_char) };
    if fd == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(unsafe { File::from_raw_fd(fd) })
    }
}
