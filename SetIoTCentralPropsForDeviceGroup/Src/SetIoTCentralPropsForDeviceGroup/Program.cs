/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Identity.Client;
using RestSharp;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Threading.Tasks;

namespace SetIoTCentralPropsForDeviceGroup
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
        /// Azure Tenant Tenant ID for Azure Sphere.
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
            if (!validateCmdArgs(args))
                return -1;

            // args are.
            // args[0] : Azure IoT Central app URL (example: https://myapp.azureiotcentral.com)
            // args[1] : Azure IoT Central API Token (quoted, since there's probably a space)
            // args[2] : Azure Sphere Device Group GUID (containing the list of devices to apply the property change)
            // args[3] : JSON file containing the property to change - example: {"thermometerTelemetryUploadEnabled": true}

            // Note: the JSON containing the property to change needs to be a read/write property in the IoT Central Device Template.

            // The DeviceUpdateList will contain the Device IDs for devices in the supplied Device Group

            // Get JSON from args[3]
            string json = LoadJsonFromFile(args[3]);
            if (!IsValidJson(json))
            {
                Console.WriteLine("The JSON you supplied doesn't appear to be correctly formatted, please fix");
                return -1;
            }

            List<string> DeviceUpdateList = new List<string>();

            string token = await GetTokenAsync().ConfigureAwait(false);
            if (string.IsNullOrEmpty(token))
            {
                Console.WriteLine("Failed to get token");
                return -1;
            }

            string result = GetData("tenants", token);

            List<Tenant> tenantList = JsonSerializer.Deserialize<List<Tenant>>(result);

            if (tenantList == null || tenantList.Count == 0)
            {
                Console.WriteLine("No tenants found\n");
                return -1;
            }

            Console.WriteLine($"I found {tenantList.Count} tenant(s)");

            string tenantId = string.Empty;
            string tenantName = string.Empty;

            foreach (Tenant tenant in tenantList)
            {
                result = GetData($"tenants/{tenant.Id}/devices", token);

                Devices devices = JsonSerializer.Deserialize<Devices>(result);
                foreach (Item item in devices.Items)
                {
                    if (item.DeviceGroupId != null && item.DeviceGroupId == args[2])
                    {
                        DeviceUpdateList.Add(item.DeviceId);
                    }
                }
            }

            if (DeviceUpdateList.Count == 0)
            {
                Console.WriteLine("No devices found for the Device Group guid you provided");
                return -1;
            }

            Console.WriteLine($"{DeviceUpdateList.Count} devices will have their properties updated");

            string iotcAppUrl = args[0].TrimEnd('/');

            foreach(string s in DeviceUpdateList)
            {
                if (isValidIoTCDevice(iotcAppUrl, s.ToLower(), args[1]))
                {
                    Console.Write($"Setting device id: {s} - ");
                    if (setIoTCDeviceProperties(iotcAppUrl, s.ToLower(), args[1], json))
                    {
                        Console.WriteLine("Success");
                    }
                    else
                    {
                        Console.WriteLine("Failed");
                    }
                }
                else
                {
                    Debug.WriteLine($"Device not found in IoTC: {s}");
                }
            }

            return 0;
        }

        /// <summary>
        /// Validate the CmdArgs
        /// </summary>
        /// <param name="args"></param>
        /// <returns></returns>
        private static bool validateCmdArgs(string [] args)
        {
            Debug.WriteLine($"{args.Count()} args passed in");
            foreach(string s in args)
            {
                Debug.WriteLine(s);
            }

            if (args.Count() != 4)
            {
                Console.WriteLine("App requires the following command line arguments:");

                Console.WriteLine("Azure IoT Central Application URL i.e. https://myapp.azureiotcentral.com");
                Console.WriteLine("Azure IoT Central Application API Token");
                Console.WriteLine("Azure Sphere Device Group Id (guid)");
                Console.WriteLine("JSON file path containing properties to be set - i.e. {\"thermometerTelemetryUploadEnabled\": true}");

                return false;
            }

            // check for valid URL
            Uri uriResult;
            bool result = Uri.TryCreate(args[0], UriKind.Absolute, out uriResult)
                && uriResult.Scheme == Uri.UriSchemeHttps && args[0].ToLower().Contains("azureiotcentral.com");
            if (!result)
            {
                Console.WriteLine("The Azure IoT Central URL you provided doesn't look valid, please fix");
                return false;
            }

            if (!args[1].StartsWith("SharedAccessSignature"))
            {
                Console.WriteLine("The API token you provided doesn't look valid, please fix");
                return false;
            }

            Guid guidOutput;
            bool isValidGuid = Guid.TryParse(args[2], out guidOutput);
            if (!isValidGuid)
            {
                Console.WriteLine("The guid you supplied doesn't look right, please fix");
                return false;
            }

            if (!File.Exists(args[3]))
            {
                Console.WriteLine($"File '{args[3]}' not found, please fix");
            }

            return true;
        }

        /// <summary>
        /// setIoTCDeviceProperties - sets the user supplied JSON to the device in IoTC
        /// </summary>
        /// <param name="baseURI"></param> This is the IoTC App URL, example: https://myapp.azureiotcentral.com
        /// <param name="deviceId"></param> The device id that to apply settings to
        /// <param name="token"></param> The IoTC API Token
        /// <param name="JsonContent"></param> Content to be applied to the device
        /// <returns></returns>
        // PUT https://appsubdomain.azureiotcentral.com/api/devices/{deviceId}/properties
        static bool setIoTCDeviceProperties(string baseURI, string deviceId, string token, string JsonContent)
        {
            RestClient client = new RestClient(baseURI);
            var request = new RestRequest($"api/devices/{deviceId.ToLower()}/properties?api-version=2022-05-31", Method.Patch).AddJsonBody(JsonContent);
            request.AddParameter("Authorization", token, ParameterType.HttpHeader);
            var response = client.Execute(request);

            bool retVal = false;
            if (response.IsSuccessful)
            {
                retVal = true;
                Debug.WriteLine($"Success: {response.Content}");
            }
            else
            {
                Debug.WriteLine($"Failed to set property: {response.Content}");
            }
            return retVal;
        }

        /// <summary>
        /// Determine whether the Azure Sphere Device Id exists in the IoTC app
        /// </summary>
        /// <param name="baseURI"></param> Azure IoT Central base App URL
        /// <param name="deviceId"></param> Azure Sphere Device Id
        /// <param name="token"></param> Azure Sphere API Token
        /// <returns></returns>
        // GET https://appsubdomain.azureiotcentral.com/api/devices/{deviceId}
        private static bool isValidIoTCDevice(string baseURI, string deviceId, string token)
        {
            bool retVal = false;
            RestClient client = new RestClient(baseURI);
            var request = new RestRequest($"api/devices/{deviceId}?api-version=2022-05-31", Method.Get);
            request.AddParameter("Authorization", token, ParameterType.HttpHeader);
            var response = client.Execute(request);

            if (response.IsSuccessful)
            {
                retVal = true;
                Debug.WriteLine($"Device Get Success: {response.Content}");
            }
            return retVal;
        }

        /// <summary>
        /// Execute an Http GET on the Azure Sphere public API
        /// </summary>
        /// <param name="relativeUrl"></param> API path to execute
        /// <param name="token"></param> Azure Sphere API Token
        /// <returns></returns>
        private static string GetData(string relativeUrl, string token)
        {
            var client = new RestClient(AzureSphereApiUri);
            var request = new RestRequest($"/v2/{relativeUrl}", Method.Get);
            request.AddParameter("Authorization", string.Format("Bearer " + token), ParameterType.HttpHeader);
            var response = client.Execute(request);

            if (response.IsSuccessful)
            {
                return response.Content;
            }
            return string.Empty;
        }

        /// <summary>
        /// Get an Azure Sphere access token based on user login credentials
        /// </summary>
        /// <returns></returns>
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

        /// <summary>
        /// Function to load JSON from a file
        /// </summary>
        /// <param name="JsonFilePath"></param>
        /// <returns>String.Empty on failure, or string containing Json on success</returns>
        private static string LoadJsonFromFile(string JsonFilePath)
        {
            string result = String.Empty;
            if (File.Exists(JsonFilePath))
            {
                result = File.ReadAllText(JsonFilePath);
            }
            else
            {
                Console.WriteLine($"File '{JsonFilePath}' not found.");
            }
            return result;
        }

        /// <summary>
        /// Check to see if user input Json is valid
        /// </summary>
        /// <param name="strInput"></param>
        /// <returns></returns>
        private static bool IsValidJson(string strInput)
        {
            bool isValid = true;
            try
            {
                JsonDocument doc = JsonDocument.Parse(strInput);
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                isValid = false;
            }
            return isValid;
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
