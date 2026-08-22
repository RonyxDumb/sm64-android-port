


param(
    [string]$Repo = "."
)

$ErrorActionPreference = "Stop"
$PSNativeCommandUseErrorActionPreference = $false

Set-Location $Repo

if (-not (Test-Path "src/pc/pc_main.c")) {
    throw "Esegui questo script dentro app\jni\src (oppure passa -Repo con quel percorso)."
}

Write-Host "== SM64 Android 60 FPS interpolation setup =="

$controllerChanges = git status --short -- src/pc/controller
if ($LASTEXITCODE -ne 0) { throw "git status failed" }

Write-Host "`nController changes currently present:"
if ($controllerChanges) {
    $controllerChanges | ForEach-Object { Write-Host $_ }
} else {
    Write-Host "(none)"
}

$currentBranch = git symbolic-ref --short -q HEAD 2>$null
if ([string]::IsNullOrWhiteSpace(($currentBranch -join ""))) {
    $currentBranch = ""
}

$targetBranch = "android-60fps-interpolation"

if ($currentBranch -ne $targetBranch) {
    $branchExists = git branch --list $targetBranch

    if ([string]::IsNullOrWhiteSpace(($branchExists -join ""))) {
        Write-Host "`nCreating $targetBranch from current commit..."
        git switch -c $targetBranch
    } else {
        Write-Host "`nSwitching to existing $targetBranch..."
        git switch $targetBranch
    }

    if ($LASTEXITCODE -ne 0) {
        throw "Unable to switch/create $targetBranch."
    }
}

$patchUrl = "https://raw.githubusercontent.com/sm64-port/sm64-port/master/enhancements/60fps.patch"
$patchPath = Join-Path (Get-Location) "60fps.android.base.patch"
$exclude = "src/menu/intro_geo.c"

Write-Host "`nDownloading upstream 60 FPS interpolation patch..."
Invoke-WebRequest $patchUrl -OutFile $patchPath

Write-Host "`nChecking patch compatibility (excluding legacy intro_geo.c)..."

# Use cmd.exe so stderr is captured as ordinary text and does not become a PowerShell NativeCommandError.
$checkCmd = "git apply --check --verbose --exclude=$exclude `"$patchPath`" 2^>^&1"
$checkOutput = cmd /d /c $checkCmd
$checkExit = $LASTEXITCODE

$checkOutput | ForEach-Object { Write-Host $_ }

if ($checkExit -ne 0) {
    Write-Host ""
    Write-Host "STOP: ci sono altre incompatibilita nella patch."
    Write-Host "Nessuna modifica della patch e stata applicata."
    Write-Host "Incollami tutto l'output qui sopra."
    exit 2
}

Write-Host "`nCompatibility check passed."
Write-Host "Applying interpolation patch..."

$applyCmd = "git apply --verbose --exclude=$exclude `"$patchPath`" 2^>^&1"
$applyOutput = cmd /d /c $applyCmd
$applyExit = $LASTEXITCODE

$applyOutput | ForEach-Object { Write-Host $_ }

if ($applyExit -ne 0) {
    throw "git apply failed unexpectedly."
}

Write-Host "`nPatch applied. Controller files were not targeted."
Write-Host ""
Write-Host "Changed files:"
git status --short

Write-Host ""
Write-Host "IMPORTANT:"
Write-Host "- SM64 simulation remains 30 Hz."
Write-Host "- Rendering interpolation supplies 60 visual frames."
Write-Host "- src/menu/intro_geo.c is intentionally left untouched for now."
Write-Host "- Do NOT add a second gfx_run loop and do NOT change game logic to 60 Hz."
Write-Host ""
Write-Host "Next: compile. If there is a compiler/linker error, send the FIRST error block."