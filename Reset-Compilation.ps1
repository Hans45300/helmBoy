# Script de gestion de compilation HelmBoy
# Auteur: Marc Scheffer
# Derniere modification: 10 novembre 2025

Clear-Host

Write-Host "======================================" -ForegroundColor Cyan
Write-Host "     HelmBoy - Gestion Build         " -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""

function Show-Menu {
    Clear-Host
    Write-Host "Choisissez une option:" -ForegroundColor Yellow
    Write-Host "1. Retour arriere vers le remote (hard reset + nettoyage)" -ForegroundColor White
    Write-Host "2. Mise a jour des sous-modules Git" -ForegroundColor White
    Write-Host "3. Nettoyage et reconstruction du dossier build" -ForegroundColor White
    Write-Host "4. Compilation vers une cible specifique" -ForegroundColor White
    Write-Host "5. Compilation complete Release" -ForegroundColor White
    Write-Host "6. Compilation complete Debug" -ForegroundColor White
    Write-Host "7. Quitter" -ForegroundColor White
    Write-Host ""
}

function Show-TargetMenu {
    Write-Host "Cibles disponibles :" -ForegroundColor Yellow
    Write-Host "1. HelmBoyData (Ressources binaires)" -ForegroundColor White
    Write-Host "2. mopo (Bibliotheque audio)" -ForegroundColor White
    Write-Host "3. HelmBoyPlugin (LV2)" -ForegroundColor White
    Write-Host "4. HelmBoyPlugin (VST3)" -ForegroundColor White
    Write-Host "5. HelmBoyStandalone (Application standalone)" -ForegroundColor White
    Write-Host "6. Retour au menu principal" -ForegroundColor White
    Write-Host ""
}

function Reset-ToRemote {
    Write-Host "Retour arriere vers le remote..." -ForegroundColor Red
    Write-Host "ATTENTION: Cela va supprimer TOUTES les modifications locales!" -ForegroundColor Red
    $confirm = Read-Host "etes-vous sur? (oui/non)"

    if ($confirm -eq "oui" -or $confirm -eq "o" -or $confirm -eq "y" -or $confirm -eq "yes") {
        Write-Host "Execution du reset git..." -ForegroundColor Yellow
        git reset --hard HEAD
        git clean -fd
        Write-Host "Reset termine avec succes!" -ForegroundColor Green
    } else {
        Write-Host "Operation annulee." -ForegroundColor Yellow
    }
}

function Clear-BuildDirectory {
    Clear-Host
    Write-Host 'Nettoyage et reconstruction du dossier build...' -ForegroundColor Yellow

    if (Test-Path "./build") {
        Write-Host "Suppression du dossier build existant..." -ForegroundColor Yellow
        Remove-Item -Path "./build" -Recurse -Force
    }

    Write-Host "Generation des fichiers de build..." -ForegroundColor Yellow
    cmake -S . -B build

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build directory reconstruit avec succes!" -ForegroundColor Green
    } else {
        Write-Host "Erreur lors de la generation des fichiers de build!" -ForegroundColor Red
    }
}

function Build-Target($targetName) {
    Clear-Host
    Write-Host "Compilation de la cible: $targetName" -ForegroundColor Yellow

    # Verifier que le dossier build existe
    if (-not (Test-Path "./build")) {
        Write-Host "Le dossier build n'existe pas. Generation..." -ForegroundColor Yellow
        Remove-Item /logs/builds.log
        cmake -S . -B build | Tee-Object /logs/builds.log
    }

    # Compiler la cible
    Remove-Item -Path /logs/compile.log
    cmake --build build --config Release --target $targetName --parallel | Tee-Object /logs/compile.log

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Compilation de $targetName terminee avec succes!" -ForegroundColor Green
    } else {
        Write-Host "Erreur lors de la compilation de $targetName !" -ForegroundColor Red
    }
}

function Build-All {
    Clear-Host
    Write-Host 'Compilation complete (toutes les cibles)...' -ForegroundColor Yellow

    # Verifier que le dossier build existe
    if (-not (Test-Path "./build")) {
        Write-Host "Le dossier build n'existe pas. Generation..." -ForegroundColor Yellow
        Remove-Item /logs/builds.log
        cmake -S . -B build | Tee-Object /logs/builds.log
    }

    # Compiler toutes les cibles
    Remove-Item -Path /logs/compile.log
    cmake --build build --config Release --parallel | Tee-Object /logs/compile.log

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Compilation complete terminee avec succes!" -ForegroundColor Green
    } else {
        Write-Host "Erreur lors de la compilation complete!" -ForegroundColor Red
    }
}

function Build-AllDebug {
    Clear-Host
    Write-Host 'Compilation complete (Debug)...' -ForegroundColor Yellow

    # Verifier que le dossier build existe
    if (-not (Test-Path "./build")) {
        Write-Host "Le dossier build n'existe pas. Generation..." -ForegroundColor Yellow
        Remove-Item /logs/builds.log
        cmake -S . -B build | Tee-Object /logs/builds.log
    }

    # Compiler toutes les cibles
    Remove-Item -Path /logs/compile.log
    cmake --build build --config Debug --parallel | Tee-Object /logs/compileDebug.log

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Compilation complete en debug terminee avec succes!" -ForegroundColor Green
    } else {
        Write-Host "Erreur lors de la compilation complete en debug !" -ForegroundColor Red
    }
}

# Boucle principale
do {
    Show-Menu
    $choice = Read-Host "Votre choix (1-7)"
    Write-Host ""

    switch ($choice) {
        "1" {
            Reset-ToRemote
        }
        "2"{
            Clear-Host
            Write-Host "Mise a jour des sous-modules Git..." -ForegroundColor Yellow
            git submodule update --init --recursive
            set-location externals/JUCE
            git checkout
            git fetch
            git pull
            Write-Host "Mise a jour de JUCE terminee." -ForegroundColor Green
            write-host "Mise a jour de VST3_SDK..." -ForegroundColor Yellow
            set-location ../VST3_SDK
            git checkout
            git fetch
            git pull
            write-host "Mise a jour de VST3_SDK terminee." -ForegroundColor Green
            write-host "Mise a jour de xsimd..." -ForegroundColor Yellow
            set-location ../xsimd
            git checkout
            git fetch
            git pull
            set-location ../../
            write-host "Mise a jour de xsimd terminee." -ForegroundColor Green
            Write-Host "Mise a jour terminee avec succes!" -ForegroundColor Green
        }
        "3" {
            Clear-BuildDirectory
        }
        "4" {
            do {
                Show-TargetMenu
                $targetChoice = Read-Host "Votre choix (1-6)"
                Write-Host ""

                    switch ($targetChoice) {
                    "1" { Build-Target "HelmBoyData" }
                    "2" { Build-Target "mopo" }
                    "3" { Build-Target "HelmBoyPlugin_LV2" }
                    "4" { Build-Target "HelmBoyPlugin_VST3" }
                    "5" { Build-Target "HelmBoyStandalone" }
                    "6" { break }
                    default { Write-Host "Choix invalide!" -ForegroundColor Red }
                }

                if ($targetChoice -ne "6") {
                    Write-Host ""
                    Read-Host "Appuyez sur Entree pour continuer"
                    Clear-Host
                }

            } while ($targetChoice -ne "6")
        }
        "5" {
            Clear-Host
            Build-All
        }
        "6"{
            Clear-Host
            Build-AllDebug
        }
        "7" {
            Clear-Host
            Write-Host "Au revoir !" -ForegroundColor Cyan
            exit
        }
        default {
            Write-Host "Choix invalide !" -ForegroundColor Red
        }
    }

    if ($choice -ne "5") {
        Write-Host ""
        Read-Host "Appuyez sur Entree pour continuer"
        Clear-Host
    }

} while ($choice -ne "7")