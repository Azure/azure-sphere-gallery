/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.AspNetCore.Mvc;
using Newtonsoft.Json;
using System.Diagnostics;
using System.Threading.Tasks;

namespace WebHookPublicAPIServicePrincipal.Controllers
{
    [Route("[controller]")]
    [ApiController]
    public class Root : ControllerBase
    {
        [HttpGet]
        public string Get()
        {
            return $"API Service is running...";
        }
    }

    [Route("[controller]")]
    [ApiController]
    public class WebHook : ControllerBase
    {
        /// <summary>
        /// Users Azure Sphere Tenant Id
        /// </summary>
        private static string AzureSphereTenantId = "";     // TODO: add your Azure Sphere Tenant Id (azsphere tenant show-selected)

        // GET: used to initialize the Token process
        [HttpGet]
        public async Task<string> Get([FromQuery] string DeviceId = "")
        {
            if (string.IsNullOrEmpty(DeviceId))
                return "Bad Request";

            // Get Service Principal Token.
            string token = await Utils.GetAS3Token();

            // Get some basic information for the device in the Azure Sphere tenant.
            string deviceData = Utils.GetData($"tenants/{AzureSphereTenantId}/devices/{DeviceId}", token);
            Debug.WriteLine(deviceData);

            if (string.IsNullOrEmpty(deviceData))
            {
                return "Bad Request";
            }

            DeviceInfo devInfo = JsonConvert.DeserializeObject<DeviceInfo>(deviceData);

            // https://prod.core.sphere.azure.net/v2/tenants/{tenantId}/devicegroups/{deviceGroupId}
            string deviceGroup = Utils.GetData($"tenants/{AzureSphereTenantId}/devicegroups/{devInfo.DeviceGroupId}", token);
            Debug.WriteLine(deviceGroup);
            DeviceGroupInfo dgInfo = JsonConvert.DeserializeObject<DeviceGroupInfo>(deviceGroup);

            // https://prod.core.sphere.azure.net/v2/tenants/{tenantId}/products/{productId}
            string product = Utils.GetData($"tenants/{AzureSphereTenantId}/products/{devInfo.ProductId}", token);
            Debug.WriteLine(product);
            ProductInfo prodInfo = JsonConvert.DeserializeObject<ProductInfo>(product);

            string[] AppUpdatePolicies = new string[] { "Update All", "No 3rd Party App Updates", "No Updates" };

            string ret = $"Device           : {DeviceId}\n" +
                $"Product          : {prodInfo.Name}\n" +
                $"Device Group     : {dgInfo.Name}\n" +
                $"Retail Eval      : {dgInfo.OsFeedType}\n" +
                $"App Update Policy: {AppUpdatePolicies[dgInfo.UpdatePolicy]}";

            Debug.WriteLine(ret);

            return ret;
        }
    }
}

