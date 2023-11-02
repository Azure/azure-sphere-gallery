//! The Certstore module contains functions and types that install and manage certificates on a device.
//!
//! An app can call these functions only if the CertStore capability is enabled in the application manifest.
//!
//! A valid identifier must be a unique string from one to [`MAX_IDENTIFIER_LENGTH`] characters in length. The following characters are valid in an identifer:
//! - 'A' to 'Z'
//! - 'a' to 'z'
//! - '0' to '9'
//! - '.' or '-' or '_'
//!
//! *** Caution ***
//! Because certificate IDs are system-wide, an azsphere command or a function call that adds a new certificate can overwrite a certificate that was added by an earlier command or function call, potentially causing network connection failures. We strongly recommend that you develop clear certificate update procedures and choose certificate IDs carefully.

use super::wificonfig;
use azure_sphere_sys::applibs::{certstore, static_inline_helpers};
use chrono::{DateTime, NaiveDateTime, Utc};
use std::ffi::{OsStr, OsString};
use std::io::Error;
use std::os::unix::ffi::OsStrExt;

/// Maximum length of a certificate identifier
pub const MAX_IDENTIFIER_LENGTH: u32 = static_inline_helpers::CERTSTORE_MAX_IDENTIFIER_LENGTH;

/// Maximum length of a certificate in bytes
pub const MAX_CERT_SIZE: u32 = static_inline_helpers::CERTSTORE_MAX_CERT_SIZE;

/// Maximum length of a private key password
pub const MAX_PRIVATE_KEY_PASSWORD_LENGTH: u32 =
    static_inline_helpers::CERTSTORE_MAX_PRIVATE_KEY_PASSWORD_LENGTH;

fn convert_tm_to_datetime(tm: &mut static_inline_helpers::tm) -> DateTime<Utc> {
    let timestamp = unsafe { static_inline_helpers::mktime(tm) };
    let naive =
        NaiveDateTime::from_timestamp_opt(timestamp / 1000, (timestamp % 1000) as u32 * 1_000_000)
            .unwrap();
    DateTime::from_naive_utc_and_offset(naive, Utc)
}

/// Installs a client certificate that consists of a public certificate and a private key with the specified ID. The ID can then be used to refer to the certificate in other functions. If any type of certificate is already installed with the same ID, it will be replaced with the new certificate.
pub fn install_client_certificate<P: AsRef<OsStr>>(
    identifier: P,
    cert_blob: &[u8],
    private_key_blob: &[u8],
    private_key_password: &[u8],
) -> Result<(), std::io::Error> {
    let identifier = identifier.as_ref();
    let identifier = std::ffi::CString::new(identifier.as_bytes()).unwrap();
    let identifier = identifier.as_ptr();

    // Null-terminate the private key password
    let private_key_password = std::ffi::CString::new(private_key_password).unwrap();

    let result = unsafe {
        certstore::CertStore_InstallClientCertificate(
            identifier,
            cert_blob.as_ptr(),
            cert_blob.len(),
            private_key_blob.as_ptr(),
            private_key_blob.len(),
            private_key_password.as_ptr(),
        )
    };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Installs a Root CA certificate that consists of a public certificate in PEM format and assigns an ID to the certificate. The ID can then be used to refer to the certificate in other functions. If any type of certificate is already installed with the same ID, it will be replaced with the new certificate.
pub fn install_root_ca_certificate<P: AsRef<OsStr>>(
    identifier: P,
    cert_blob: &[u8],
) -> Result<(), std::io::Error> {
    let identifier = identifier.as_ref();
    let identifier = std::ffi::CString::new(identifier.as_bytes()).unwrap();
    let identifier = identifier.as_ptr();

    let result = unsafe {
        certstore::CertStore_InstallRootCACertificate(
            identifier,
            cert_blob.as_ptr(),
            cert_blob.len(),
        )
    };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Gets the number of certificates installed on the device.
pub fn get_certificate_count() -> Result<usize, std::io::Error> {
    let result = unsafe { certstore::CertStore_GetCertificateCount() };
    if result < 0 {
        Err(Error::last_os_error())
    } else {
        Ok(result as usize)
    }
}

fn get_certificate_identifer_at(index: usize) -> Result<OsString, std::io::Error> {
    unsafe {
        let mut identifier = certstore::CertStore_Identifier {
            identifier: [0u8; 17],
        };
        let result = certstore::CertStore_GetCertificateIdentifierAt(index, &mut identifier);
        if result == -1 {
            Err(Error::last_os_error())
        } else {
            let result = OsStr::from_bytes(&identifier.identifier);
            Ok(result.to_os_string())
        }
    }
}

/// Gets the ID of the certificate at the specified index.
pub fn get_certificate_at(index: usize) -> Result<Certificate, std::io::Error> {
    let identifier = get_certificate_identifer_at(index)?;
    Ok(Certificate { identifier })
}

fn delete_certificate(identifier: &OsStr) -> Result<(), std::io::Error> {
    let result = unsafe { certstore::CertStore_DeleteCertificate(identifier.as_bytes().as_ptr()) };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Identifies a certificate by name
#[derive(Debug, Clone)]
pub struct Certificate {
    identifier: OsString,
}

impl Certificate {
    /// Create a new certificate identifier
    pub fn new<P: AsRef<OsStr>>(identifier: P) -> Self {
        let identifier = identifier.as_ref();
        Certificate {
            identifier: OsString::from(identifier),
        }
    }

    /// Get the certificate's identifier
    pub fn identifer(&self) -> Result<String, std::io::Error> {
        let result = self.identifier.clone().into_string().unwrap();
        Ok(result)
    }

    /// Get the certificate's issuer name
    pub fn issuer_name(&self) -> Result<Vec<u8>, std::io::Error> {
        get_certificate_issuer_name(&self.identifier)
    }

    /// Get the certificate's subject name
    pub fn subject_name(&self) -> Result<Vec<u8>, std::io::Error> {
        get_certificate_subject_name(&self.identifier)
    }

    /// Get the certificate's not-before time
    pub fn not_before(&self) -> Result<DateTime<Utc>, std::io::Error> {
        get_certificate_not_before(&self.identifier)
    }

    // Get the certificate's not-after time
    pub fn not_after(&self) -> Result<DateTime<Utc>, std::io::Error> {
        get_certificate_not_after(&self.identifier)
    }

    /// Delete the certificate
    pub fn delete(&self) -> Result<(), std::io::Error> {
        delete_certificate(&self.identifier)
    }
}

/// Iterate through the device's certificate store
#[derive(Clone, Debug)]
pub struct Certificates {
    curr: usize,
}

impl Iterator for Certificates {
    type Item = Certificate;

    fn next(&mut self) -> Option<Self::Item> {
        let current = self.curr;
        let identifier = get_certificate_identifer_at(current);
        match identifier {
            Err(_) => None,
            Ok(id) => {
                self.curr = self.curr + 1;
                Some(Certificate { identifier: id })
            }
        }
    }
}

/// Get an interator through the certificate store
pub fn certificates() -> Certificates {
    Certificates { curr: 0 }
}

/// Gets the remaining space that is available on the device for certificate storage, in bytes.
pub fn get_certificate_available_space() -> Result<usize, std::io::Error> {
    let result = unsafe { certstore::CertStore_GetAvailableSpace() };
    if result < 0 {
        Err(Error::last_os_error())
    } else {
        Ok(result as usize)
    }
}

fn get_certificate_subject_name(identifier: &OsString) -> Result<Vec<u8>, std::io::Error> {
    let identifier = std::ffi::CString::new(identifier.as_bytes()).unwrap();
    let identifier = identifier.as_ptr();
    unsafe {
        let mut subject_name = static_inline_helpers::CertStore_SubjectName { name: [0u8; 301] };
        let result = static_inline_helpers::CertStore_GetCertificateSubjectName_inline(
            identifier,
            &mut subject_name,
        );
        if result == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(wificonfig::vec_from_null_terminated_or_max(
                &subject_name.name,
            ))
        }
    }
}

fn get_certificate_issuer_name(identifier: &OsString) -> Result<Vec<u8>, std::io::Error> {
    let identifier = std::ffi::CString::new(identifier.as_bytes()).unwrap();
    let identifier = identifier.as_ptr();
    unsafe {
        let mut issuer_name = static_inline_helpers::CertStore_IssuerName { name: [0u8; 301] };
        let result = static_inline_helpers::CertStore_GetCertificateIssuerName_inline(
            identifier,
            &mut issuer_name,
        );
        if result == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(wificonfig::vec_from_null_terminated_or_max(
                &issuer_name.name,
            ))
        }
    }
}

fn get_certificate_not_before(identifier: &OsString) -> Result<DateTime<Utc>, std::io::Error> {
    let identifier = std::ffi::CString::new(identifier.as_bytes()).unwrap();
    let identifier = identifier.as_ptr();
    unsafe {
        let mut not_before = static_inline_helpers::tm {
            tm_gmtoff: 0,
            tm_hour: 0,
            tm_isdst: 0,
            tm_mday: 0,
            tm_min: 0,
            tm_mon: 0,
            tm_sec: 0,
            tm_wday: 0,
            tm_yday: 0,
            tm_year: 0,
            tm_zone: std::ptr::null_mut(),
        };
        let result = static_inline_helpers::CertStore_GetCertificateNotBefore_inline(
            identifier,
            &mut not_before,
        );
        if result == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(convert_tm_to_datetime(&mut not_before))
        }
    }
}

fn get_certificate_not_after(identifier: &OsString) -> Result<DateTime<Utc>, std::io::Error> {
    let identifier = std::ffi::CString::new(identifier.as_bytes()).unwrap();
    let identifier = identifier.as_ptr();
    unsafe {
        let mut not_after = static_inline_helpers::tm {
            tm_gmtoff: 0,
            tm_hour: 0,
            tm_isdst: 0,
            tm_mday: 0,
            tm_min: 0,
            tm_mon: 0,
            tm_sec: 0,
            tm_wday: 0,
            tm_yday: 0,
            tm_year: 0,
            tm_zone: std::ptr::null_mut(),
        };
        let result = static_inline_helpers::CertStore_GetCertificateNotAfter_inline(
            identifier,
            &mut not_after,
        );
        if result == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(convert_tm_to_datetime(&mut not_after))
        }
    }
}

fn move_certificate_helper(
    source_identifier: &OsStr,
    dest_identifier: &OsStr,
) -> Result<(), std::io::Error> {
    let source_identifier = std::ffi::CString::new(source_identifier.as_bytes()).unwrap();
    let source_identifier = source_identifier.as_ptr();

    let dest_identifier = std::ffi::CString::new(dest_identifier.as_bytes()).unwrap();
    let dest_identifier = dest_identifier.as_ptr();

    let result =
        unsafe { certstore::CertStore_MoveCertificate(source_identifier, dest_identifier) };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Renames a certificate. Both certificates must already be installed in the certificate store.
pub fn move_certificate(
    source: &Certificate,
    destination: &Certificate,
) -> Result<(), std::io::Error> {
    move_certificate_helper(&source.identifier, &destination.identifier)
}
