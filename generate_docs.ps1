# generate_docs.ps1 - PowerShell script to generate Doxygen documentation for HelmBoy

Write-Host "Generating HelmBoy Synthesizer Documentation..." -ForegroundColor Green

# Check if Doxygen is installed
$doxygenPath = Get-Command doxygen -ErrorAction SilentlyContinue
if ($null -eq $doxygenPath) {
    Write-Host "Error: Doxygen is not installed or not in PATH." -ForegroundColor Red
    Write-Host "Please download and install Doxygen from: https://www.doxygen.nl/download.html" -ForegroundColor Yellow
    Write-Host "Make sure to add Doxygen to your system PATH." -ForegroundColor Yellow
    exit 1
}

Write-Host "Found Doxygen at: $($doxygenPath.Source)" -ForegroundColor Cyan

# Create docs directory if it doesn't exist
if (!(Test-Path "docs")) {
    New-Item -ItemType Directory -Path "docs" | Out-Null
    Write-Host "Created docs directory" -ForegroundColor Yellow
}


# Update Doxyfile with current date
$currentDate = Get-Date -Format "yyyy.MM.dd"
$doxyContent = Get-Content "Doxyfile"
#get doxyfile output folder
$doxyOutputFolder = ($doxyContent | Select-String -Pattern "OUTPUT_DIRECTORY\s*=\s*(.*)").Matches[0].Groups[1].Value.Trim()
$doxyContent = $doxyContent -replace "PROJECT_NUMBER\s*=.*", "PROJECT_NUMBER = `"$currentDate`""
$doxyContent | Set-Content "Doxyfile.tmp"

# Generate documentation
Write-Host "Running Doxygen..." -ForegroundColor Cyan
try {
    & doxygen "Doxyfile.tmp" 2>&1 | Tee-Object -Variable doxygenOutput
    $exitCode = $LASTEXITCODE

    # Clean up temporary file
    Remove-Item "Doxyfile.tmp" -ErrorAction SilentlyContinue

    if ($exitCode -eq 0) {
        Write-Host "Documentation generated successfully!" -ForegroundColor Green
        Write-Host "Open $doxyOutputFolder\html\index.html in your web browser to view the documentation." -ForegroundColor Yellow

        # Optional: Open documentation in default browser
        $openDocs = Read-Host "Would you like to open the documentation now? (y/N)"
        if ($openDocs -eq "y" -or $openDocs -eq "Y") {
            $indexPath = Join-Path (Get-Location) "$doxyOutputFolder\html\index.html"
            if (Test-Path $indexPath) {
                Start-Process $indexPath
            } else {
                Write-Host "Warning: index.html not found at expected location." -ForegroundColor Yellow
            }
        }
    } else {
        Write-Host "Error: Documentation generation failed!" -ForegroundColor Red
        Write-Host "Doxygen output:" -ForegroundColor Yellow
        Write-Host $doxygenOutput -ForegroundColor White
        exit $exitCode
    }
} catch {
    Write-Host "Error running Doxygen: $($_.Exception.Message)" -ForegroundColor Red
    Remove-Item "Doxyfile.tmp" -ErrorAction SilentlyContinue
    exit 1
}

Write-Host "Documentation generation complete." -ForegroundColor Green

# Display some statistics if docs were generated
$htmlDir = "docs\html"
if (Test-Path $htmlDir) {
    $htmlFiles = Get-ChildItem -Path $htmlDir -Filter "*.html" -Recurse
    $cssFiles = Get-ChildItem -Path $htmlDir -Filter "*.css" -Recurse
    $jsFiles = Get-ChildItem -Path $htmlDir -Filter "*.js" -Recurse

    Write-Host "`nDocumentation Statistics:" -ForegroundColor Cyan
    Write-Host "  HTML files: $($htmlFiles.Count)" -ForegroundColor White
    Write-Host "  CSS files: $($cssFiles.Count)" -ForegroundColor White
    Write-Host "  JavaScript files: $($jsFiles.Count)" -ForegroundColor White

    $totalSize = ($htmlFiles + $cssFiles + $jsFiles | Measure-Object -Property Length -Sum).Sum
    $sizeInMB = [math]::Round($totalSize / 1MB, 2)
    Write-Host "  Total size: $sizeInMB MB" -ForegroundColor White
}