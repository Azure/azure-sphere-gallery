/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Azure.Identity;
using RestSharp;
using System;
using System.Threading.Tasks;

namespace WebHookPublicAPIServicePrincipal
{
    public class Utils
    {
        /// <summary>
        /// Azure Sphere Public API URI
        /// </summary>
        private const string AzureSphereApiUri = "https://prod.core.sphere.azure.net";
        public static string GetData(string EndpointUrl, string token)
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

        public static async Task<string> GetAS3Token()
        {
            DefaultAzureCredential credential = new DefaultAzureCredential();
            var result = await credential.GetTokenAsync(new Azure.Core.TokenRequestContext(
            new[] { "https://firstparty.sphere.azure.net/api/.default" }));

            return result.Token;
        }
    }

    public class DeviceInfo
    {
        public string DeviceId { get; set; }
        public string TenantId { get; set; }
        public string ProductId { get; set; }
        public string DeviceGroupId { get; set; }
        public int ChipSku { get; set; }
    }

    public class DeviceGroupInfo
    {
        public string Id { get; set; }
        public string TenantId { get; set; }
        public int OsFeedType { get; set; }
        public string ProductId { get; set; }
        public string Name { get; set; }
        public string Description { get; set; }
        public object CurrentDeployment { get; set; }
        public int UpdatePolicy { get; set; }
        public bool AllowCrashDumpsCollection { get; set; }
    }

    public class ProductInfo
    {
        public string Id { get; set; }
        public string TenantId { get; set; }
        public string Name { get; set; }
        public object Description { get; set; }
    }

}
