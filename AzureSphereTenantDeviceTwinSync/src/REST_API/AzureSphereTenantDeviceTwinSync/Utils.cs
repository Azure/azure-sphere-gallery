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
        ///  AS3 Endpoint to obtain OS Image and Component IDs
        /// </summary>
        private const string AzureSphereOSVersions = "https://prod.releases.sphere.azure.net/versions/mt3620an.json";


        /// <summary>
        /// Attempts to get a Value from Keyvault based on a given key.
        /// </summary>
        public static string GetKeyVaultString(string Key)
        {
            var client = new SecretClient(new Uri(keyVaultUrl), new DefaultAzureCredential());
            KeyVaultSecret secret=client.GetSecret(Key);
            
            return secret.Value;
        }

        /// <summary>
        /// Azure Sphere Public API URI
        /// </summary>
        private const string AzureSphereApiUri = "https://prod.core.sphere.azure.net";

        public static string GetAS3Data(string EndpointUrl, string token)
        {
            var client = new RestClient(AzureSphereApiUri);
            var request = new RestRequest($"/v2/{EndpointUrl}", Method.GET);
            request.AddParameter("Authorization", string.Format("Bearer " + token), ParameterType.HttpHeader);
            var response = client.Execute(request);

            if (response.IsSuccessful)
            {
                return response.Content;
            }
            return string.Empty;
        }

        public static string GetOSVersionForDevice(string deviceId, string UserTenantId, string token)
        {
            string retVal = string.Empty;
            // Get Azure Sphere OS Versions.
            string osVersionsJson = GetAS3Data(AzureSphereOSVersions, string.Empty);
            OSVersions osVersions = JsonConvert.DeserializeObject<OSVersions>(osVersionsJson);
            if (osVersions.versions.Length == 0)
            {
                Debug.WriteLine("Could not get Azure Sphere OS Versions");
                return retVal;
            }

            string deviceImageList = GetAS3Data($"tenants/{UserTenantId}/devices/{deviceId}/images", token);
            Debug.WriteLine("Device Image List:");
            Debug.WriteLine(deviceImageList);
            DeviceImages deviceImages = JsonConvert.DeserializeObject<DeviceImages>(deviceImageList);
            if (deviceImages.Items.Length == 0)
            {
                Debug.WriteLine("Failed to get the device targeted image list");
                return retVal;
            }

            // if we get here then we have...
            // Device ID
            // Device Tenant ID
            // OS Versions List
            // device targeted image list

            // Get the first targeted Id (Image Id) from deviceImageList
            string imageId = deviceImages.Items[0].Id;

            bool found = false;
            string versionName = string.Empty;
            // now walk the osVersions list in reverse order to find the matching image Id
            for (int x = osVersions.versions.Length - 1; x > -1; x--)
            {
                Version version = osVersions.versions[x];
                for (int y = 0; y < version.images.Length; y++)
                {
                    if (imageId == version.images[y].iid)
                    {
                        found = true;
                        versionName = version.name;
                        break;
                    }
                }

                if (found == true)
                {
                    break;
                }
            }

            if (found == true)
            {
                Debug.WriteLine("OS Version Info:");
                Debug.WriteLine($"Device ID:     {deviceId}");
                Debug.WriteLine($"Targeted with: {versionName}");
                retVal = versionName;
            }

            return retVal;
        }


    }

    // Device Image List

    public class DeviceImages
    {
        public cItem[] Items { get; set; }
        public object ContinuationToken { get; set; }
    }

    public class cItem
    {
        public string Id { get; set; }
        public string Name { get; set; }
        public string ComponentId { get; set; }
        public object Description { get; set; }
        public int ImageType { get; set; }
        public int Type { get; set; }
    }

    // OS Version info.

    public class OSVersions
    {
        public Version[] versions { get; set; }
    }

    public class Version
    {
        public string name { get; set; }
        public Image[] images { get; set; }
    }

    public class Image
    {
        public string cid { get; set; }
        public string iid { get; set; }
    }
}
