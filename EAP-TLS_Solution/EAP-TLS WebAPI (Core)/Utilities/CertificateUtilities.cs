using System;
using System.Security.Cryptography.X509Certificates;
using System.Text;

namespace EapTlsWebAPICore.Utilities
{
    public class CertificateUtilities
    {
        // Useful to retrieve the cert from the server's Cert store (more proper if using a custom web server)
        static X509Certificate2 GetCertByThumbprint(string thumbprint)
        {
            X509Store store = null;
            X509Certificate2 cert = null;
            try
            {
                store = new X509Store(StoreName.Root, StoreLocation.LocalMachine);
                store.Open(OpenFlags.OpenExistingOnly | OpenFlags.ReadOnly);

                cert = store.Certificates.Find(X509FindType.FindByThumbprint, thumbprint, true)[0];
            }
            finally
            {
                if (store != null) store.Close();
            }

            return cert;
        }

        static public string GetX509CertificatePEM(X509Certificate2 cert)
        {
            StringBuilder builder = new StringBuilder();

            if (cert != null)
            {
                builder.AppendLine("-----BEGIN CERTIFICATE-----");
                builder.AppendLine(Convert.ToBase64String(cert.RawData, Base64FormattingOptions.None));
                builder.AppendLine("-----END CERTIFICATE-----");
            }

            return builder.ToString();
        }

        static public X509Certificate2 GetCertFromFile(string certPath, string certPassword)
        {            
            // X509Certificate is immutable on this platform. Use the equivalent constructor instead.
            try
            {
                if (string.IsNullOrEmpty(certPassword))
                {
                    X509Certificate2 cert = new X509Certificate2(certPath);
                    return cert;
                }
                else
                {
                    X509Certificate2 cert = new X509Certificate2(certPath, certPassword, X509KeyStorageFlags.PersistKeySet);
                    return cert;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(string.Format("EXCEPTION retrieving certificate at '{0}': {1}", certPath, ex.ToString()));
            }

            return null;
        }
    }
}
