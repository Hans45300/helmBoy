Write-Host "Starting coding session..."
Write-Host "Updating XSimD..."
Set-Location $PSScriptRoot/../externals/xsimd
& git fetch
& git pull
Write-Host "Updating JUCE..."
Set-Location $PSScriptRoot/../externals/JUCE
& git fetch
& git pull
Write-Host "Updating VST3_SDK..."
Set-Location $PSScriptRoot/../externals/VST3_SDK
& git fetch
& git pull
Write-Host "Updating Main Repository..."
Set-Location ../..
& git fetch
& git pull
Write-Host "Done!"
