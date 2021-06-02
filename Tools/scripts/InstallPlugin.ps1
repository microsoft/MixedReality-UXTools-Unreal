<#
.SYNOPSIS
    Install a plugin into the engine to allow dependent plugins to be packaged.
    The plugin is installed as a marketplace plugin.

.PARAMETER UnrealEngine
    Path to Unreal Engine (root folder).

.PARAMETER PluginPath
    Path to the packaged plugin to install.
#>
param
(
    [Parameter(Mandatory=$true)]
    [string]$UnrealEngine,
    [Parameter(Mandatory=$true)]
    [string]$PluginPath
)

$PluginsPath = "$UnrealEngine\Engine\Plugins"
if ((-not (Test-Path -Path $UnrealEngine -PathType Container)) -or
    (-not (Test-Path -Path $PluginsPath -PathType Container)))
{
    Write-Host -ForegroundColor Red "Incorrect UnrealEngine path provided: $UnrealEngine"
    Write-Host -ForegroundColor Red "UnrealEngine parameter should point to the root installation folder of Unreal Engine"
    throw "Unreal Engine not found"
}

$PluginName = Split-Path $PluginPath -Leaf
if ((-not (Test-Path -Path $PluginPath -PathType Container)) -or
    (-not (Test-Path -Path "$PluginPath\$PluginName.uplugin" -PathType Leaf)))
{
    Write-Host -ForegroundColor Red "Invalid plugin path provided: $PluginPath"
    Write-Host -ForegroundColor Red "PluginPath parameter should point to the root folder of the plugin"
    throw "Plugin not found"
}

$MarketplacePath = "$PluginsPath\Marketplace"
$InstallPath = "$MarketplacePath\$PluginName"
if (Test-Path -Path $InstallPath -PathType Container)
{
    Write-Host "Removing existing version of '$PluginName'..."
    Remove-Item -Path $InstallPath -Recurse
}

Write-Host "Installing '$PluginPath' into '$MarketplacePath'..."
Copy-Item -Path $PluginPath -Destination $InstallPath -Recurse

$Result = 0
if ($?)
{
    Write-Host "Successfully installed $PluginName."
}
else
{
    Write-Host -ForegroundColor Red "Failed to install $PluginName."
    $Result = 1
}

exit $Result
