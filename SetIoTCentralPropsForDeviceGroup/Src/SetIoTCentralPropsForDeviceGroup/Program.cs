/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Identity.Client;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using RestSharp;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
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
            // args[3] : Valid JSON containing the property to change (example: { "StatusLed" : false } )

            // The DeviceUpdateList will contain the Device IDs for devices in the supplied Device Group
            List<string> DeviceUpdateList = new List<string>();


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

            string tenantId = string.Empty;
            string tenantName = string.Empty;

            foreach (Tenant tenant in tenantList)
            {
                result = GetData($"tenants/{tenant.Id}/devices", token);

                Devices devices = JsonConvert.DeserializeObject<Devices>(result);
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
                    string currentProperties=GetDeviceProperties(iotcAppUrl, s.ToLower(), args[1]);
                    Debug.WriteLine(currentProperties);
                    if (setIoTCDeviceProperties(iotcAppUrl, s.ToLower(), args[1], args[3],currentProperties))
                    {
                        Console.WriteLine("Successed");
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
                Console.WriteLine("JSON properties i.e. \"{\"StatusLED\": true}\"");

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

            if (!IsValidJson(args[3]))
            {
                Console.WriteLine("The JSON you supplied doesn't appear to be correctly formatted, please fix");
                return false;
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
        /// <param name="currentProperties"></param> IoTC device properties (contains the device template)
        /// <returns></returns>
        // PUT https://appsubdomain.azureiotcentral.com/api/preview/devices/{deviceId}/properties
        static bool setIoTCDeviceProperties(string baseURI, string deviceId, string token, string JsonContent, string currentProperties)
        {
            // get first key from properties
            IList<JToken> templateObj = JObject.Parse(currentProperties);
            var iotcTemplate = ((JProperty)templateObj[0]).Name;

            IList<JToken> userObj = JObject.Parse(JsonContent);
            var userKey = ((JProperty)userObj[0]).Name;
            var userValue= ((JProperty)userObj[0]).Value;

            bool quoteUserValue = false;
            if (userValue.Type == JTokenType.String)
                quoteUserValue = true;

            // now build the JSON to send to the IoTC API
            string json = "{\"" + iotcTemplate + "\":{\"" + userKey + "\":";
            if (quoteUserValue)
                json += "\"";
            if (userValue.Type == JTokenType.Boolean)
            {
                if (userValue.ToString() == "True")
                    json += "true";
                else
                    json += "false";
            }
            else
            {
                json += userValue;
            }
            if (quoteUserValue)
                json += "\"";
            json += "}}";

            Debug.WriteLine(json);

            bool retVal = false;

            RestClient client = new RestClient(baseURI);

            var request = new RestRequest($"api/preview/devices/{deviceId.ToLower()}/properties", Method.PUT);
            request.AddJsonBody(json);

            request.AddParameter("Authorization", token, ParameterType.HttpHeader);
            var response = client.Execute(request);

            if (response.IsSuccessful)
            {
                retVal = true;
                Debug.WriteLine($"Device Get Successed: {response.Content}");
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
        // GET https://appsubdomain.azureiotcentral.com/api/preview/devices/{deviceId}
        private static bool isValidIoTCDevice(string baseURI, string deviceId, string token)
        {
            bool retVal = false;
            RestClient client = new RestClient(baseURI);
            var request = new RestRequest($"api/preview/devices/{deviceId}", Method.GET);
            request.AddParameter("Authorization", token, ParameterType.HttpHeader);
            var response = client.Execute(request);

            if (response.IsSuccessful)
            {
                retVal = true;
                Debug.WriteLine($"Device Get Successed: {response.Content}");
            }
            return retVal;
        }

        // GET https://appsubdomain.azureiotcentral.com/api/preview/devices/{deviceId}
        private static string GetDeviceProperties(string baseURI, string deviceId, string token)
        {
            RestClient client = new RestClient(baseURI);
            var request = new RestRequest($"api/preview/devices/{deviceId.ToLower()}/properties", Method.GET);
            request.AddParameter("Authorization", token, ParameterType.HttpHeader);
            var response = client.Execute(request);

            if (response.IsSuccessful)
            {
                return response.Content;
            }

            return string.Empty;
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
            var request = new RestRequest($"/v2/{relativeUrl}", Method.GET);
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
        /// Check to see if user input Json is valid
        /// </summary>
        /// <param name="strInput"></param>
        /// <returns></returns>
        private static bool IsValidJson(string strInput)
        {
            if (string.IsNullOrWhiteSpace(strInput)) { return false; }
            strInput = strInput.Trim();
            if ((strInput.StartsWith("{") && strInput.EndsWith("}")) || //For object
                (strInput.StartsWith("[") && strInput.EndsWith("]"))) //For array
            {
                try
                {
                    var obj = JToken.Parse(strInput);
                    return true;
                }
                catch (JsonReaderException jex)
                {
                    return false;
                }
                catch (Exception ex) //some other exception
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
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
