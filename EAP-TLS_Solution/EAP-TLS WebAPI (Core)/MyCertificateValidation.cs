using System;
using System.Diagnostics;
using System.Security.Cryptography.X509Certificates;

namespace EapTlsWebAPICore
{
    public class MyCertificateValidation 
    {
        static public bool ValidateCertificate(X509Certificate2 clientCertificate)
        {
            Console.WriteLine("Azure Sphere device Id: " + clientCertificate?.GetNameInfo(X509NameType.SimpleName, false));
            Console.WriteLine("Certificate Thumbprint: " + clientCertificate?.Thumbprint);

            bool isValid = true;
            // [...] Your *additional* custom validation logic in here                               
            return isValid;
        }
    }
}
