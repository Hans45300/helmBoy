param(
	[switch]$Clean,
	[switch]$CleanTemp,
	[switch]$NoLaunch,
	[Alias("BuildTargets")]
	[string[]]$BuildMode = "Standalone",
	[string]$PluginHostPath = "",
	[switch]$InstallVst3ForTesting,
	[switch]$OpenArtefactFolder,
	[switch]$SafeBuild,
	[switch]$DiagOnRetry,
	[ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
	[string]$Config = "Release",
	[int]$MinBuildFreeGB = 8,
	[int]$Jobs = [Math]::Max(1, [Environment]::ProcessorCount)
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "build"
$localTempRoot = Join-Path $env:LOCALAPPDATA "Temp\helmBoy-build"
$buildTempDir = Join-Path $localTempRoot ("session_" + [System.Diagnostics.Process]::GetCurrentProcess().Id)
Set-Location $repoRoot

function Resolve-BuildModes {
	param([string[]]$InputModes)

	$validModes = @("Standalone", "VST3", "LV2", "Plugin", "All")
	$resolved = New-Object System.Collections.Generic.List[string]

	foreach ($entry in $InputModes) {
		if ([string]::IsNullOrWhiteSpace($entry)) {
			continue
		}

		$parts = $entry -split '[,;\s]+' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
		foreach ($part in $parts) {
			$match = $validModes | Where-Object { $_.Equals($part, [System.StringComparison]::OrdinalIgnoreCase) } | Select-Object -First 1
			if ($null -eq $match) {
				throw "Mode de build invalide: '$part'. Modes valides: $($validModes -join ', ')"
			}
			$resolved.Add($match)
		}
	}

	if ($resolved.Count -eq 0) {
		$resolved.Add("Standalone")
	}

	return @($resolved.ToArray())
}

function Test-AnyBuildMode {
	param(
		[string[]]$CurrentModes,
		[string[]]$Needles
	)

	foreach ($needle in $Needles) {
		if ($CurrentModes -contains $needle) {
			return $true
		}
	}

	return $false
}

$BuildMode = Resolve-BuildModes -InputModes $BuildMode
Write-Host -ForegroundColor Green "Modes de build resolus: $($BuildMode -join ', ')"

function Get-FreeSpaceGB {
	param([string]$Path)

	$fullPath = [System.IO.Path]::GetFullPath($Path)
	$root = [System.IO.Path]::GetPathRoot($fullPath)
	if ([string]::IsNullOrWhiteSpace($root)) {
		return 0
	}

	$deviceId = $root.TrimEnd("\\")
	$drive = Get-CimInstance Win32_LogicalDisk -Filter "DeviceID='$deviceId'"
	if ($null -eq $drive) {
		return 0
	}

	return [Math]::Round(($drive.FreeSpace / 1GB), 2)
}

#region functions
function Configure-BuildTemp {
	param(
		[string]$TempDir,
		[switch]$Purge
	)

	if ([string]::IsNullOrWhiteSpace($TempDir)) {
		throw "TempDir invalide pour Configure-BuildTemp."
	}

	$resolvedTemp = [System.IO.Path]::GetFullPath($TempDir)

	if ($Purge -and (Test-Path $resolvedTemp)) {
		Write-Host -ForegroundColor Yellow "Nettoyage du dossier temp local build..."
		Get-ChildItem -Path $resolvedTemp -Force -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
	}

	if (-not (Test-Path $resolvedTemp)) {
		New-Item -ItemType Directory -Path $resolvedTemp -Force | Out-Null
	}

	$env:TEMP = $resolvedTemp
	$env:TMP = $resolvedTemp
	Write-Host -ForegroundColor Green "TEMP/TMP rediriges vers: $resolvedTemp"
}

function Adjust-ParallelJobs {
	param(
		[int]$RequestedJobs,
		[int]$FreeGB
	)

	if ($FreeGB -lt 12 -and $RequestedJobs -gt 4) {
		Write-Warning "Espace disque limite ($FreeGB GB). Jobs reduits de $RequestedJobs a 4."
		return 4
	}

	return $RequestedJobs
}

function Invoke-BuildTarget {
	param(
		[string]$Target,
		[string]$Configuration,
		[int]$ParallelJobs,
		[switch]$ForceSingleProc,
		[switch]$Diag
	)

	if ($ForceSingleProc) {
		if ($Diag) {
			$binlogPath = Join-Path $buildDir "$($Target)_$($Configuration)_retry.binlog"
			Write-Warning "Diagnostic MSBuild active. Binlog: $binlogPath"
			cmake --build $buildDir --config $Configuration --target $Target -- /m:1 /nodeReuse:false /p:UseMultiToolTask=false /v:diag /bl:$binlogPath | Out-Host
		}
		else {
			cmake --build $buildDir --config $Configuration --target $Target -- /m:1 /nodeReuse:false /p:UseMultiToolTask=false /v:minimal | Out-Host
		}
	}
	else {
		cmake --build $buildDir --config $Configuration --target $Target --parallel $ParallelJobs -- /nodeReuse:false | Out-Host
	}

	return $LASTEXITCODE
}

function Reset-TransientMsBuildState {
	param(
		[string]$Target,
		[string]$Configuration
	)

	$procNames = @('cl', 'c1xx', 'c2', 'mspdbsrv', 'link')
	foreach ($name in $procNames) {
		Get-Process -Name $name -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
	}

	$intermediateCandidates = @(
		(Join-Path $buildDir "$Target.dir\\$Configuration"),
		(Join-Path $buildDir "$Target\\$Target.dir\\$Configuration")
	)

	foreach ($candidate in $intermediateCandidates) {
		if (Test-Path $candidate) {
			Write-Warning "Purge des intermediaires potentiellement corrompus: $candidate"
			Remove-Item -LiteralPath $candidate -Recurse -Force -ErrorAction SilentlyContinue
		}
	}

	$tlogCandidates = @(
		(Join-Path $buildDir "$Target.dir\\$Target.tlog"),
		(Join-Path $buildDir "$Target\\$Target.dir\\$Target.tlog")
	)

	foreach ($candidate in $tlogCandidates) {
		if (Test-Path $candidate) {
			Remove-Item -LiteralPath $candidate -Recurse -Force -ErrorAction SilentlyContinue
		}
	}
}

function Get-TransientResetTargets {
	param([string]$PrimaryTarget)

	$targets = New-Object System.Collections.Generic.List[string]
	$targets.Add($PrimaryTarget)

	if ($PrimaryTarget -like "HelmBoyPlugin_*") {
		$targets.Add("mopo")
		$targets.Add("HelmBoyData")
		$targets.Add("HelmBoyPlugin_rc_lib")
	}
	elseif ($PrimaryTarget -eq "HelmBoyStandalone") {
		$targets.Add("mopo")
		$targets.Add("HelmBoyData")
		$targets.Add("HelmBoyStandalone_rc_lib")
	}

	return @($targets | Select-Object -Unique)
}

function Reset-TransientMsBuildStateForTargetGroup {
	param(
		[string]$PrimaryTarget,
		[string]$Configuration
	)

	$targetsToReset = Get-TransientResetTargets -PrimaryTarget $PrimaryTarget
	foreach ($targetName in $targetsToReset) {
		Reset-TransientMsBuildState -Target $targetName -Configuration $Configuration
	}
}

function Invoke-BuildTargetWithRetry {
	param(
		[string]$Target,
		[string]$Configuration,
		[int]$ParallelJobs,
		[switch]$ForceSafeBuild,
		[switch]$EnableDiagOnRetry
	)

	$firstTrySingle = $ForceSafeBuild.IsPresent
	Ensure-ValidRootBuildTree -BuildDir $buildDir -RepoRoot $repoRoot
	$code = Invoke-BuildTarget -Target $Target -Configuration $Configuration -ParallelJobs $ParallelJobs -ForceSingleProc:$firstTrySingle

	if ($code -ne 0 -and -not $firstTrySingle) {
		Write-Warning "Echec build parallele sur $Target. Retry en mode safe (/m:1)..."
		Reset-TransientMsBuildStateForTargetGroup -PrimaryTarget $Target -Configuration $Configuration
		$code = Invoke-BuildTarget -Target $Target -Configuration $Configuration -ParallelJobs 1 -ForceSingleProc
	}

	if ($code -ne 0 -and $EnableDiagOnRetry) {
		Write-Warning "Retry diagnostic actif sur $Target (/v:diag + binlog)..."
		Reset-TransientMsBuildStateForTargetGroup -PrimaryTarget $Target -Configuration $Configuration
		$code = Invoke-BuildTarget -Target $Target -Configuration $Configuration -ParallelJobs 1 -ForceSingleProc -Diag
	}

	if ($code -ne 0) {
		Write-Warning "Build tree potentiellement incoherent apres echec de $Target. Reconfiguration forcee puis dernier retry safe..."
		Ensure-ValidRootBuildTree -BuildDir $buildDir -RepoRoot $repoRoot -ForceReconfigure
		Reset-TransientMsBuildStateForTargetGroup -PrimaryTarget $Target -Configuration $Configuration
		$code = Invoke-BuildTarget -Target $Target -Configuration $Configuration -ParallelJobs 1 -ForceSingleProc
	}

	if ($code -ne 0) {
		throw "Compilation $Target echouee (code=$code)."
	}
}

function Invoke-Step {
	param(
		[string]$Message,
		[scriptblock]$Action
	)

	Write-Host -ForegroundColor Green $Message
	& $Action
	if ($LASTEXITCODE -ne 0) {
		throw "Echec de l'etape: $Message"
	}
}

function Patch-JuceaideExitCode {
	param([string]$ProjectFile)

	if (-not (Test-Path $ProjectFile)) {
		return
	}

	[string]$xml = Get-Content -Raw -Path $ProjectFile
	$updated = $xml -replace '(juceaide\.exe header[^\r\n]*\r?\n)if %errorlevel% neq 0 goto :cmEnd', '$1ver >nul'

	if ($updated -ne $xml) {
		Set-Content -Path $ProjectFile -Value $updated -Encoding UTF8
		Write-Warning "Patch juceaide applique sur $ProjectFile"
	}
}

function Wait-ForVisibleWindow {
	param(
		[string]$ProcessName,
		[int]$TimeoutMs = 12000,
		[int]$PollMs = 200
	)

	$sw = [System.Diagnostics.Stopwatch]::StartNew()
	while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
		$running = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue
		if ($running) {
			$visible = $running | Where-Object { $_.MainWindowHandle -ne 0 }
			if ($visible) {
				return $visible[0]
			}
		}
		Start-Sleep -Milliseconds $PollMs
	}

	return $null
}

function Get-CMakeCacheCompilerPaths {
	param([string]$CacheFile)

	if (-not (Test-Path $CacheFile)) {
		return @()
	}

	$compilerLines = Get-Content -Path $CacheFile -ErrorAction SilentlyContinue |
		Where-Object { $_ -match '^CMAKE_(C|CXX)_COMPILER:FILEPATH=' }

	$paths = New-Object System.Collections.Generic.List[string]
	foreach ($line in $compilerLines) {
		$parts = $line -split '=', 2
		if ($parts.Count -eq 2 -and -not [string]::IsNullOrWhiteSpace($parts[1])) {
			$paths.Add($parts[1].Trim())
		}
	}

	return @($paths.ToArray())
}

function Reset-StaleCMakeCompilerCaches {
	param([string]$RootBuildDir)

	$cacheLocations = @(
		(Join-Path $RootBuildDir "CMakeCache.txt"),
		(Join-Path $RootBuildDir "externals\JUCE\tools\CMakeCache.txt"),
		(Join-Path $RootBuildDir "HelmBoyPlugin_artefacts\JuceLibraryCode\vst3_helper\CMakeCache.txt")
	)

	foreach ($cacheFile in $cacheLocations) {
		if (-not (Test-Path $cacheFile)) {
			continue
		}

		$compilerPaths = Get-CMakeCacheCompilerPaths -CacheFile $cacheFile
		$missingCompiler = $false
		$instanceLine = Get-Content -Path $cacheFile -ErrorAction SilentlyContinue |
			Where-Object { $_ -match '^CMAKE_GENERATOR_INSTANCE:[^=]*=' } |
			Select-Object -First 1
		$missingInstance = $false
		if (-not [string]::IsNullOrWhiteSpace($instanceLine)) {
			$instancePath = (($instanceLine -split '=', 2)[1] -split ',', 2)[0].Trim()
			if (-not [string]::IsNullOrWhiteSpace($instancePath)) {
				$missingInstance = -not (Test-Path -LiteralPath $instancePath -PathType Container)
			}
		}
		foreach ($compilerPath in $compilerPaths) {
			if (-not (Test-Path $compilerPath)) {
				$missingCompiler = $true
				break
			}
		}

		if (-not $missingCompiler -and -not $missingInstance) {
			continue
		}

		$cacheDir = Split-Path -Parent $cacheFile
		Write-Warning "Cache CMake obsolete detecte ($cacheFile). Reinitialisation des metadonnees CMake."
		$metadataPaths = @($cacheFile, (Join-Path $cacheDir "CMakeFiles"))
		foreach ($metadataPath in $metadataPaths) {
			if (Test-Path -LiteralPath $metadataPath) {
				Remove-Item -LiteralPath $metadataPath -Recurse -Force -ErrorAction Stop
			}
		}
	}
}

function Reset-JuceaideBootstrapArtifacts {
	param([string]$RootBuildDir)

	$juceaideToolsDir = Join-Path $RootBuildDir "externals\JUCE\tools"
	if (-not (Test-Path $juceaideToolsDir)) {
		return
	}

	Write-Warning "Purge preventive juceaide: $juceaideToolsDir"

	try {
		Get-ChildItem -Path $juceaideToolsDir -Recurse -Force -ErrorAction SilentlyContinue |
			ForEach-Object {
				try {
					if ($_.Attributes -band [System.IO.FileAttributes]::ReadOnly) {
						$_.Attributes = ($_.Attributes -bxor [System.IO.FileAttributes]::ReadOnly)
					}
				}
				catch {
					# best effort
				}
			}
	}
	catch {
		# best effort
	}

	Remove-Item -Path $juceaideToolsDir -Recurse -Force -ErrorAction SilentlyContinue
}

function Stop-OrphanedToolchainProcesses {
	$procNames = @('cl', 'c1xx', 'c2', 'mspdbsrv', 'link', 'rc')
	foreach ($name in $procNames) {
		Get-Process -Name $name -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
	}
}

function Ensure-ValidRootBuildTree {
	param(
		[string]$BuildDir,
		[string]$RepoRoot,
		[switch]$ForceReconfigure
	)

	$cacheFile = Join-Path $BuildDir "CMakeCache.txt"
	$needsConfigure = $ForceReconfigure.IsPresent -or -not (Test-Path $cacheFile)

	if (-not $needsConfigure) {
		$homeLine = Get-Content -Path $cacheFile -ErrorAction SilentlyContinue |
			Where-Object { $_ -like "CMAKE_HOME_DIRECTORY:INTERNAL=*" } |
			Select-Object -First 1

		if ([string]::IsNullOrWhiteSpace($homeLine)) {
			$needsConfigure = $true
		}
		else {
			$cacheHome = ($homeLine -split '=', 2)[1].Trim()
			$repoNormalized = ([System.IO.Path]::GetFullPath($RepoRoot)).Replace('\\', '/')
			$cacheNormalized = ([System.IO.Path]::GetFullPath($cacheHome)).Replace('\\', '/')
			if ($repoNormalized -ne $cacheNormalized) {
				Write-Warning "CMAKE_HOME_DIRECTORY inattendu: $cacheHome (attendu: $RepoRoot)."
				$needsConfigure = $true
			}
		}
	}

	if (-not $needsConfigure) {
		return
	}

	if (-not (Test-Path $BuildDir)) {
		New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
	}

	Write-Warning "Reconfiguration CMake du build tree racine..."
	cmake -S $RepoRoot -B $BuildDir -G "Visual Studio 18 2026" -A x64 | Out-Host
	if ($LASTEXITCODE -ne 0) {
		throw "Reconfiguration CMake echouee pour $BuildDir"
	}
}

function Install-Vst3BundleForTesting {
	param(
		[string]$SourceVst3Dir
	)

	if (-not (Test-Path $SourceVst3Dir)) {
		throw "Source VST3 introuvable: $SourceVst3Dir"
	}

	$targetCandidates = @()
	if ($env:CommonProgramFiles) {
		$targetCandidates += (Join-Path $env:CommonProgramFiles "VST3")
	}
	if ($env:LOCALAPPDATA) {
		$targetCandidates += (Join-Path $env:LOCALAPPDATA "Programs\Common\VST3")
	}

	$sourceName = Split-Path -Leaf $SourceVst3Dir
	$lastErrorMessage = ""

	foreach ($baseTarget in $targetCandidates) {
		try {
			if (-not (Test-Path $baseTarget)) {
				New-Item -ItemType Directory -Path $baseTarget -Force | Out-Null
			}

			$targetPath = Join-Path $baseTarget $sourceName
			if (Test-Path $targetPath) {
				Remove-Item -Path $targetPath -Recurse -Force -ErrorAction Stop
			}

			Copy-Item -Path $SourceVst3Dir -Destination $targetPath -Recurse -Force -ErrorAction Stop
			Write-Host -ForegroundColor Green "Plugin VST3 installe pour test: $targetPath"
			return $targetPath
		}
		catch {
			$lastErrorMessage = $_.Exception.Message
			Write-Warning "Installation VST3 impossible vers $baseTarget ($lastErrorMessage)"
		}
	}

	throw "Impossible d'installer le VST3 automatiquement. Derniere erreur: $lastErrorMessage"
}

#endregion functions

if ($Clean -and (Test-Path $buildDir)) {
	Write-Host -ForegroundColor Yellow "Nettoyage complet du dossier build..."
	Remove-Item $buildDir -Recurse -Force -ErrorAction SilentlyContinue
}

if (-not (Test-Path $buildDir)) {
	New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
}

Configure-BuildTemp -TempDir $buildTempDir -Purge:$CleanTemp

$freeBuildGB = Get-FreeSpaceGB -Path $buildDir
Write-Host -ForegroundColor Green "Espace libre sur disque build: $freeBuildGB GB"
if ($freeBuildGB -lt $MinBuildFreeGB) {
	throw "Espace disque insuffisant sur le disque de build ($freeBuildGB GB). Minimum requis: $MinBuildFreeGB GB."
}

$Jobs = Adjust-ParallelJobs -RequestedJobs $Jobs -FreeGB $freeBuildGB

Reset-StaleCMakeCompilerCaches -RootBuildDir $buildDir
Reset-JuceaideBootstrapArtifacts -RootBuildDir $buildDir

$previousCMakeParallel = $env:CMAKE_BUILD_PARALLEL_LEVEL
$previousCMakeGenerator = $env:CMAKE_GENERATOR
$previousCMakeGeneratorPlatform = $env:CMAKE_GENERATOR_PLATFORM
$env:CMAKE_BUILD_PARALLEL_LEVEL = "1"
$env:CMAKE_GENERATOR = "Visual Studio 18 2026"
$env:CMAKE_GENERATOR_PLATFORM = "x64"
try {
	$configured = $false
	for ($attempt = 1; $attempt -le 2 -and -not $configured; $attempt++) {
		if ($attempt -gt 1) {
			Write-Warning "Retry configuration CMake apres echec juceaide/bootstrap..."
			Stop-OrphanedToolchainProcesses
			Reset-JuceaideBootstrapArtifacts -RootBuildDir $buildDir
		}

		cmake -S . -B $buildDir -G "Visual Studio 18 2026" -A x64 | Out-Host
		if ($LASTEXITCODE -eq 0) {
			$configured = $true
		}
	}

	if (-not $configured) {
		throw "Echec de l'etape: Configuration CMake..."
	}
}
finally {
	if ([string]::IsNullOrWhiteSpace($previousCMakeParallel)) {
		Remove-Item Env:CMAKE_BUILD_PARALLEL_LEVEL -ErrorAction SilentlyContinue
	}
	else {
		$env:CMAKE_BUILD_PARALLEL_LEVEL = $previousCMakeParallel
	}

	if ([string]::IsNullOrWhiteSpace($previousCMakeGenerator)) {
		Remove-Item Env:CMAKE_GENERATOR -ErrorAction SilentlyContinue
	}
	else {
		$env:CMAKE_GENERATOR = $previousCMakeGenerator
	}

	if ([string]::IsNullOrWhiteSpace($previousCMakeGeneratorPlatform)) {
		Remove-Item Env:CMAKE_GENERATOR_PLATFORM -ErrorAction SilentlyContinue
	}
	else {
		$env:CMAKE_GENERATOR_PLATFORM = $previousCMakeGeneratorPlatform
	}
}

Patch-JuceaideExitCode -ProjectFile (Join-Path $buildDir "HelmBoyPlugin.vcxproj")
Patch-JuceaideExitCode -ProjectFile (Join-Path $buildDir "HelmBoyStandalone.vcxproj")

$targets = @()
foreach ($buildEntry in $BuildMode) {
	switch ($buildEntry) {
		"Standalone" { $targets += @("HelmBoyStandalone") }
		"VST3" { $targets += @("HelmBoyPlugin_VST3") }
		"LV2" { $targets += @("HelmBoyPlugin_LV2") }
		"Plugin" { $targets += @("HelmBoyPlugin_VST3", "HelmBoyPlugin_LV2") }
		"All" { $targets += @("HelmBoyPlugin_VST3", "HelmBoyPlugin_LV2", "HelmBoyStandalone") }
	}
}
Write-Host -ForegroundColor Green "Cibles de build: $($targets -join ", ")"

$uniqueTargets = $targets | Select-Object -Unique

if ($uniqueTargets -contains "HelmBoyStandalone") {
	Invoke-Step -Message "Pre-build des dependances standalone (HelmBoyData)..." -Action {
		Invoke-BuildTargetWithRetry -Target "HelmBoyData" -Configuration $Config -ParallelJobs $Jobs -ForceSafeBuild:$SafeBuild -EnableDiagOnRetry:$DiagOnRetry
	}
}

foreach ($target in $uniqueTargets) {
	Invoke-Step -Message "Compilation de $target ($Config, jobs=$Jobs)..." -Action {
		Invoke-BuildTargetWithRetry -Target $target -Configuration $Config -ParallelJobs $Jobs -ForceSafeBuild:$SafeBuild -EnableDiagOnRetry:$DiagOnRetry
	}
}

Write-Host -ForegroundColor Green "Compilation terminee pour mode=$BuildMode."

$pluginVst3Path = Join-Path $repoRoot "build\HelmBoyPlugin_artefacts\$Config\VST3\helmBoy.vst3"
$pluginLv2Path = Join-Path $repoRoot "build\HelmBoyPlugin_artefacts\$Config\LV2\helmBoy.lv2"

if (Test-AnyBuildMode -CurrentModes $BuildMode -Needles @("VST3", "Plugin", "All")) {
	if (Test-Path $pluginVst3Path) {
		Write-Host -ForegroundColor Green "Plugin VST3 genere: $pluginVst3Path"
		if ($InstallVst3ForTesting) {
			$installedVst3Path = Install-Vst3BundleForTesting -SourceVst3Dir $pluginVst3Path
			Write-Host -ForegroundColor Cyan "Cantabile: lancez un re-scan des plugins, puis chargez helmBoy."
		}
		Write-Host -ForegroundColor Cyan "Test plugin rapide:"
		Write-Host -ForegroundColor Cyan "  1) Ouvrir votre DAW"
		Write-Host -ForegroundColor Cyan "  2) Scanner le dossier VST3 (ou dossier installe si -InstallVst3ForTesting)"
		Write-Host -ForegroundColor Cyan "  3) Charger helmBoy sur une piste"

		if (-not [string]::IsNullOrWhiteSpace($PluginHostPath)) {
			if (Test-Path $PluginHostPath) {
				Write-Host -ForegroundColor Green "Lancement de l'hote plugin: $PluginHostPath"
				Start-Process -FilePath $PluginHostPath | Out-Null
			}
			else {
				Write-Warning "PluginHostPath introuvable: $PluginHostPath"
			}
		}
	}
	else {
		Write-Warning "Plugin VST3 non trouve apres build initial: $pluginVst3Path"
		Write-Warning "Tentative de build explicite de la cible VST3..."
		Invoke-BuildTargetWithRetry -Target "HelmBoyPlugin_VST3" -Configuration $Config -ParallelJobs $Jobs -ForceSafeBuild:$SafeBuild -EnableDiagOnRetry:$DiagOnRetry

		if (Test-Path $pluginVst3Path) {
			Write-Host -ForegroundColor Green "Plugin VST3 genere apres fallback: $pluginVst3Path"
			if ($InstallVst3ForTesting) {
				$installedVst3Path = Install-Vst3BundleForTesting -SourceVst3Dir $pluginVst3Path
				Write-Host -ForegroundColor Cyan "Cantabile: lancez un re-scan des plugins, puis chargez helmBoy."
			}
			if (-not [string]::IsNullOrWhiteSpace($PluginHostPath) -and (Test-Path $PluginHostPath)) {
				Write-Host -ForegroundColor Green "Lancement de l'hote plugin: $PluginHostPath"
				Start-Process -FilePath $PluginHostPath | Out-Null
			}
		}
		else {
			Write-Warning "Plugin VST3 toujours introuvable apres fallback: $pluginVst3Path"
			Write-Warning "Verifier la configuration active ($Config) et la presence du projet HelmBoyPlugin_VST3 dans build/."
		}
	}
}

if (Test-AnyBuildMode -CurrentModes $BuildMode -Needles @("LV2", "Plugin", "All")) {
	if (Test-Path $pluginLv2Path) {
		Write-Host -ForegroundColor Green "Plugin LV2 genere: $pluginLv2Path"
	}
	else {
		Write-Warning "Plugin LV2 non trouve apres build initial: $pluginLv2Path"
		Write-Warning "Tentative de build explicite de la cible LV2..."
		Invoke-BuildTargetWithRetry -Target "HelmBoyPlugin_LV2" -Configuration $Config -ParallelJobs $Jobs -ForceSafeBuild:$SafeBuild -EnableDiagOnRetry:$DiagOnRetry

		if (Test-Path $pluginLv2Path) {
			Write-Host -ForegroundColor Green "Plugin LV2 genere apres fallback: $pluginLv2Path"
		}
		else {
			Write-Warning "Plugin LV2 toujours introuvable apres fallback: $pluginLv2Path"
			Write-Warning "Verifier la configuration active ($Config) et la presence du projet HelmBoyPlugin_LV2 dans build/."
		}
	}
}

if ($OpenArtefactFolder) {
	if (Test-AnyBuildMode -CurrentModes $BuildMode -Needles @("VST3", "Plugin", "All")) {
		$pluginArtefactDir = Split-Path -Parent $pluginVst3Path
		if (Test-Path $pluginArtefactDir) {
			Start-Process explorer.exe $pluginArtefactDir
		}
	}

	if (Test-AnyBuildMode -CurrentModes $BuildMode -Needles @("LV2", "Plugin", "All")) {
		$lv2ArtefactDir = Split-Path -Parent $pluginLv2Path
		if (Test-Path $lv2ArtefactDir) {
			Start-Process explorer.exe $lv2ArtefactDir
		}
	}
}

if ($NoLaunch -or (Test-AnyBuildMode -CurrentModes $BuildMode -Needles @("VST3", "LV2", "Plugin", "All"))) {
	Write-Host -ForegroundColor Yellow "Lancement standalone ignore (NoLaunch ou mode Plugin)."
	exit 0
}

$standaloneExe = Join-Path $repoRoot "build\HelmBoyStandalone_artefacts\$Config\helmBoy.exe"
if (-not (Test-Path $standaloneExe)) {
	throw "Executable introuvable: $standaloneExe"
}

$workDir = Split-Path -Parent $standaloneExe

$existing = Get-Process -Name "helmBoy" -ErrorAction SilentlyContinue
if ($existing) {
	$visible = $existing | Where-Object { $_.MainWindowHandle -ne 0 }
	if ($visible) {
		Write-Host -ForegroundColor Yellow "Standalone deja lance (PID=$($visible[0].Id))."
		exit 0
	}

	Write-Warning "Instance helmBoy detectee sans fenetre. Redemarrage de precaution."
	$existing | Stop-Process -Force -ErrorAction SilentlyContinue
}

Start-Process -FilePath $standaloneExe -WorkingDirectory $workDir | Out-Null
$startedVisible = Wait-ForVisibleWindow -ProcessName "helmBoy" -TimeoutMs 12000 -PollMs 200
if ($startedVisible) {
	Write-Host -ForegroundColor Green "Lancement du standalone confirme (PID=$($startedVisible.Id))."
	if ($OpenArtefactFolder) {
		Start-Process explorer.exe $workDir
	}
}
else {
	Write-Warning "Le standalone est lance mais aucune fenetre visible apres 12s."
	Write-Warning "Test manuel conseille: $standaloneExe"
}