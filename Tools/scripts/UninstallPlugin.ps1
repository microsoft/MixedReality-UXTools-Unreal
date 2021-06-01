<#
.SYNOPSIS
    Uninstall a plugin from the engine.
    Should only be used to remove plugins installed using `InstallPlugin.ps1`

.PARAMETER UnrealEngine
    Path to Unreal Engine (root folder).

.PARAMETER PluginName
    The name of the plugin to uninstall.
#>
param
(
    [Parameter(Mandatory=$true)]
    [string]$UnrealEngine,
    [Parameter(Mandatory=$true)]
    [string]$PluginName
)

$PluginsPath = "$UnrealEngine\Engine\Plugins"
if ((-not (Test-Path -Path $UnrealEngine -PathType Container)) -or
    (-not (Test-Path -Path $PluginsPath -PathType Container)))
{
    Write-Host -ForegroundColor Red "Incorrect UnrealEngine path provided: $UnrealEngine"
    Write-Host -ForegroundColor Red "UnrealEngine parameter should point to the root installation folder of Unreal Engine"
    throw "Unreal Engine not found"
}

$MarketplacePath = "$PluginsPath\Marketplace"
$PluginPath = "$MarketplacePath\$PluginName"
if (-not (Test-Path -Path $PluginPath -PathType Container))
{
    Write-Host -ForegroundColor Red "Invalid plugin provided: $UnrealEngine"
    Write-Host -ForegroundColor Red "The plugin is not installed."
    throw "Plugin not found"
}

Write-Host "Uninstalling $PluginName from '$MarketplacePath'..."
Remove-Item -Path $PluginPath -Recurse

$Result = 0
if ($?)
{
    Write-Host "Successfully uninstalled $PluginName."
}
else
{
    Write-Host -ForegroundColor Red "Failed to uninstall $PluginName."
    $Result = 1
}

exit $Result
