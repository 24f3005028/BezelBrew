param([string]$Zig = "$PSScriptRoot\tools\ziglang\zig.exe", [string]$GoogleClientJson = "", [string]$SpotifyClientId = "")
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force "$PSScriptRoot\dist" | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR = "$PSScriptRoot\tools\zig-cache"
New-Item -ItemType Directory -Force "$PSScriptRoot\tools\tmp" | Out-Null
$env:TEMP = "$PSScriptRoot\tools\tmp"
$env:TMP = $env:TEMP
& $Zig rc /I "$PSScriptRoot\src" /fo "$PSScriptRoot\dist\WinPixel.res" "$PSScriptRoot\src\WinPixel.rc"
if ($LASTEXITCODE -ne 0) { throw "Resource compilation failed" }
$publisherFlags = @()
if ($GoogleClientJson) {
    $publisherFolder = Join-Path $PSScriptRoot 'tools\publisher-build'
    python "$PSScriptRoot\packaging\prepare_google.py" $GoogleClientJson "$publisherFolder\bezelbrew_google_generated.hpp"
    if ($LASTEXITCODE -ne 0) {throw 'Could not prepare publisher Google sign-in.'}
    $publisherFlags = @('-DBEZELBREW_PUBLISHER_GOOGLE=1', "-I$publisherFolder")
}
if ($SpotifyClientId) {
    if ($SpotifyClientId -cnotmatch '^[a-fA-F0-9]{32}$') { throw 'Spotify Client ID must be 32 hexadecimal characters.' }
    $publisherFlags += "-DBEZELBREW_SPOTIFY_CLIENT_ID=$SpotifyClientId"
}
& $Zig c++ @publisherFlags "$PSScriptRoot\src\main.cpp" "$PSScriptRoot\dist\WinPixel.res" -o "$PSScriptRoot\dist\BezelBrew.exe" -target x86_64-windows-gnu -O2 -s -municode "-Wl,--subsystem,windows" -luser32 -lcomctl32 -lcomdlg32 -luxtheme -ladvapi32 -lgdi32 -lgdiplus -ldwmapi -lwinmm -lshell32 -lshlwapi -lxmllite -lole32 -luuid -lwinhttp -lws2_32 -lbcrypt -lcrypt32 -nostdlib++ -fno-exceptions -fno-rtti
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }





# Compatibility entry point for existing links and automation; same binary, no helper.
Copy-Item -LiteralPath "$PSScriptRoot\dist\BezelBrew.exe" -Destination "$PSScriptRoot\dist\WinPixel.exe" -Force

Copy-Item -LiteralPath "$PSScriptRoot\docs\CONNECTORS.txt" -Destination "$PSScriptRoot\dist\Connectors.txt" -Force
Copy-Item -LiteralPath "$PSScriptRoot\packaging\GOOGLE_SIGN_IN.md" -Destination "$PSScriptRoot\dist\GooglePublisherSetup.txt" -Force
Copy-Item -LiteralPath "$PSScriptRoot\LICENSE" -Destination "$PSScriptRoot\dist\LICENSE.txt" -Force
