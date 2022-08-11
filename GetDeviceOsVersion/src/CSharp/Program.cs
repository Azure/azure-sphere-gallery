/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using System.Text.Json;
using Microsoft.Azure.Sphere.DeviceAPI;

namespace GetDeviceOsVersion
{
    /// <summary>
    /// A class containing the main functionality of the program.
    /// </summary>
    class Program
    {
        /// <summary>
        /// The main entry point of the program that finds the OS version.
        /// </summary>
        static void Main()
        {
            // Get the list of attached devices IP addresses
            List<DeviceInfo> devices = JsonSerializer.Deserialize<List<DeviceInfo>>(Devices.GetAttachedDevices());

            // Get the list of Azure Sphere published OS images
            Version[] versions = GetPublishedOSList();

            foreach (DeviceInfo device in devices)
            {
                string OSVersion = GetOSVersion(versions, GetDeviceImages(GetDeviceImageList(device.IpAddress)));
                Console.WriteLine($"Device IP = {device.IpAddress}, OS version = {OSVersion}");
            }
        }

        /// <summary>
        /// Get the Azure Sphere OS Versions and their Image and Component IDs.
        /// </summary>
        /// <returns>An array of OS versions and their Image and Component IDs.</returns>
        private static Version[] GetPublishedOSList()
        {
            // Get the json containing the list of image and component IDs for each OS version.
            string response = GetRequest("https://prod.releases.sphere.azure.net/versions/mt3620an.json");
            PublishedOSList osList = JsonSerializer.Deserialize<PublishedOSList>(response);

            return osList.versions;
        }

        /// <summary>
        /// Gets a list of components installed on the device.
        /// </summary>
        /// <param name="deviceIp">The IP address of the device you would like to retrieve the image list from.</param>
        /// <returns>The list of components running on the device.</returns>
        private static Component[] GetDeviceImageList(string deviceIp)
        {
            Devices.SetActiveDeviceIpAddress(deviceIp);
            string response = Image.GetImages();
            DeviceImages deviceImages = JsonSerializer.Deserialize<DeviceImages>(response);

            return deviceImages.components;
        }

        /// <summary>
        /// Get a list of images on the device that are not applications.
        /// </summary>
        /// <param name="components">The array of components you would like to retrieve the device images from.</param>
        /// <returns>The images with any applications filtered out.</returns>
        private static Dictionary<string, string> GetDeviceImages(Component[] components)
        {
            Dictionary<string, string> images = new();
            foreach (Component component in components)
            {
                // Image types https://docs.microsoft.com/rest/api/azure-sphere/image/get-metadata
                if (component.image_type != 10)
                {
                    // Not an app.
                    images.Add(component.name, component.uid);
                }
            }

            return images;
        }

        /// <summary>
        /// Gets the current OS Version for the device by matching device image IDs to the published OS Image IDs
        /// </summary>
        /// <param name="version">The OS versions you are comparing the device against.</param>
        /// <param name="deviceImages">The device images running on your device.</param>
        /// <returns>The OS version your device is running if found, otherwise an empty string.</returns>
        private static string GetOSVersion(Version[] version, Dictionary<string, string> deviceImages)
        {
            // walk backwards up the OS image list
            for (int x = version.Length - 1; x > -1; x--)
            {
                for (int y = 0; y < version[x].images.Length; y++)
                {
                    for (int z = 0; z < deviceImages.Count; z++)
                    {
                        if (version[x].images[y].cid == deviceImages[deviceImages.Keys.ToList()[z]])
                        {
                            return version[x].name;
                        }
                    }
                }
            }

            return "Unknown";
        }

        /// <summary>
        /// Makes a "GET" request to the given address.
        /// </summary>
        /// <param name="address">The address to make the "GET" request to.</param>
        /// <returns>The content of the response, as a string.</returns>
        private static string GetRequest(string address)
        {
            HttpResponseMessage response = new HttpClient().GetAsync(address).GetAwaiter().GetResult();

            return response.Content.ReadAsStringAsync().GetAwaiter().GetResult();
        }
    }
}
