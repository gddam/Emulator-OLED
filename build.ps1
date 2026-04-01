param(
    [string]$Sketch = "../src/main.ino",
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$msysRoot = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { "C:\msys64" }
$profiles = @("ucrt64", "mingw64")
$selectedProfile = $null

foreach ($profile in $profiles) {
    $binDir = Join-Path $msysRoot "$profile\bin"
    $sdlConfig = Join-Path $msysRoot "$profile\lib\cmake\SDL2\SDL2Config.cmake"
    if ((Test-Path (Join-Path $binDir "c++.exe")) -and (Test-Path $sdlConfig)) {
        $selectedProfile = $profile
        break
    }
}

if (-not $selectedProfile) {
    throw "MSYS2 toolchain with SDL2 was not found under $msysRoot. Install SDL2 in ucrt64 or mingw64 first."
}

$usrBin = Join-Path $msysRoot "usr\bin"
$toolchainBin = Join-Path $msysRoot "$selectedProfile\bin"
$cmake = Join-Path $usrBin "cmake.exe"
$buildPath = Join-Path $scriptDir $BuildDir

if (-not (Test-Path $cmake)) {
    throw "cmake.exe was not found under $usrBin."
}

$env:PATH = "$usrBin;$toolchainBin;$env:PATH"

& $cmake `
    -S $scriptDir `
    -B $buildPath `
    -G Ninja `
    "-DCMAKE_PREFIX_PATH=$($msysRoot)\$selectedProfile" `
    "-DWOUOUI_SKETCH=$Sketch"

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $cmake --build $buildPath
exit $LASTEXITCODE
