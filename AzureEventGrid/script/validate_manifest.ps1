# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
param(
    [Parameter(Mandatory=$True)][string] $Manifest
)

$ret=0
$deviceAuthPlaceholder="00000000-0000-0000-0000-000000000000"
$eventGridPlaceholder="<your_event_grid_mqtt_hostname>"

function Test-CmdArgs {
    param(
        $cmdArgs
    )
    
    $errors = @()

    if (-not $cmdArgs.Contains("--Hostname")) {
        $errors += "Error: The hostname of your Azure Event Grid must be set in the 'CmdArgs' field of your app_manifest.json: `"--Hostname`",`"<your_event_grid_mqtt_hostname>`"" 
    }

    if ($cmdArgs.Contains($eventGridPlaceholder)) {
        $errors += "Error: Replace `"<your_event_grid_mqtt_hostname>`" with the value of your Azure Event Grid MQTT hostname." 
    }

    return $errors
}

function Test-AllowedConnections {
    param(
        $allowedConnections
    )

    $errors = @()

    if (($allowedConnections -eq $null) -or 
        ($allowedConnections.Count -eq 0) -or
        ($allowedConnections -eq $eventGridPlaceholder)) {
        $errors += "Error: The 'AllowedConnections' field of your app_manifest.json must contain the address of your Azure Event Grid."
    }

    return $errors
}

Write-Output "Validating project $($Manifest)"

$manifestPreset = Test-Path $Manifest

if (-not $manifestPreset) {
    Write-Output "Error: Cannot find the app_manifest.json at $($Manifest)"
    return 1
}

$json = Get-Content $Manifest -Raw
$jsonObj = ConvertFrom-Json -InputObject $json

$cmdArgs = $jsonobj.CmdArgs
Write-Host $cmdArgs
if ($cmdArgs -eq $null -or $cmdArgs.Count -eq 0) {
    Write-Output "Error: The 'CmdArgs' field in your app_manifest.json must be set."
    $ret=1
}

$cmdArgsErrors = Test-CmdArgs $cmdArgs

if ($cmdArgsErrors.Count -gt 0) {
    Write-Output $cmdArgsErrors
    $ret = 1
}

$allowedConnectionsErrors = Test-AllowedConnections $jsonobj.Capabilities.AllowedConnections

if ($allowedConnectionsErrors.Count -gt 0) {
    Write-Output $allowedConnectionsErrors
    $ret = 1
}

$deviceAuth=$jsonobj.Capabilities.DeviceAuthentication
if ($deviceAuth -eq $null -or $deviceAuth -eq $deviceAuthPlaceholder) {
    Write-Output "Error: The 'DeviceAuthentication' field in your app_manifest.json must be set to your Azure Sphere catalog ID. This can be obtained using the 'az sphere catalog show command' in the command prompt."
    $ret=1
}

if ($ret -eq 0) {
    Write-Output "app_manifest.json parameters exist."
}

exit $ret
