/// \file certstore.h
/// \brief This header contains the functions needed to store, delete and list certificates in the
/// device.
///
#pragma once

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     The maximum length of a certificate blob.
/// </summary>
#define CERTSTORE_MAX_CERT_SIZE 8192

/// <summary>
///     The maximum length for the private key password (not including the null terminator)
/// </summary>
#define CERTSTORE_MAX_PRIVATE_KEY_PASSWORD_LENGTH 256

#include <applibs/certstore_structs.h>
#include <applibs/certstore_internal.h>

/// <summary>
///     Installs a client certificate that consists of a public certificate and a private key. Both
///     must be in PEM format. If the private key is encrypted, its password must be given. The
///     identifier can then be used to refer to the certificate in other functions. If any other
///     certificate already exists with the same identifier, it will be replaced with the one
///     provided.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the provided <paramref name="identifier"/>, <paramref name="certBlob"/>, or
///     <paramref name="privateKeyBlob"/> is NULL.</para>
///     <para>EACCES: this operation is not allowed because CertStore capability is not
///     present.</para>
///     <para>ERANGE: <paramref name="certBlobLength"/> or <paramref name="privateKeyBlobLength"/>
///     is zero or greater than <see cref="CERTSTORE_MAX_CERT_SIZE"/>, or the <paramref
///     name="privateKeyPassword"/> length is greater than <see
///     cref="CERTSTORE_MAX_PRIVATE_KEY_PASSWORD_LENGTH"/> or is not null-terminated, or <paramref
///     name="identifier"/> is greater than <see cref="CERTSTORE_MAX_IDENTIFIER_LENGTH"/>.</para>
///     <para>ENOSPC: there is not enough space in the certificate storage for this
///     certificate.</para>
///     <para>EINVAL: the <paramref name="identifier"/> is not null-terminated or either the
///     <paramref name="certBlob"/> or the <paramref name="privateKeyBlob"/> has invalid
///     data, or a password is provided for an unencrypted private key.</para>
///     <para>EAGAIN: the OS certstore component isn't ready yet.</para>
///     Any other errno may also be specified; such errors aren't deterministic and
///     there's no guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="identifier">
///     The certificate identifier.
/// </param>
/// <param name="certBlob">
///     A pointer to a blob that has the public certificate contents in PEM format.
/// </param>
/// <param name="certBlobLength">
///     The length of the public certificate blob not including the null terminator.
/// </param>
/// <param name="privateKeyBlob">
///     A pointer to a blob that has the private key contents in PEM format.
/// </param>
/// <param name="privateKeyBlobLength">
///     The length of the private key blob not including the null terminator.
/// </param>
/// <param name="privateKeyPassword">
///     A pointer to the password string to decrypt the <paramref name="privateKeyBlob"/>. The
///     string must be null-terminated and the length must be less than or equals to <see
///     cref="CERTSTORE_MAX_PRIVATE_KEY_PASSWORD_LENGTH"/> bytes (excluding the termination). If
///     <paramref name="privateKeyBlob"/> is encrypted, this parameter is required. Otherwise, set
///     this parameter to NULL.
/// </param>
/// <remarks>
///     A valid identifier must be a unique string from one to <see
///     cref="CERTSTORE_MAX_IDENTIFIER_LENGTH"/> characters in length. The following characters are
///     valid in an identifier:
///     - 'A' to 'Z'
///     - 'a' to 'z'
///     - '0' to '9'
///     - '.' or '-' or '_'
/// </remarks>
/// <returns>
///     0 for success, -1 for failure, in which case errno is set to the error value.
/// </returns>
int CertStore_InstallClientCertificate(const char *identifier, const char *certBlob,
                                       size_t certBlobLength, const char *privateKeyBlob,
                                       size_t privateKeyBlobLength, const char *privateKeyPassword);

/// <summary>
///     Installs a root CA certificate that consists of a public certificate in PEM format. The
///     identifier can then be used to refer to the certificate in other functions. If any other
///     certificate already exists with the same identifier, it will be replaced with the one
///     provided.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the provided <paramref name="identifier"/> or <paramref name="certBlob"/> is
///     NULL.</para>
///     <para>EACCES: this operation is not allowed because CertStore capability is not
///     present.</para>
///     <para>ERANGE: <paramref name="certBlobLength"/> is zero or greater than
///     CERTSTORE_MAX_CERT_SIZE or <paramref name="identifier"/> is greater than <see
///     cref="CERTSTORE_MAX_IDENTIFIER_LENGTH"/>.</para>
///     <para>ENOSPC: there is not enough space in the certificate storage for this
///     certificate.</para>
///     <para>EINVAL: the <paramref name="identifier"/> is not null-terminated or the <paramref
///     name="certBlob"/> has invalid data.</para>
///     <para>EAGAIN: the OS certstore component isn't ready yet.</para>
///     Any other errno may also be specified; such errors aren't deterministic
///     and there's no guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="identifier">
///     The certificate identifier.
/// </param>
/// <param name="certBlob">
///     A pointer to a blob that has the public certificate contents in PEM format.
/// </param>
/// <param name="certBlobLength">
///     The length of the public certificate blob not including the null terminator.
/// </param>
/// <remarks>
///     A valid identifier must be a unique string from one to <see
///     cref="CERTSTORE_MAX_IDENTIFIER_LENGTH"/> characters in length. The following characters are
///     valid in an identifier:
///     - 'A' to 'Z'
///     - 'a' to 'z'
///     - '0' to '9'
///     - '.' or '-' or '_'
/// </remarks>
/// <returns>
///     0 for success, -1 for failure, in which case errno is set to the error value.
/// </returns>
int CertStore_InstallRootCACertificate(const char *identifier, const char *certBlob,
                                       size_t certBlobLength);

/// <summary>
///     Deletes the certificate referenced by the identifier.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the provided <paramref name="identifier"/> is NULL.</para>
///     <para>EAGAIN: the OS is not ready for certificate operations, please try the
///     request again later.</para>
///     <para>EACCES: this operation is not allowed because CertStore capability is not
///     present.</para>
///     <para>ENOENT: no certificate with the identifier provided is
///     installed.</para>
///     Any other errno may also be specified; such errors aren't deterministic and
///     there's no guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="identifier">
///     The certificate identifier.
/// </param>
/// <returns>
///     0 for success, -1 for failure, in which case errno is set to the error value.
/// </returns>
int CertStore_DeleteCertificate(const char *identifier);

/// <summary>
///     Gets the number of certificates installed on the device.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EAGAIN: the OS certstore component isn't ready yet.</para>
///     <para>EACCES: this operation is not allowed because CertStore capability is not
///     present.</para>
///     Any other errno may also be specified; such errors aren't deterministic and
///     there's no guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <returns>
///     If the call is successful, the number of installed certificates is returned. Otherwise, it
///     returns -1 for failure, in which case errno is set to the error value.
/// </returns>
ssize_t CertStore_GetCertificateCount(void);

/// <summary>
///     Gets the identifier of the certificate at a given index. The caller can call the <see
///     cref="CertStore_GetCertificateCount"/> API in order to get the total count and then call
///     this API with index values from 0 to (count - 1).
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EAGAIN: the OS certstore component isn't ready yet.</para>
///     <para>EACCES: this operation is not allowed because CertStore capability is not
///     present.</para>
///     <para>ERANGE: the <paramref name="index"/> is not in the valid range.</para>
///     <para>EFAULT: the <paramref name="outIdentifier"/> is null.</para>
///     Any other errno may also be specified; such errors aren't deterministic and
///     there's no guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="index">
///     The index of the certificate whose identifier needs to be returned. The index starts at 0
///     and goes till the count of certificates in the system.
/// </param>
/// <param name="outIdentifier">
///     The pointer to the struct <see cref="CertStore_Identifier"/> that receives the
///     identifier of the certificate installed on the device.
/// </param>
/// <returns>
///     0 for success, -1 for failure, in which case errno is set to the error value.
/// </returns>
int CertStore_GetCertificateIdentifierAt(size_t index, CertStore_Identifier *outIdentifier);

/// <summary>
///     Gets the available space, in bytes, to install certificates.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EAGAIN: the OS certstore component isn't ready yet.</para>
///     <para>EACCES: this operation is not allowed because CertStore capability is not
///     present.</para>
///     Any other errno may also be specified; such errors aren't deterministic and
///     there's no guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <returns>
///     If the call is successful, the available space in bytes is returned. Otherwise, it returns
///     -1 for failure, in which case errno is set to the error value.
/// </returns>
ssize_t CertStore_GetAvailableSpace(void);

/// <summary>
///     Retrieves the subject name field from the certificate with the specified identifier
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCESS: the application manifest does not include the CertStore capability.</para>
///     <para>EAGAIN: the OS certstore component isn't ready yet.</para>
///     <para>EFAULT: the <paramref name="identifier"/> or <paramref name="outSubjectName"/> is
///     NULL.</para>
///     <para>EINVAL: the <paramref name="identifier"/> specifies an invalid or corrupted
///     certificate.</para>
///     <para>ENOENT: the <paramref name="identifier"/> is not found.</para>
///     Any other errno may also be specified; such errors aren't deterministic and
///     there's no guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="identifier">
///     The certificate identifier
/// </param>
/// <param name="outSubjectName">
///     A pointer to a struct that will be populated with the field
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int CertStore_GetCertificateSubjectName(const char *identifier,
                                               struct CertStore_SubjectName *outSubjectName);

/// <summary>
///     Retrieves the issuer name field from the certificate with the specified identifier
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCESS: the application manifest does not include the CertStore capability.</para>
///     <para>EAGAIN: the OS certstore component isn't ready yet.</para>
///     <para>EFAULT: the <paramref name="identifier"/> or <paramref name="outIssuerName"/> is
///     NULL.</para>
///     <para>EINVAL: the <paramref name="identifier"/> specifies an invalid or corrupted
///     certificate.</para>
///     <para>ENOENT: the <paramref name="identifier"/> is not found.</para>
///     Any other errno may also be specified; such errors aren't deterministic and
///     there's no guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="identifier">
///     The certificate identifier
/// </param>
/// <param name="outIssuerName">
///     A pointer to a struct that will be populated with the Issuer Name field
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int CertStore_GetCertificateIssuerName(const char *identifier,
                                              struct CertStore_IssuerName *outIssuerName);

/// <summary>
///     Retrieves the NotBefore field from the certificate with the specified identifier
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCESS: the application manifest does not include the CertStore capability.</para>
///     <para>EAGAIN: the OS certstore component isn't ready yet.</para>
///     <para>EFAULT: the <paramref name="identifier"/> or <paramref name="outNotBefore"/> is
///     NULL.</para>
///     <para>EINVAL: the <paramref name="identifier"/> specifies an invalid or corrupted
///     certificate.</para>
///     <para>ENOENT: the <paramref name="identifier"/> is not found.</para>
///     Any other errno may also be specified; such errors aren't deterministic and
///     there's no guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="identifier">
///     The certificate identifier
/// </param>
/// <param name="outNotBefore">
///     A pointer to a struct that will be populated with the Not Before validity data.
///     The following fields will be undefined in the returned structure: tm_wday, tm_yday, tm_isdst
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int CertStore_GetCertificateNotBefore(const char *identifier, struct tm *outNotBefore);

/// <summary>
///     Retrieves the Not After field from the certificate with the specified identifier
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCESS: the application manifest does not include the CertStore capability.</para>
///     <para>EAGAIN: the OS certstore component isn't ready yet.</para>
///     <para>EFAULT: the <paramref name="identifier"/> or <paramref name="outNotAfter"/> is
///     NULL.</para>
///     <para>EINVAL: the <paramref name="identifier"/> specifies an invalid or corrupted
///     certificate.</para>
///     <para>ENOENT: the <paramref name="identifier"/> is not found.</para>
///     Any other errno may also be specified; such errors aren't deterministic and
///     there's no guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="identifier">
///     The certificate identifier
/// </param>
/// <param name="outNotAfter">
///     A pointer to a struct that will be populated with the Not After validity data
///     The following fields will be undefined in the returned structure: tm_wday, tm_yday, tm_isdst
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int CertStore_GetCertificateNotAfter(const char *identifier, struct tm *outNotAfter);

/// <summary>
///     Renames the certificate with identifier <paramref name="sourceIdentifier"/> to
///     identifier <paramref name="destIdentifier"/>. If any other certificate with identifier
///     <paramref name="destIdentifier"/> already exists, its contents will be replaced. This
///     operation is atomic. However, any Wifi configuration using the referenced certificates will
///     not see any effect until the configuration is reloaded with <see
///     cref="WifiConfig_ReloadConfig"/>.
///     <para>**Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: <paramref name="sourceIdentifier"/> or <paramref name="destIdentifier"/> is
///     NULL.</para>
///     <para>EINVAL: <paramref name="sourceIdentifier"/> or <paramref name="destIdentifier"/> is an
///     invalid identifier.</para>
///     <para>ERANGE: <paramref name="sourceIdentifier"/> or <paramref name="destIdentifier"/> is
///     greater than <see cref="CERTSTORE_MAX_IDENTIFIER_LENGTH"/>.</para>
///     <para>ENOENT: the certificate with the identifier <paramref name="sourceIdentifier"/> does
///     not exist.</para>
///     <para>EACCES: this operation is not allowed because the CertStore capability is not
///     present.</para>
///     <para>EAGAIN: the OS certstore component isn't ready yet.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="sourceIdentifier">
///     The identifier of the existing certificate to move. The identifier will no longer exist when
///     the operation completes successfully.
/// </param>
/// <param name="destIdentifier">
///     The identifier that the source certificate will take on.
/// </param>
/// <returns>
///     0 for success, -1 for failure, in which case errno is set to the error value.
/// </returns>
int CertStore_MoveCertificate(const char *sourceIdentifier, const char *destIdentifier);

#ifdef __cplusplus
}
#endif
