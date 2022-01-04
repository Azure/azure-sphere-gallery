$device_group = "CO2Monitor/Field Test"

Write-Output "`nListing images for device group id: $device_group`n"

azsphere device-group deployment list --device-group $device_group
