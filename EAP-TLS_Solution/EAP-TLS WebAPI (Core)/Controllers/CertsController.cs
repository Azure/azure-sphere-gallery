using System;
using System.IO;
using System.Security.Cryptography.X509Certificates;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Configuration;

namespace EapTlsWebAPICore.Controllers
{
    public class ResponseToClient
    {
        public DateTimeOffset Timestamp { get; set; }
        public string RootCACertificate { get; set; }
        public string EapTlsNetworkSsid { get; set; }
        public string ClientIdentity { get; set; }        
        public string ClientPublicCertificate { get; set; }
        public string ClientPrivateKey { get; set; }
        public string ClientPrivateKeyPass { get; set; } // Optionally, it could be injected into the Azure Sphere device, through a SW update
    }

    [ApiController]
    [Route("api/[controller]")]
    public class CertsController : Controller
    {
        private readonly IConfiguration _config;
        
        public CertsController(IConfiguration config)
        {
            _config = config;
        }

        private ResponseToClient CallCMSDeviceVerification(X509Certificate2 clientDAACertificate, bool needRootCACertificate, bool needClientCertificate)
        {
            string deviceId = clientDAACertificate.GetNameInfo(X509NameType.SimpleName, false);
            string rootCACertificatePEM = null;
            string newEapTlsNetworkSsid = null;
            string newClientIdentity = null;
            string newClientACertificatePEM = null;
            string newClientPrivateKey = null;
            string newClientPrivateKeyPass = "";
            ResponseToClient response = new ResponseToClient
            {
                Timestamp = DateTime.UtcNow,
            };

            try
            {
#if TEST_RESPONSE
                // All below values should be returned by the call to the CMS.
                // At the moment, just filling-in with locally-stored certs (even though it could be enough for the PoC)
                newEapTlsNetworkSsid = _config.GetValue<string>("eapTlsNetworkSsid");
                var fileProvider = new Microsoft.Extensions.FileProviders.PhysicalFileProvider(Directory.GetCurrentDirectory());
                string certPath = Path.Combine(fileProvider.Root, _config.GetValue<string>("certificateStorePath"));
                if (needRootCACertificate)
                {
                    rootCACertificatePEM = System.IO.File.ReadAllText(Path.Combine(certPath, _config.GetValue<string>("radiusRootCAPublicCertificateFileName")));
                }
                if (needClientCertificate)
                {
                    newClientIdentity = _config.GetValue<string>("clientIdentity");
                    newClientACertificatePEM = System.IO.File.ReadAllText(Path.Combine(certPath, _config.GetValue<string>("clientPublicCertificateFileName")));
                    newClientPrivateKey = System.IO.File.ReadAllText(Path.Combine(certPath, _config.GetValue<string>("clientPrivateKeyFileName")));

                    // This should ideally be provided to the client through i.e. an AS3 SW update, for extra-security.
                    newClientPrivateKeyPass = ""; //_config.GetValue<string>("clientPrivateKeyPassword")
                }
                fileProvider.Dispose();
#else
                /*
                    TODO:
                        - Call the CMS to retrieve the client certificate for the caller device.
                        - The CMS will verify if caller's 'deviceId' is recorded in a Custom DB of valid/deployed Azure Sphere devices.

                    If pass, the CMS will return:
                        - The current valid RootCA certificate: except for the first device enrollment, most of the time it will not be requested by the client.
                          This certificate must be kept updated by the Administrator within this CMS's WebAPI's host configurations, and eventually pushed to the client anyways (and therefore setting needRootCACertificate=true in the response!)
                        - The EP-TLS's SSID, thus allowing the device to be administered and eventually moved across facilities
                        - A valid client identity & full private certificate for the client caller device
                */
#endif
                response.EapTlsNetworkSsid = newEapTlsNetworkSsid;

                if (needRootCACertificate)
                {
                    response.RootCACertificate = rootCACertificatePEM;

                }
                else
                {
                    // parson.c returns error for null strings
                    response.RootCACertificate = "";
                }
                if (needClientCertificate)
                {
                    response.ClientIdentity = newClientIdentity;
                    response.ClientPublicCertificate = newClientACertificatePEM;
                    response.ClientPrivateKey = newClientPrivateKey;
                    response.ClientPrivateKeyPass = newClientPrivateKeyPass; // This should ideally be provided to the client through an AS3 SW update, for extra-security.
                }
                else
                {
                    // parson.c returns error for null strings
                    response.ClientIdentity = "";
                    response.ClientPublicCertificate = "";
                    response.ClientPrivateKey = "";
                    response.ClientPrivateKeyPass = "";
                }

            }
            catch (Exception ex)
            {
                Console.WriteLine(string.Format("Error calling the CMS: {0} \n{1}", ex.Message, ex.ToString()));
            }

            return response;
        }

        // NOTE: These parameter names must match the ones defined in the Azure Sphere App code
        [HttpGet]        
        public ActionResult<ResponseToClient> Get([FromQuery] bool needRootCACertificate, [FromQuery] bool needClientCertificate)
        {
            Console.WriteLine(string.Format("Serving request: needRootCACertificate=={0}, needClientCertificate=={1}", needRootCACertificate.ToString(), needClientCertificate.ToString()));

            X509Certificate2 clientDAACertificate = Request.HttpContext.Connection.ClientCertificate;
            ResponseToClient response = new ResponseToClient { Timestamp = DateTime.UtcNow };

            if (clientDAACertificate != null)
            {
                try
                {
                    response = CallCMSDeviceVerification(clientDAACertificate, needRootCACertificate, needClientCertificate);
                }
                catch (Exception ex)
                {
                    Console.WriteLine(string.Format("Error extracting the DAA client certificate: {0} \n{1}", ex.Message, ex.ToString()));
                }
            }

            return Json(response);
        }
    }
}
