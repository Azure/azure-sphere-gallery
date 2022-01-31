/// \file storage.h
/// \brief This header contains functionality for interacting with on-device storage.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     <para>Returns a null-terminated string which is the absolute path to a location
///     within the image package of the running application, given a relative path
///     inside the image package.</para>
///
///     <para>The location of the image package - and the path returned by this function
///     - will not change while an application is running. However, the location may
///     change between executions of an application.</para>
///
///     <para>The memory for the returned string will be allocated by this function, and
///     should be freed by the caller using free().</para>
///
///     <para>This function does not check the path exists in the image package. The path
///     may not begin with '/' or '.', and may not contain '..'.</para>
///
///     <para> **Errors** </para>
///     <para>In case of errors, this function will return NULL and set errno to the error
///     value.</para>
///     <para>EINVAL: <paramref name="relativePath" /> begins with '/' or '.', or
///     contains '..'.</para>
///     <para>EFAULT: <paramref name="relativePath" /> is NULL. </para>
///     <para>ENOMEM: Out of memory.</para>
///     <para>Any other errno may also be specified; such errors are not deterministic and
///     no guarantee is made that the same behavior will be retained through system updates.</para>
/// </summary>
/// <param name="relativePath">
///     A relative path from the root of the image package. Must not start with the
///     directory separator character '/'.
/// </param>
/// <returns>
///     The absolute path including the image package root, or NULL on error,
///     in which case errno is set to the error value.  When a non-NULL pointer
///     is returned, the memory for the returned string will be allocated by
///     this function; callers are responsible for freeing it using free()
///     when they have finished using it.
/// </returns>
char *Storage_GetAbsolutePathInImagePackage(const char *relativePath);

/// <summary>
///     <para>Takes a relative path inside the image package and returns an
///     opened read-only file descriptor.</para>
///
///     <para>The caller should close the returned file descriptor with the
///     <see cref="close" /> function.</para>
///
///     <para>This function should only be used to open regular files inside
///     the image package.</para>
///
///     <para> **Errors** </para>
///     <para>In case of error, this function returns -1, with more information
///     in errno.  See <see cref="Storage_GetAbsolutePathInImagePackage" /> for error
///     codes related to filenames.</para>
///     <para>EINVAL: <paramref name="relativePath" /> begins with '/' or '.', or
///     contains '..'.</para>
///     <para>EFAULT: <paramref name="relativePath" /> is NULL. </para>
///     <para>ENOMEM: Out of memory.</para>
///     <para>Any other errno may also be specified; such errors are not deterministic and
///     no guarantee is made that the same behavior will be retained through system updates.</para>
/// </summary>
/// <param name="relativePath">
///     A relative path from the root of the image package. See
///     <see cref="Storage_GetAbsolutePathInImagePackage" /> for restrictions
///     on the filename.
/// </param>
/// <returns>
///     An opened file descriptor, or -1 on error, in which case errno is set to the error value.
/// </returns>
/// <seealso cref="Storage_GetAbsolutePathInImagePackage" />
int Storage_OpenFileInImagePackage(const char *relativePath);

/// <summary>
///     <para>Returns a new descriptor to a file in nonvolatile storage where data will be
///     persisted over device reboot. This file will be retained over reboot as well
///     as over application update.</para>
///
///     <para>The file will be created if it does not exist. Otherwise, this function will return a
///     new file descriptor to an existing file.  The file can be deleted later with
///     <see cref="Storage_DeleteMutableFile"/></para>
///
///     <para> **Errors** </para>
///     <para>In case of errors, this function will return -1 and set errno to the error value.
///     </para>
///     <para>EACCES: The application does not have the application capability
///     (MutableStorage) required in order to use this API, or does not have a non-zero size
///     specified for the capability (SizeKB).</para>
///     <para>EIO: An error occurred while trying to create the file.</para>
///     <para>Any other errno may also be specified; such errors are not deterministic and
///     no guarantee is made that the same behavior will be retained through system updates.</para>
/// </summary>
/// <returns>
///     An opened file descriptor to persistent, mutable storage, or -1 on failure, in which case
///     errno is set to the error value.
/// </returns>
int Storage_OpenMutableFile(void);

/// <summary>
///     <para>Deletes any existing file previously obtained through <see
///     cref="Storage_OpenMutableFile"/>.  All descriptors on the file must have been closed,
///     otherwise the behavior of this function is undefined.</para>
///
///     <para>WARNING: this action is permanent!</para>
///
///     <para> **Errors** </para>
///     <para>In case of errors, this function will return -1 and set errno to the error value.
///     </para>
///     <para>EACCES: The application does not have the application capability (MutableStorage)
///     required in order to use this API.</para>
///     <para>EIO: An error occurred while trying to delete the file.</para>
///     <para>ENOENT:There was no existing mutable storage file to delete.</para>
///     <para>Any other errno may also be specified; such errors are not deterministic and
///     no guarantee is made that the same behavior will be retained through system updates.</para>
/// </summary>
/// <returns>0 on success, or -1 on failure, in which case errno is set to the error
/// value.</returns>
int Storage_DeleteMutableFile(void);

#ifdef __cplusplus
}
#endif
