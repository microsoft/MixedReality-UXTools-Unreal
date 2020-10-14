<#
.SYNOPSIS
    Utility functions for building of Unreal projects.
#>

<#
.SYNOPSIS
    Create copies of files (if those files already exist).

.PARAMETER Files
    List of paths.

.OUTPUTS
    {string:string} - dictionary with new paths as keys, old paths as values.
#>
function Backup-Files
{
    [CmdletBinding()]
    param (
        [string[]]$Files
    )
    process
    {
        $Result = @{}
        $UniqueTimestamp = (Get-Date).Ticks
        $Files | ForEach-Object {
            if (Test-Path -Path $_ -PathType Leaf)
            {
                $NewFileName = "$($_).$UniqueTimestamp"
                $Result[$NewFileName] = $_
                Write-Host "Back up $_ => $NewFileName"
                Rename-Item -Path $_ -NewName $NewFileName -Force -ErrorAction Stop
            }
        }
        return $Result
    }
}

<#
.SYNOPSIS
    Restore files previously backed up with Backup-Files.

.PARAMETER Files
    {string:string} - dictionary with new paths as keys, old paths as values.
#>
function Restore-BackupFiles
{
    [CmdletBinding()]
    param (
        $FilePathDictionary
    )
    process
    {
        Write-Host "Restoring backed up files."
        $Errors = $False
        $FilePathDictionary.keys | ForEach-Object {
            $BackupFileName = $_
            $OriginalFilename = $FilePathDictionary[$BackupFileName]
            try
            {
                Write-Host "Restoring $BackupFileName => $OriginalFilename"
                Remove-Item $OriginalFilename -Force
                Rename-Item -Path $BackupFileName -NewName $OriginalFilename -Force
            }
            catch
            {
                $Errors = $True
                Write-Host -ForegroundColor Red  "Could not restore file: $OriginalFilename"
                Write-Host -ForegroundColor Red "   from backup file: $BackupFileName"
                Write-Host $_  # exception trace
            }
        }
        if ($Errors)
        {
            throw "Not all backed up files were restored."
        }
    }
}

<#
.SYNOPSIS
    Create Unreal Engine BuildConfiguration.xml file.
#>
function New-BuildConfiguration
{
    [CmdletBinding()]
    param (
        [string]$OutPath,
        [boolean]$UseStaticAnalyzer = $True,
        [boolean]$UseUnityBuild = $True
    )
    process
    {
        Write-Host "Writing new Unreal Engine Build Configuration to: $OutPath"
        $Lines = @()
        $Lines += "<?xml version=`"1.0`" encoding=`"utf-8`" ?>"
        $Lines += "<Configuration xmlns=`"https://www.unrealengine.com/BuildConfiguration`">"
        $Lines += "    <BuildConfiguration>"
        $Lines += "        <bUseUnityBuild>" + "$UseUnityBuild".ToLower() + "</bUseUnityBuild>"
        $Lines += "    </BuildConfiguration>"
        $Lines += "    <WindowsPlatform>"
        if ($UseStaticAnalyzer)
        {
            Write-Host "    StaticAnalyzer: VisualCpp"
            $Lines += "        <StaticAnalyzer>VisualCpp</StaticAnalyzer>"
        }
        $Lines += "    </WindowsPlatform>"
        $Lines += "</Configuration>"
        $Lines -join "`r`n" | Out-File -FilePath $OutPath -Encoding ascii
    }
}

function Get-UnrealBuildLogPath
{
    [CmdletBinding()]
    param (
        [Parameter(Mandatory=$true)]
        [string]$UnrealEngine
    )
    process
    {
        $EngineId = ($UnrealEngine -replace "\\","+" -replace ":","").TrimEnd("+")
        return "$($Env:APPDATA)\Unreal Engine\AutomationTool\Logs\$EngineId\Log.txt"
    }
}

# Example match:
# ProcessResult.StdOut:   R:/MixedReali.../UXTools/file.cpp(33): warning C4996: Warning message.
# Groups: 1 = file path; 2 = line [,column]; 3 = error/warning; 4 = code; 5 = message
$BUILD_EVENT_REGEX_MSBUILD = "([\w\d:/\\_\-\.]+)\(([\d,]+)\)\s*: (warning|error) ([A-Z\d]+){0,1}: (.+)"
# R:\MixedReali...\UXToolsGame\file.cpp(1): error: Expected file.h to be first header included.
# Groups: 1 = file path; 2 = line [,column]; 3 = error/warning; 4 = message
$BUILD_EVENT_REGEX_UNREAL = "([\w\d:/\\_\-\.]+)\(([\d,]+)\): (warning|error): (.+)"
$BUILD_EVENT_REGEX_GENERAL = "(WARNING|ERROR): (.+)"

<#
.SYNOPSIS
    Parse a line of output and report error/warning message to ADO pipeline.

.OUTPUTS
    [boolean] true if no error found (success)
#>
function Format-BuildError {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory=$true)]
        [AllowEmptyString()]
        [string]$Line,
        [boolean]$WarningsAsErrors = $True
    )
    process
    {
        if ([string]::IsNullOrEmpty($Line))
        {
            return $True
        }
        $IssueType = "warning"
        if ($Line -cmatch $BUILD_EVENT_REGEX_MSBUILD)
        {
            $SourcePath = $Matches[1] 
            $LineNumber = $Matches[2]
            $IssueType = $Matches[3]
            if ($WarningsAsErrors)
            {
                $IssueType = "error"
            }
            $Code = $Matches[4]
            $Message = $Matches[5]
            $Column = 1
            if ($LineNumber -contains ",")
            {
                $Line, $Column = $LineNumber -split ","
            }
            $Location = "linenumber=$LineNumber;columnnumber=$Column"
            Write-Host "##vso[task.logissue type=$IssueType;sourcepath=$SourcePath;$Location;code=$Code;]$Message"
        }
        elseif ($Line -cmatch $BUILD_EVENT_REGEX_UNREAL)
        {
            $SourcePath = $Matches[1] 
            $LineNumber = $Matches[2] 
            $IssueType = $Matches[3]
            if ($WarningsAsErrors)
            {
                $IssueType = "error"
            }
            $Message = $Matches[4]
            $Column = 1
            if ($LineNumber -contains ",")
            {
                $LineNumber, $Column = $LineNumber -split ","
            }
            $Location = "linenumber=$LineNumber;columnnumber=$Column"
            Write-Host "##vso[task.logissue type=$IssueType;sourcepath=$SourcePath;$Location;]$Message"
        }
        elseif ($Line -cmatch $BUILD_EVENT_REGEX_GENERAL)
        {
            $IssueType = $Matches[1].ToLower()
            if ($WarningsAsErrors)
            {
                $IssueType = "error"
            }
            $Message = $Matches[2]
            Write-Host "##vso[task.logissue type=$IssueType;]$Message"
        }
        return ($IssueType -ne "error")
    }
}

<#
.SYNOPSIS
    Parse build log for errors and warnings.

.OUTPUTS
    [boolean] $True is no erros found.
#>
function Read-UnrealBuildLog {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory=$true)]
        [string]$UnrealEngine,
        [boolean]$WarningsAsErrors = $True
    )
    process
    {
        $Success = $True

        $LogFilePath = (Get-UnrealBuildLogPath -UnrealEngine $UnrealEngine)
        if (-not (Test-Path -Path $LogFilePath -PathType Leaf))
        {
            throw "UAT log not found: $LogFilePath"
        }
        Write-Host "Reading UAT log: $LogFilePath"

        Get-Content -Path $LogFilePath -Encoding UTF8 | ForEach-Object {
            $Success = (Format-BuildError -Line $_) -and $Success
        }
        return $Success
    }
}

function Get-UATPath
{
    [CmdletBinding()]
    param (
        [Parameter(Mandatory=$true)]
        [string]$UnrealEngine
    )
    process
    {
        $UE4BatchFilesDir = "$UnrealEngine/Engine/Build/BatchFiles"
        $UATPath = "$UE4BatchFilesDir/RunUAT.bat"
        if ((-not (Test-Path -Path $UnrealEngine -PathType Container)) -or
            (-not (Test-Path -Path $UATPath -PathType Leaf)))
        {
            Write-Host -ForegroundColor Red "Incorrect UnrealEngine path provided: $UnrealEngine"
            Write-Host -ForegroundColor Red "UnrealEngine parameter should point to the root installation folder of Unreal Engine"
            throw "Unreal Engine not found"
        }
        return $UATPath
    }
}

<#
.SYNOPSIS
    Build a project using Unreal Automation Tool.

.PARAMETER UnrealEngine
    Path to Unreal Engine (root folder).

.PARAMETER CommandArgs
    Arguments passed to Unreal Automation Tool.

.OUTPUTS
    [boolean] true = success, false = errors occurred
#>
function Start-UAT
{
    [CmdletBinding()]
    param
    (
        [Parameter(Mandatory=$true)]
        [string]$UnrealEngine,
        [Parameter(Mandatory=$true)]
        [string[]]$CommandArgs,
        [boolean]$UseStaticAnalyzer = $True,
        [boolean]$WarningsAsErrors = $True,
        [boolean]$UseUnityBuild = $True
    )
    process
    {
        $UATPath = (Get-UATPath -UnrealEngine $UnrealEngine)

        # Remove previous build log
        $UnrealBuildLogPath = (Get-UnrealBuildLogPath -UnrealEngine $UnrealEngine)
        if (Test-Path -Path $UnrealBuildLogPath)
        {
            Write-Host "Removing existing build log: $UnrealBuildLogPath"
            Remove-Item -Path $UnrealBuildLogPath -Force -ErrorAction Stop
        }

        # Remove BuildConfiguration.xml files
        $MyDocuments = [Environment]::GetFolderPath("MyDocuments")
        $UserBuildConfigurationPath = "$Env:APPDATA\Unreal Engine\UnrealBuildTool\BuildConfiguration.xml"
        $BuildConfigurationFiles = @(
            $UserBuildConfigurationPath,
            "$MyDocuments\Unreal Engine\UnrealBuildTool\BuildConfiguration.xml"
        )
        $BuildConfigurationBackup = (Backup-Files -Files $BuildConfigurationFiles -ErrorAction Stop)

        # Write BuildConfiguration.xml file
        New-BuildConfiguration -OutPath $UserBuildConfigurationPath `
                               -UseStaticAnalyzer $UseStaticAnalyzer `
                               -UseUnityBuild $UseUnityBuild `
                               -ErrorAction Stop

        $Arguments = ($CommandArgs -join " ")
        Write-Host "Running UAT command:`n`t$UATPath $Arguments"

        $ProcInfo = New-Object System.Diagnostics.ProcessStartInfo
        $ProcInfo.FileName  = $UATPath
        $ProcInfo.Arguments = $Arguments
        $ProcInfo.UseShellExecute = $False
        $ProcInfo.RedirectStandardOutput = $True

        $Process = New-Object System.Diagnostics.Process
        $Process.StartInfo = $ProcInfo

        [Void]$Process.Start()

        $Success = $True
        $Line = ""
        while (1)
        {
            $Line = $Process.StandardOutput.ReadLine()

            if ($null -eq $Line)
            {
                break
            }
            else
            {
                Write-Host $line
                $Success = (Format-BuildError -Line $Line) -and $Success
            }
        }
        if ($Success)
        {
            Write-Host "No errors found in the log."
        }
        else
        {
            Write-Host "Errors reported during build (see log)."
        }

        $BuildResultCode = $Process.ExitCode
        $Success = $Success -and ($BuildResultCode -eq 0)
        Write-Host "UAT finished with RC=$BuildResultCode"

        # Restore BuildConfiguration.xml files
        Restore-BackupFiles -FilePathDictionary $BuildConfigurationBackup

        return $Success
    }
}
