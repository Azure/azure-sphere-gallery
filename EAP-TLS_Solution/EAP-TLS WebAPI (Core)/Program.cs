using System;
using System.IO;
using System.Net;
using System.Security.Cryptography.X509Certificates;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Server.Kestrel.Core;
using Microsoft.AspNetCore.Server.Kestrel.Https;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Configuration;

namespace EapTlsWebAPICore
{
    public class Program
    {
        // NOTE: register 'az-CA' and the AS3 tenant's public certificates into the (hosting machine's) "User Trusted Certificates store"

        public static int Main(string[] args)
        {
            try
            {
                Console.WriteLine("Starting web host");
                CreateHostBuilder(args).Build().Run();
                return 0;
            }
            catch (Exception ex)
            {
                Console.WriteLine("Host terminated unexpectedly");
                Console.WriteLine(ex.ToString());
                return 1;
            }
        }

        public static IHostBuilder CreateHostBuilder(string[] args) =>
           Host.CreateDefaultBuilder(args)
               .ConfigureWebHostDefaults(webBuilder =>
               {
                   webBuilder.UseStartup<Startup>()
                   .ConfigureKestrel(options =>
                   {
                       var configuration = options.ApplicationServices.GetRequiredService<IConfiguration>();

                       // Being this a self-hosted web-server, we need to setup the Server's *private* certificate.
                       // In a production hosting environment, this will be deployed by the administrators in the hosting Web Server
                       var rootCACertificate = new X509Certificate2(Path.Combine("certs", configuration.GetValue<string>("webApiPrivateCertificateFileName")), configuration.GetValue<string>("webApiPrivateKeyPassword"));

                       options.Limits.MaxConcurrentConnections = 10;
                       options.Limits.RequestHeadersTimeout = TimeSpan.FromMinutes(1);
                       options.Limits.MinRequestBodyDataRate = null;
                       options.ConfigureHttpsDefaults(o =>
                       {
                           o.ServerCertificate = rootCACertificate;
                           o.ClientCertificateMode = ClientCertificateMode.RequireCertificate;
                           o.SslProtocols = System.Security.Authentication.SslProtocols.Tls12;
                           o.ClientCertificateValidation = (cert, certChain, sslPolicyErrors) =>
                           {
                               // https://docs.microsoft.com/en-us/aspnet/core/security/authentication/certauth
                               Console.WriteLine("Azure Sphere Device ID: " + cert?.GetNameInfo(X509NameType.SimpleName, false));
                               Console.WriteLine("Certificate Thumbprint: " + cert?.Thumbprint);

                               bool isValid = true;
                               // [...] Your *additional* custom validation logic in here   
                               //isValid = MyCertificateValidation.ValidateCertificate(cert);
                               return isValid;
                           };
                       });
                       options.Listen(new IPEndPoint(new IPAddress(0), 44378), listenOptions =>
                       {
                           listenOptions.UseHttps(rootCACertificate);
                           listenOptions.Protocols = HttpProtocols.Http1AndHttp2;
                           listenOptions.UseConnectionLogging();

                       });
                   });
               });
    }
}
