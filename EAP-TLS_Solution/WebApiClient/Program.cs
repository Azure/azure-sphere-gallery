using System;
using System.IO;
using System.Net;
using System.Security.Cryptography.X509Certificates;

namespace AppNetApi_Client
{
    class Program
    {
        static void Main()
        {
            CallHttpsApi(false, false);
            CallHttpsApi(false, true);
            CallHttpsApi(true, false);
            CallHttpsApi(true, true);
        }

        static X509Certificate2 GetCertByThumbprint(string thumbprint)
        {
            X509Store store = null;
            X509Certificate2 cert = null;
            try
            {
                store = new X509Store(StoreName.Root, StoreLocation.LocalMachine);
                store.Open(OpenFlags.OpenExistingOnly | OpenFlags.ReadOnly);

                cert = store.Certificates.Find(
                                    X509FindType.FindByThumbprint,
                                    thumbprint,
                                    true
                                    )[0];
            }
            finally
            {
                if (store != null) store.Close();
            }

            return cert;
        }

        static X509Certificate2 GetCertFromFile(string certPath, string certPassword)
        {
            X509Certificate2 cert = new X509Certificate2();
            cert.Import(certPath, certPassword, X509KeyStorageFlags.PersistKeySet);

            return cert;
        }

        static void CallHttpsApi(bool needRootCACertificate, bool needClientCertificate)
        {            
            // These definitions must match the ones used in the Clients's code!!!
            const string WEB_API_ROOTCERTIFICATE_FIELD = "needRootCACertificate";
            const string WEB_API_CLIENTCERTIFICATE_FIELD = "needClientCertificate";
            string certFile = @"..\..\certs\iotuser.pfx";
            string certPassword = "1234";

            string baseWebUrl = "localhost:44378/api/certs";
            string url = string.Format("https://{0}/api/certs?{1}", baseWebUrl, WEB_API_ROOTCERTIFICATE_FIELD + "=" + needRootCACertificate.ToString() + "&" + WEB_API_CLIENTCERTIFICATE_FIELD + "=" + needClientCertificate.ToString());

            HttpWebRequest req = (HttpWebRequest)WebRequest.Create(url);
            req.Credentials = CredentialCache.DefaultCredentials;
            req.ClientCertificates.Add(GetCertFromFile(certFile, certPassword));
            HttpWebResponse resp = (HttpWebResponse)req.GetResponse();
            using (var readStream = new StreamReader(resp.GetResponseStream()))
            {
                Console.WriteLine(readStream.ReadToEnd());
            }
            Console.ReadLine();
        }
    }
}
