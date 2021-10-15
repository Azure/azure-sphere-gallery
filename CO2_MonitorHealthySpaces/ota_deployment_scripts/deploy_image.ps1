$image_path_filename = "../src/out/ARM-Release/co2monitor.imagepackage"
$device_group = "CO2Monitor/Field Test"

Write-Output "`nRun this script from the ota_deployment_scripts folder`n"

# This is an example of the out from the azsphere image add command that will be parsed below for the image id

Write-Output "`nSelected tenant"

azsphere tenant show-selected

Write-Output "`nUploading image`n"

$upload_image = azsphere image add --image $image_path_filename

Write-Output $upload_image

$i = $upload_image.Split(">").Trim()

# This is where you'll find the image id in the image upload return string
$image_id = $i[2].split(":")[1].Trim()

Write-Output "`nCreating deployment for device group id: '$device_group' for image id: $image_id"

azsphere device-group deployment create --device-group $device_group --images $image_id

write-host "`nImage details for image id: $image_id"

azsphere image show -i $image_id

Write-Output "`nList of all images for device group id: $device_group"

azsphere device-group deployment list --device-group $device_group