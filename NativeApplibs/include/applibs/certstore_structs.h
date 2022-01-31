/// \file certstore_structs.h
/// \brief This header contains public structs that may be used by other libraries.
///
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     The maximum length of a certificate identifier (not including the null terminator).
/// </summary>
#define CERTSTORE_MAX_IDENTIFIER_LENGTH 16

/// <summary>
///    The maximum length for the certificate Subject Name field (not including the null terminator)
/// </summary>
#define CERTSTORE_SUBJECTNAME_MAX_LENGTH 300

/// <summary>
///    The maximum length for the certificate Issuer Name field (not including the null terminator)
/// </summary>
#define CERTSTORE_ISSUERNAME_MAX_LENGTH 300

/// <summary>
///     The struct containing the identifier for certificates.
/// </summary>
typedef struct CertStore_Identifier {
    // Null-terminated string containing the identifier of the certificate
    char identifier[CERTSTORE_MAX_IDENTIFIER_LENGTH + 1];
} CertStore_Identifier;

/// <summary>
///    The struct containing the Subject Name of a certificate
/// </summary>
typedef struct CertStore_SubjectName {
    char name[CERTSTORE_SUBJECTNAME_MAX_LENGTH + 1];
} CertStore_SubjectName;

/// <summary>
///    The struct containing the Issuer Name of a certificate
/// </summary>
typedef struct CertStore_IssuerName {
    char name[CERTSTORE_ISSUERNAME_MAX_LENGTH + 1];
} CertStore_IssuerName;

#ifdef __cplusplus
}
#endif
