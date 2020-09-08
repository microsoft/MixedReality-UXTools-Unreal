<#
.Synopsis
Convert test result file created by Unreal Engine automation (JSON) into JUnit XML format
that can be published in ADO pipeline test results page.
JUnit output description: https://llg.cubic.org/docs/junit/
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$Path,
    [Parameter(Mandatory = $true)]
    [string]$Output
)

$json = ((Get-Content $Path -Encoding UTF8) | ConvertFrom-JSON)
$out = @()
$out += '<?xml version="1.0" encoding="UTF-8"?>'
$totalTestCount = $json.succeeded + $json.succeededWithWarnings + $json.failed + $json.notRun
$out += @"
<testsuites name="$($json.clientDescriptor) - $($json.reportCreatedOn)"
            tests="$totalTestCount"
            failures="$($json.failed)"
            time="$($json.totalDuration)" >
"@
$out += @"
  <testsuite name="$($json.clientDescriptor) - $($json.reportCreatedOn)"
             id="0"
             tests="$totalTestCount"
             skipped="$($json.notRun)"
             failures="$($json.failed)"
             time="$($json.totalDuration)" >
"@

$json.tests | ForEach-Object {
    $out += @"
    <testcase name="$($_.testDisplayName)"
         classname="$($_.fullTestPath)" >
"@
    $errorMessage = ""
    $errorContext = ""
    $_.entries | ForEach-Object {
        if ($_.event.type -eq "Error")
        {
            if ($errorMessage -ne "")
            {
                $errorMessage += " "
            }
            $errorMessage += $_.event.message 
            if ($_.event.context -ne "")
            {
                if ($errorContext -ne "")
                {
                    $errorContext += "`n"
                }
                $errorContext += $_.event.context
            }
        }
    }
    if (($_.state -eq "Fail") -and ($errorMessage -eq ""))
    {
        $errorMessage = "Test failed. See logs for details"
    }
    if ($errorMessage -ne "")
    {
        $out += @"
      <failure message="$errorMessage" type="ERROR">
$errorContext
      </failure>
"@
    }
    $out += "    </testcase>"
}

$out += "  </testsuite>"
$out += "</testsuites>"

($out -join "`n") | Out-File -FilePath $Output -Encoding UTF8
