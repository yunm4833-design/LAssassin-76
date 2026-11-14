$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
cmake -S $projectRoot -B "$projectRoot/build-msvc" -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=OFF -DLASSASSIN_BUILD_STANDALONE=OFF
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
cmake --build "$projectRoot/build-msvc" --config Release --target LAssassin76_VST3 --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'VST3 build failed' }
Write-Host "Plugin: $projectRoot/build-msvc/LAssassin76_artefacts/Release/VST3/LAssassin-76.vst3"
