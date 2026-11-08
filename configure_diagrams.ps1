# Configure Doxygen for Class Diagrams
# This script updates the Doxyfile to enable Graphviz diagrams

param(
    [switch]$EnableAll = $false,
    [switch]$UmlStyle = $false
)

$ErrorActionPreference = "Stop"

$doxyfilePath = "Doxyfile"

Write-Host "=== Configuring Doxygen for Class Diagrams ===" -ForegroundColor Green
Write-Host

# Check if Doxyfile exists
if (-not (Test-Path $doxyfilePath)) {
    Write-Host "? Doxyfile not found in current directory" -ForegroundColor Red
    exit 1
}

# Check if Graphviz is installed
try {
    $dotVersion = & dot -V 2>&1
    Write-Host "? Graphviz found: $dotVersion" -ForegroundColor Green
} catch {
    Write-Host "? Graphviz (dot) not found. Please install Graphviz first:" -ForegroundColor Red
    Write-Host "   Run: .\install_graphviz.ps1" -ForegroundColor Yellow
    exit 1
}

Write-Host "?? Configuring Doxyfile for diagrams..." -ForegroundColor Cyan

# Read the Doxyfile
$content = Get-Content $doxyfilePath

# Configuration updates
$updates = @{
    "HAVE_DOT" = "YES"
    "CLASS_DIAGRAMS" = "YES"
    "COLLABORATION_GRAPH" = if ($EnableAll) { "YES" } else { "YES" }
    "GROUP_GRAPHS" = if ($EnableAll) { "YES" } else { "YES" }
    "UML_LOOK" = if ($UmlStyle) { "YES" } else { "NO" }
    "INCLUDE_GRAPH" = if ($EnableAll) { "YES" } else { "NO" }
    "INCLUDED_BY_GRAPH" = if ($EnableAll) { "YES" } else { "NO" }
    "CALL_GRAPH" = if ($EnableAll) { "NO" } else { "NO" }  # Can be very large
    "CALLER_GRAPH" = if ($EnableAll) { "NO" } else { "NO" } # Can be very large
    "GRAPHICAL_HIERARCHY" = "YES"
    "DIRECTORY_GRAPH" = if ($EnableAll) { "YES" } else { "YES" }
    "DOT_IMAGE_FORMAT" = "svg"
    "INTERACTIVE_SVG" = "YES"
    "DOT_TRANSPARENT" = "YES"
    "GENERATE_LEGEND" = "YES"
}

# Apply updates
$modified = $false
foreach ($key in $updates.Keys) {
    $value = $updates[$key]
    $pattern = "^$key\s*=.*"
    $replacement = "$key               = $value"

    $newContent = @()
    $found = $false

    foreach ($line in $content) {
        if ($line -match $pattern) {
            if ($line -notmatch "=\s*$value\s*$") {
                Write-Host "  Updating: $key = $value" -ForegroundColor Yellow
                $newContent += $replacement
                $modified = $true
            } else {
                Write-Host "  Already set: $key = $value" -ForegroundColor Gray
                $newContent += $line
            }
            $found = $true
        } else {
            $newContent += $line
        }
    }

    if (-not $found) {
        Write-Host "  Warning: $key not found in Doxyfile" -ForegroundColor Yellow
    }

    $content = $newContent
}

# Save updated Doxyfile
if ($modified) {
    $content | Set-Content $doxyfilePath -Encoding UTF8
    Write-Host "? Doxyfile updated successfully" -ForegroundColor Green
} else {
    Write-Host "? Doxyfile already configured correctly" -ForegroundColor Green
}

Write-Host
Write-Host "?? Diagram Configuration Summary:" -ForegroundColor Cyan
Write-Host "  • Class Diagrams: Enabled" -ForegroundColor White
Write-Host "  • Collaboration Graphs: Enabled" -ForegroundColor White
Write-Host "  • Graphical Hierarchy: Enabled" -ForegroundColor White
Write-Host "  • Directory Graphs: Enabled" -ForegroundColor White
Write-Host "  • Output Format: SVG (interactive)" -ForegroundColor White
Write-Host "  • UML Style: $(if ($UmlStyle) { 'Enabled' } else { 'Disabled' })" -ForegroundColor White

if ($EnableAll) {
    Write-Host "  • Include Graphs: Enabled" -ForegroundColor White
    Write-Host "  • Call/Caller Graphs: Disabled (can be very large)" -ForegroundColor Yellow
} else {
    Write-Host "  • Include Graphs: Disabled (use -EnableAll to enable)" -ForegroundColor Gray
}

Write-Host
Write-Host "?? Diagram configuration completed!" -ForegroundColor Green
Write-Host "Run: .\generate_docs.ps1 to generate documentation with diagrams" -ForegroundColor Cyan

# Show example of what diagrams will include
Write-Host
Write-Host "?? Diagrams that will be generated:" -ForegroundColor Cyan
Write-Host "  • Class inheritance hierarchies" -ForegroundColor White
Write-Host "  • Class collaboration (usage) relationships" -ForegroundColor White
Write-Host "  • Module/group structure diagrams" -ForegroundColor White
Write-Host "  • Directory dependency graphs" -ForegroundColor White
Write-Host "  • Interactive SVG diagrams with clickable elements" -ForegroundColor White

if ($UmlStyle) {
    Write-Host "  • UML-style class diagrams with member details" -ForegroundColor White
}

Write-Host
Write-Host "?? Tips:" -ForegroundColor Cyan
Write-Host "  • Use -UmlStyle for detailed UML-style diagrams" -ForegroundColor White
Write-Host "  • Use -EnableAll for maximum diagram coverage (slower generation)" -ForegroundColor White
Write-Host "  • SVG diagrams are interactive - click elements to navigate!" -ForegroundColor White