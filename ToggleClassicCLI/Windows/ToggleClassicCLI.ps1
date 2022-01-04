<#
# --------------------------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# This file is licensed under the MIT License.
#
# cliv1
#
# Powershell script to Disable/Enable Azure Sphere Classic CLI
# --------------------------------------------------------------------------------------------
#>

Write-Host 'Enable/Disable Azure Sphere Classic CLI'

$sdkDefaultPath='C:\Program Files (x86)\Microsoft Azure Sphere SDK\'
$cliV1DefaultPath=$sdkDefaultPath+'Tools'
$cliV1HiddenPath=$sdkDefaultPath+'.Tools'
$cliV2DefaultPath=$sdkDefaultPath+'Tools_v2\'
$cliV1DeveloperShortcutPath='C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Azure Sphere\Azure Sphere Classic Developer Command Prompt (Deprecated).lnk'
$cliV1RenamedShortcut=$sdkDefaultPath+'cmdshortcut.lnk'

if (-not (Test-Path -Path $sdkDefaultPath -PathType container))
{
    Write-Host 'Azure Sphere SDK not found'  
    Exit 1  
}

if (-not (Test-Path -Path $cliV2DefaultPath -PathType container))
{
    Write-Host 'Azure Sphere CLIv2 not found'
    Write-Host 'The latest SDK can be downloaded from: https://aka.ms/AzureSphereSDKDownload/Windows'
    Exit 1  
}

Write-Host "Checking for elevated permissions..."
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(`
[Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Warning "Insufficient permissions to run this script. Open the PowerShell console as an administrator and run this script again."
Break
}

if (Test-Path -Path $cliV1DefaultPath -PathType Container ) 
{ 
    Write-Host 'Azure Sphere Classic CLI is currently enabled'
    Write-Host 'Removing Azure Sphere Classic CLI'
    # Azure Sphere Classic CLI folder exists.
    # Rename Tools folder to '.Tools' and set directory attributes to hide the directory
    [System.IO.Directory]::Move($cliV1DefaultPath,$cliV1HiddenPath)

    # set folder hidden attribute
    $toolsDirectory=Get-Item $cliV1HiddenPath -Force
    $toolsDirectory.Attributes=$toolsDirectory.Attributes -bor "Hidden"

    # now hide the command line shortcut
    if (Test-Path -Path $cliV1DeveloperShortcutPath -PathType Leaf)
    {
        # the shortcut exists on the start menu, check whether the copy exists in the SDK folder
        if (Test-Path -Path $cliV1RenamedShortcut -PathType Leaf)
        {
            (get-item $cliV1RenamedShortcut -Force).Attributes-="Hidden"
            Remove-Item -Path $cliV1RenamedShortcut
        }

        # copy and rename the shortcut
        Copy-Item -Path $cliV1DeveloperShortcutPath -Destination $cliV1RenamedShortcut
        (get-item $cliV1RenamedShortcut).Attributes+="Hidden"
        Remove-Item -Path $cliV1DeveloperShortcutPath
    }
}
else
{
    # Check to see whether the 'hidden' Tools folder exists
    if (Test-Path -Path $cliV1HiddenPath -PathType Container ) 
    {
        Write-Host 'Azure Sphere Classic CLI is currently disabled'
        Write-Host 'Restoring Azure Sphere Classic CLI'
        [System.IO.Directory]::Move($cliV1HiddenPath,$cliV1DefaultPath)
        $toolsDirectory=Get-Item $cliV1DefaultPath -Force
        $toolsDirectory.Attributes=$toolsDirectory.Attributes -bxor "Hidden"

        # now restore the command line shortcut
        if (Test-Path -Path $cliV1RenamedShortcut -PathType Leaf)
        {
            # the shortcut exists on in the SDK folder, check whether the link exists in the start menu folder
            if (Test-Path -Path $cliV1DeveloperShortcutPath -PathType Leaf)
            {
                (get-item $cliV1DeveloperShortcutPath -Force).Attributes-="Hidden"
                Remove-Item -Path $cliV1DeveloperShortcutPath
            }

            # copy and rename the shortcut
            Copy-Item -Path $cliV1RenamedShortcut -Destination $cliV1DeveloperShortcutPath
            (get-item $cliV1DeveloperShortcutPath -Force).Attributes-="Hidden"

            # remove the copy
            (get-item $cliV1RenamedShortcut -Force).Attributes-="Hidden"
            Remove-Item -Path $cliV1RenamedShortcut
        }
    }
}

Write-Host 'Done'
