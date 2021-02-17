/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Identity.Client;
using RestSharp;
using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Newtonsoft.Json;
using System.Linq;
using System.Collections.Immutable;

namespace DeviceTenantFinder
{
    class Program
    {
        /// <summary>
        /// Azure Sphere Public API resource URI
        /// </summary>
        private static readonly List<string> Scopes = new List<string>() { "https://sphere.azure.net/api/user_impersonation" };
        /// <summary>
        /// Azure Sphere Public API client application ID.
        /// </summary>
        private const string ClientApplicationId = "0B1C8F7E-28D2-4378-97E2-7D7D63F7C87F";
        /// <summary>
        /// Azure Sphere Tenant ID.
        /// </summary>
        public const string Tenant = "7d71c83c-ccdf-45b7-b3c9-9c41b94406d9";
        /// <summary>
        /// Azure Sphere Public API URI
        /// </summary>
        private const string AzureSphereApiUri = "https://prod.core.sphere.azure.net";
        /// <summary>
        /// Program entry-point.
        /// </summary>
        /// <returns>Zero on success, otherwise non-zero.</returns>

        public static async Task<int> Main(string[] args)
        {
            if (args.Count() != 1)
            {
                Console.WriteLine("App requires a device id on the command line");
                return -1;
            }

            if (args[0].Length != 128)
            {
                Console.WriteLine("Device Id Length is expected to be 128 characters, try again");
                return -1;
            }

            // we should have something that looks like a device Id, now go get tenants, devices within a tenant, and look for the device Id

            string token = await GetTokenAsync().ConfigureAwait(false);
            if (string.IsNullOrEmpty(token))
            {
                Console.WriteLine("Failed to get token");
                return -1;
            }

            string result = GetData("tenants", token);

            List<Tenant> tenantList = JsonConvert.DeserializeObject<List<Tenant>>(result);

            if (tenantList.Count == 0)
            {
                Console.WriteLine("No tenants found\n");
                return -1;
            }

            Console.WriteLine($"I found {tenantList.Count} tenant(s)");

            bool deviceFound = false;
            string tenantId = string.Empty;
            string tenantName = string.Empty;

            string productId = "None";
            string deviceGroupId = "None";


            foreach (Tenant tenant in tenantList)
            {
                result = GetData($"tenants/{tenant.Id}/devices", token);

                Devices devices = JsonConvert.DeserializeObject<Devices>(result);
                foreach (Item item in devices.Items)
                {
                    if (item.DeviceId == args[0])
                    {
                        deviceFound = true;
                        tenantId = tenant.Id;
                        tenantName = tenant.Name;
                        if (item.DeviceGroupId != null)
                            deviceGroupId = item.DeviceGroupId;
                        if (item.ProductId != null)
                            productId = item.ProductId;
                        break;
                    }
                }
            }

            if (!deviceFound)
            {
                Console.WriteLine($"Device Id Not Found: {args[0]}");
                return -1;
            }

            Console.WriteLine($"Device Found   : {args[0]}");
            Console.WriteLine($"Tenant Id      : {tenantId}");
            Console.WriteLine($"Tenant Name    : {tenantName}");
            Console.WriteLine($"Product Id     : {productId}");
            Console.WriteLine($"Device Group Id: {deviceGroupId}");


            return 0;
        }

        private static string GetData(string relativeUrl, string token)
        {
            var client = new RestClient(AzureSphereApiUri);
            var request = new RestRequest($"/v2/{relativeUrl}", Method.GET);
            request.AddParameter("Authorization", string.Format("Bearer " + token), ParameterType.HttpHeader);
            var response = client.Execute(request);

            if (response.IsSuccessful)
            {
                return response.Content;
            }
            return string.Empty;
        }

        private static async Task<string> GetTokenAsync()
        {
            IPublicClientApplication publicClientApp = PublicClientApplicationBuilder
                .Create(ClientApplicationId)
                .WithAuthority(AzureCloudInstance.AzurePublic, Tenant)
                .WithRedirectUri("http://localhost")
                .Build();

            AuthenticationResult authResult = await publicClientApp.AcquireTokenInteractive(Scopes).ExecuteAsync();

            return authResult.AccessToken;
        }
    }
}

// Tenant helper for JSON deserialization
public class Tenant
{
    public string Id { get; set; }
    public string Name { get; set; }
    public string[] Roles { get; set; }
}

// Devices helper for JSON deserialization

public class Devices
{
    public Item[] Items { get; set; }
    public object ContinuationToken { get; set; }
}

public class Item
{
    public string DeviceId { get; set; }
    public string TenantId { get; set; }
    public string ProductId { get; set; }
    public string DeviceGroupId { get; set; }
    public int ChipSku { get; set; }
}
