/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

namespace GetDeviceOsVersion
{
    // List of public OS versions/components
    public class PublishedOSList
    {
        public Version[] versions { get; set; }
    }
    public class Version
    {
        public string name { get; set; }
        public Images[] images { get; set; }
    }
    public class Images
    {
        public string cid { get; set; }
        public string iid { get; set; }
    }

    // List of device components
    public class DeviceImages
    {
        public bool is_ota_update_in_progress { get; set; }
        public bool has_staged_updates { get; set; }
        public bool restart_required { get; set; }
        public Component[] components { get; set; }
    }

    public class Component
    {
        public string uid { get; set; }
        public int image_type { get; set; }
        public bool is_update_staged { get; set; }
        public bool does_image_type_require_restart { get; set; }
        public DeviceImage[] images { get; set; }
        public string name { get; set; }
    }

    public class DeviceImage
    {
        public string uid { get; set; }
        public int length_in_bytes { get; set; }
        public int uncompressed_length_in_bytes { get; set; }
        public int replica_type { get; set; }
    }

    // Similar to DeviceImage
    public class DeviceInfo
    {
        public string IpAddress { get; set; }
        public string ConnectionPath { get; set; }
    }
}
