using Microsoft.AspNetCore.Mvc;
using Microsoft.Azure.Devices;
using Microsoft.Identity.Client;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Configuration;
using Microsoft.Azure.Devices.Shared;

namespace DevicePropertiesWebHook.Controllers
{
    [Route("[controller]")]
    [ApiController]
    public class Root : ControllerBase
    {
        private IConfiguration _configuration;

        public Root(IConfiguration configuration)
        {
            _configuration = configuration;
        }

        [HttpGet]
        public string Get()
        {
            bool useDeviceCodeFlow = _configuration.GetValue<bool>("MySettings:UseDeviceCodeFlow");
            if (useDeviceCodeFlow)
            {
                string authStatus = ADToken.isAuthComplete() ? "Auth is complete" : "Auth has not completed";
                return $"API Service is running and using Device Code Authentication... {authStatus}";
            }
            else
            {
                return "API Service is running and using Managed Identity...";
            }
        }
    }

    [Route("[controller]")]
    [ApiController]
    public class WebHook : ControllerBase
    {
        /// <summary>
        /// Users Azure Sphere Tenant Id
        /// </summary>
        private static string AzureSphereTenantId = "";
        /// <summary>
        /// IoT Hub 'Owner' connection string (replace with KeyVault key)
        /// </summary>
        private static string IoTHubConnectionString = "";
        /// <summary>
        /// API Key used for service validation 
        /// </summary>
        /// <returns></returns>
        private static string APIKey = "";

        /// <summary>
        /// List of App Update policies for a device group
        /// </summary>
        private static string[] AppUpdatePolicies = new string[] { "Update All", "No 3rd Party App Updates", "No Updates" };

        private IConfiguration _configuration;

        public WebHook(IConfiguration configuration)
        {
            _configuration = configuration;
        }

        // GET: used to initialize the Token process
        [HttpGet]
        public string Get([FromQuery] string UserAPIKey = "")
        {
            if (string.IsNullOrEmpty(UserAPIKey))
                return "Bad Request";

            Debug.WriteLine($"API Key = {UserAPIKey}");

            if (!GetSecrets())
            {
                return "Bad Request";
            }

            if (!APIKey.Equals(UserAPIKey))
            {
                return "Bad Request";
            }

            if (string.IsNullOrEmpty(APIKey) || string.IsNullOrEmpty(IoTHubConnectionString) || string.IsNullOrEmpty(AzureSphereTenantId))
            {
                return "Bad Request";
            }

            // If we get here then we have a valid API Key from the user and
            // also have the user 'Azure Sphere Tenant Id' and IoTHub Connection String

            // Initialize IPublicClientApplication
            bool useDeviceCodeFlow = _configuration.GetValue<bool>("MySettings:UseDeviceCodeFlow");

            if (useDeviceCodeFlow)
            {
                ADToken.InitPCA();
                ManualResetEvent oSignalEvent = new ManualResetEvent(false);

                ADToken.InitDeviceFlow(oSignalEvent, AzureSphereTenantId);

                oSignalEvent.WaitOne(); //This thread will block here until the reset event is sent.
                oSignalEvent.Reset();

                // GetDeviceFlowInstructions might be null if the flow wasn't completed.
                string instructions = ADToken.GetDeviceFlowInstructions();

                // Call ADToken function that wraps the token code in a thread
                // wait on ManualResetEvent to get user string.

                // normal operation will just call the code to get the token (without the thread)

                return instructions;
            }

            return "API Service is using Managed Identity";
        }

        // POST: the webhook endpoint called from EventGrid
        [HttpPost]
        public async Task<IActionResult> Post(JArray data)
        {
            Debug.WriteLine($"\nPOST Received:\n{data}");
            Debug.WriteLine("");
            string response = string.Empty;

            // see if this is an EventGrid Validation message.
            string validationCode = string.Empty;
            JToken validationObj = data[0]["data"]["validationCode"];
            if (validationObj != null)
            {
                validationCode = validationObj.ToString();
            }

            if (!string.IsNullOrEmpty(validationCode))
            {
                response = $"{{ 'validationResponse': '{validationCode}'}}";
                Debug.WriteLine($"EventGrid Validation Response: {response}");
                return Ok(response);
            }
            else
            {
                if (!GetSecrets())
                {
                    return Ok("Request Failed");
                }
                // iothub-connection-device-id
                string deviceId = data[0]["data"]["systemProperties"]["iothub-connection-device-id"].ToString();
                Debug.WriteLine($"Device ID: {deviceId}");

                JToken dataBody = data[0]["data"]["body"];
                if (dataBody != null)
                {
                    string jsonBody = data[0]["data"]["body"].ToString();
                    JObject jBody = JObject.Parse(jsonBody);
                    JProperty noUpdate = jBody.Properties().FirstOrDefault(p => p.Name.Contains("NoUpdateAvailable"));
                    JProperty appRestart = jBody.Properties().FirstOrDefault(p => p.Name.Contains("AppRestart"));

                    // If the device reports no update available, this is because it has just checked with AS3 for updates, so the AS3 device data (e.g.the current OS) is up to date.
                    // An app restart may also indicate that a software update happened, and including this facilitates testing as well as it is an easy event to trigger.
                    if (noUpdate != null || appRestart != null)
                    {
                        string token = string.Empty;

                        // get the access token.
                        bool useDeviceCodeFlow = _configuration.GetValue<bool>("MySettings:UseDeviceCodeFlow");
                        if (useDeviceCodeFlow)
                        {
                            if (ADToken.isAuthComplete())
                            {
                                Debug.WriteLine("AS3 device info...");
                                AuthenticationResult result = await ADToken.GetAToken();
                                if (result != null)
                                {
                                    token = result.AccessToken;
                                }
                            }
                        }
                        else
                        {
                            token = await Utils.GetAS3Token();
                        }

                        if (string.IsNullOrEmpty(token))
                        {
                            return Ok("No Token");
                        }

                        TwinState state = GetDeviceTwinDataFromAS3(token, deviceId);

                        // Get current twin data for 'deviceId'
                        RegistryManager registryManager = RegistryManager.CreateFromConnectionString(IoTHubConnectionString);
                        var twin = await registryManager.GetTwinAsync(deviceId);

                        string _desiredJson = twin.Properties.Desired.ToJson();
                        // anything changed?
                        if (!IsDeviceTwinUpdateNeeded(_desiredJson, state))
                        {
                            return Ok();
                        }

                        var patch = $"{{ properties: {{ desired: {JsonConvert.SerializeObject(state) } }} }}";
                        await registryManager.UpdateTwinAsync(twin.DeviceId, patch, twin.ETag);
                    }
                }

                return Ok();
            }
        }

        static TwinState GetDeviceTwinDataFromAS3(string token, string deviceId)
        {
            string deviceData = Utils.GetAS3Data($"tenants/{AzureSphereTenantId}/devices/{deviceId}", token);
            DeviceInfo devInfo = JsonConvert.DeserializeObject<DeviceInfo>(deviceData);

            // https://prod.core.sphere.azure.net/v2/tenants/{tenantId}/devicegroups/{deviceGroupId}
            string deviceGroup = Utils.GetAS3Data($"tenants/{AzureSphereTenantId}/devicegroups/{devInfo.DeviceGroupId}", token);
            Debug.WriteLine(deviceGroup);
            DeviceGroupInfo dgInfo = JsonConvert.DeserializeObject<DeviceGroupInfo>(deviceGroup);

            // https://prod.core.sphere.azure.net/v2/tenants/{tenantId}/products/{productId}
            string product = Utils.GetAS3Data($"tenants/{AzureSphereTenantId}/products/{devInfo.ProductId}", token);
            Debug.WriteLine(product);
            ProductInfo prodInfo = JsonConvert.DeserializeObject<ProductInfo>(product);

            TwinState state = new TwinState
            {
                Product = prodInfo.Name,
                DeviceGroup = dgInfo.Name,
                RetailEval = dgInfo.OsFeedType == 1 ? true : false,
                AppUpdatePolicy = AppUpdatePolicies[dgInfo.UpdatePolicy],
                OSVersion = string.IsNullOrEmpty(devInfo.LastInstalledOSVersion) ? "None" : devInfo.LastInstalledOSVersion
            };

            Debug.WriteLine($"Device                    : {deviceId}");
            Debug.WriteLine($"Product                   : {state.Product}");
            Debug.WriteLine($"Device Group              : {state.DeviceGroup}");
            Debug.WriteLine($"Retail Eval               : {state.RetailEval}");
            Debug.WriteLine($"App Update Policy         : {state.AppUpdatePolicy}");
            Debug.WriteLine($"Last Installed OS Version : {state.OSVersion}");

            return state;
        }

        static bool IsDeviceTwinUpdateNeeded(string twinJson, TwinState state)
        {
            JObject _desired = JObject.Parse(twinJson);

            string twinDesiredOS = (string)_desired["OSVersion"];
            if (string.IsNullOrEmpty(twinDesiredOS) || twinDesiredOS != state.OSVersion)
            {
                Debug.WriteLine("No OS Version, or OS Version doesn't match");
                return true;
            }

            string twinDesiredProduct = (string)_desired["Product"];
            if (string.IsNullOrEmpty(twinDesiredProduct) || twinDesiredProduct != state.Product)
            {
                Debug.WriteLine("No Product, or product names don't match");
                return true;
            }

            string twinDeviceGroup = (string)_desired["DeviceGroup"];
            if (string.IsNullOrEmpty(twinDeviceGroup) || twinDeviceGroup != state.DeviceGroup)
            {
                Debug.WriteLine("No Device Group, or Device Group names don't match");
                return true;
            }

            if (!_desired.ContainsKey("RetailEval"))
            {
                Debug.WriteLine("RetailEval not found in JSON");
                return true;
            }

            bool twinRetailEval = (bool)_desired["RetailEval"];
            if (twinRetailEval != state.RetailEval)
            {
                Debug.WriteLine("RetailEval doesn't match");
                return true;
            }

            string twinUpdtePolicy = (string)_desired["AppUpdatePolicy"];
            if (string.IsNullOrEmpty(twinUpdtePolicy) || twinUpdtePolicy != state.AppUpdatePolicy)
            {
                Debug.WriteLine("No App Update Policy, or App Update Policy names don't match");
                return true;
            }

            return false;
        }

        private bool GetSecrets()
        {
            APIKey = Utils.GetKeyVaultString("APIKey");
            IoTHubConnectionString = Utils.GetKeyVaultString("IoTHubConnectionString");
            AzureSphereTenantId = Utils.GetKeyVaultString("tenantId");

            if (string.IsNullOrEmpty(APIKey) || string.IsNullOrEmpty(IoTHubConnectionString) || string.IsNullOrEmpty(AzureSphereTenantId))
            {
                return false;
            }

            return true;
        }
    }

    /// <summary>
    /// Used for serialization of the device info into Device Twin. 
    /// </summary>
    public class TwinState
    {
        public string OSVersion { get; set; }
        public string Product { get; set; }
        public string DeviceGroup { get; set; }
        public bool RetailEval { get; set; }
        public string AppUpdatePolicy { get; set; }
    }

    /// <summary>
    /// Used to obtain a devices product and device group
    /// </summary>
    public class DeviceData
    {
        public string DeviceId { get; set; }
        public string TenantId { get; set; }
        public string ProductId { get; set; }
        public string DeviceGroupId { get; set; }
        public int ChipSku { get; set; }
    }

    public class DeviceInfo
    {
        public string DeviceId { get; set; }
        public string TenantId { get; set; }
        public string ProductId { get; set; }
        public string DeviceGroupId { get; set; }
        public int ChipSku { get; set; }
        public string LastInstalledOSVersion { get; set; }
        public string LastAvailableOSVersion { get; set; }
        public object LastUpdateCheckUTC { get; set; }
        public object LastOsUpdateUTC { get; set; }
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
