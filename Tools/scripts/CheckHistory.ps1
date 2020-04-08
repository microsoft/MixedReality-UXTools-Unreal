# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

$disallowed=(git show origin/private:disallowed.txt)
if(!$disallowed) {
	Write-Host "Unable to read disallowed text"
	exit 1
}

$bad=(git log -p) | Select-String "\b$disallowed\b" | Out-String
if($bad) {
	Write-Host "Error: found disallowed words in the history"
	Write-Host $bad
	exit 1
}

Write-Host "History check completed successfully."
exit 0
