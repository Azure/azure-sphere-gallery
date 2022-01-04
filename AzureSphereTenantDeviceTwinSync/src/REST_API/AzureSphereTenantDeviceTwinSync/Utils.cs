using Azure.Identity;
using Azure.Security.KeyVault.Secrets;
using Newtonsoft.Json;
using RestSharp;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace DevicePropertiesWebHook
{
    public class Utils
    {
        private static string keyVaultUrl = "";        // TODO: Add your KeyVault URL here.

        /// <summary>
        /// Attempts to get a Value from Keyvault based on a given key.
        /// </summary>
        public static string GetKeyVaultString(string Key)
        {
            var client = new SecretClient(new Uri(keyVaultUrl), new DefaultAzureCredential());
            KeyVaultSecret secret = client.GetSecret(Key);

            return secret.Value;
        }

        /// <summary>
        /// Used to return a Managed Identity token (not Device Code Flow)
        /// </summary>
        /// <returns></returns>
        public static async Task<string> GetAS3Token()
        {
            DefaultAzureCredential credential = new DefaultAzureCredential();
            var result = await credential.GetTokenAsync(new Azure.Core.TokenRequestContext(
            new[] { "https://firstparty.sphere.azure.net/api/.default" }));

            return result.Token;
        }

        /// <summary>
        /// Azure Sphere Public API URI
        /// </summary>
        private const string AzureSphereApiUri = "https://prod.core.sphere.azure.net";

        public static string GetAS3Data(string EndpointUrl, string token)
        {
            var request = (RestRequest)null;
            var client = (RestClient)null;

            // calling an Azure Sphere API
            if (!string.IsNullOrEmpty(token))
            {
                client = new RestClient(AzureSphereApiUri);
                request = new RestRequest($"/v2/{EndpointUrl}", Method.GET);
                request.AddParameter("Authorization", string.Format("Bearer " + token), ParameterType.HttpHeader);
            }
            else
            {
                Uri hostUri = new Uri(EndpointUrl);
                string hostName = hostUri.Host;
                string relativePath = hostUri.PathAndQuery;
                client = new RestClient("https://" + hostName);
                request = new RestRequest(relativePath, Method.GET);
            }
            var response = client.Execute(request);

            if (response.IsSuccessful)
            {
                return response.Content;
            }
            return string.Empty;
        }
    }
}
