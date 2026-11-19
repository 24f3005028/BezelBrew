# Per-user integration; stable legacy launch support during the BezelBrew rename.
param([switch]$Remove)
$ErrorActionPreference = 'Stop'
$executable = Join-Path $PSScriptRoot 'dist\BezelBrew.exe'
$legacyExecutable = Join-Path $PSScriptRoot 'dist\WinPixel.exe'
$startupFolder = [Environment]::GetFolderPath('Startup')
$programsFolder = [Environment]::GetFolderPath('Programs')
$shell = New-Object -ComObject WScript.Shell
$entries = @(@{Folder=$startupFolder;Arguments='';Hotkey=''},@{Folder=$programsFolder;Arguments='--toggle';Hotkey='CTRL+ALT+N'})
if (!$Remove -and !(Test-Path -LiteralPath $executable)) {throw 'Build dist\BezelBrew.exe first.'}
# Validate both destination links before making any changes.
foreach ($entry in $entries) {
    $destination = Join-Path $entry.Folder 'BezelBrew.lnk'
    if (Test-Path -LiteralPath $destination) {
        $target=$shell.CreateShortcut($destination).TargetPath
        if ($target -ne $executable -and $target -ne $legacyExecutable) {throw "An unrelated shortcut already exists at $destination. It was left untouched."}
    }
}
foreach ($entry in $entries) {
    $destination=Join-Path $entry.Folder 'BezelBrew.lnk'
    if (!$Remove) {
        $shortcut=$shell.CreateShortcut($destination)
        $shortcut.TargetPath=$executable
        $shortcut.Arguments=$entry.Arguments
        $shortcut.WorkingDirectory=Join-Path $PSScriptRoot 'dist'
        $shortcut.Hotkey=$entry.Hotkey
        $shortcut.Description='BezelBrew - Ctrl+Alt+N opens or hides the island'
        $shortcut.WindowStyle=7
        $shortcut.Save()
    } elseif (Test-Path -LiteralPath $destination) {Remove-Item -LiteralPath $destination}
    # Retire only this project's former shortcuts, preserving unrelated links.
    $legacy=Join-Path $entry.Folder 'WinPixel.lnk'
    if (Test-Path -LiteralPath $legacy) {
        $target=$shell.CreateShortcut($legacy).TargetPath
        if ($target -eq $executable -or $target -eq $legacyExecutable) {Remove-Item -LiteralPath $legacy}
    }
}
# The new Startup shortcut replaces only this project's previous Run values.
$runPath='HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$runValues=if (Test-Path -LiteralPath $runPath) {Get-ItemProperty -LiteralPath $runPath} else {$null}
foreach ($name in @('WinPixel','BezelBrew')) {
    $property=if ($runValues) {$runValues.PSObject.Properties[$name]} else {$null}
    $value=if ($property) {$property.Value} else {$null}
    if ($value -eq $executable -or $value -eq $legacyExecutable -or $value -eq ('"'+$executable+'"') -or $value -eq ('"'+$legacyExecutable+'"')) {
        Remove-ItemProperty -LiteralPath $runPath -Name $name
    }
}
if ($Remove) {Write-Output 'Removed owned BezelBrew/WinPixel startup and launch shortcuts.'}
else {[pscustomobject]@{StartupShortcut=(Join-Path $startupFolder 'BezelBrew.lnk');LaunchShortcut=(Join-Path $programsFolder 'BezelBrew.lnk');Hotkey='Ctrl+Alt+N';Executable=$executable} | ConvertTo-Json}
