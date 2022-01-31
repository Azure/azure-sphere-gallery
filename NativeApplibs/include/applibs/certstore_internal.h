/// \file certstore_internal.h
/// \brief This header contains internal functions for the CertStore library, which
/// manages certificates on a device.
///
/// These functions are not thread safe.
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

enum z__CertInfoField {
    z__CertInfo_NotBefore = 1,
    z__CertInfo_NotAfter = 2,
    z__CertInfo_SubjectName = 3,
    z__CertInfo_IssuerName = 4
};

int z__CertStore_GetCertificateInfo(const char *identifier, enum z__CertInfoField field, void *data,
                                    size_t size);

static inline int CertStore_GetCertificateSubjectName(const char *identifier,
                                                      struct CertStore_SubjectName *outSubjectName)
{
    return z__CertStore_GetCertificateInfo(identifier, z__CertInfo_SubjectName, (void *)outSubjectName,
                                           sizeof(struct CertStore_SubjectName));
}

static inline int CertStore_GetCertificateIssuerName(const char *identifier,
                                                     struct CertStore_IssuerName *outIssuerName)
{
    return z__CertStore_GetCertificateInfo(identifier, z__CertInfo_IssuerName, (void *)outIssuerName,
                                           sizeof(struct CertStore_IssuerName));
}

static inline int CertStore_GetCertificateNotBefore(const char *identifier, struct tm *outNotBefore)
{
    return z__CertStore_GetCertificateInfo(identifier, z__CertInfo_NotBefore, (void *)outNotBefore,
                                           sizeof(struct tm));
}

static inline int CertStore_GetCertificateNotAfter(const char *identifier, struct tm *outNotAfter)
{
    return z__CertStore_GetCertificateInfo(identifier, z__CertInfo_NotAfter, (void *)outNotAfter,
                                           sizeof(struct tm));
}

#ifdef __cplusplus
}
#endif
