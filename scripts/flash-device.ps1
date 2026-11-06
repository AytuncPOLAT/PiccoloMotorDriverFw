<#
.SYNOPSIS
Safely flashes a PiccoloMotorDriverFW ELF to an STM32H723VG with J-Link.

.PARAMETER Firmware
Path to the firmware ELF. Defaults to build/Release/PiccoloMotorDriver.elf.

.EXAMPLE
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\flash-device.ps1 -Firmware .\build\Release\PiccoloMotorDriver.elf

.EXAMPLE
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\flash-device.ps1 -Firmware .\build\Release\PiccoloMotorDriver.elf -WhatIf
#>
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$Firmware = (Join-Path (Split-Path $PSScriptRoot -Parent) "build\Release\PiccoloMotorDriver.elf"),
    [ValidateRange(100, 12000)]
    [int]$SpeedKHz = 4000
)

$ErrorActionPreference = "Stop"

$device = "STM32H723VG"
$flashStart = [UInt64]0x08000000
$flashLimit = [UInt64]0x08020000
$configStart = [UInt64]0x080E0000
$configSize = [UInt64]0x20000
$firmwarePath = (Resolve-Path -LiteralPath $Firmware).Path

$objdump = Get-Command arm-none-eabi-objdump.exe -ErrorAction Stop
$objcopy = Get-Command arm-none-eabi-objcopy.exe -ErrorAction Stop
$sectionOutput = & $objdump.Source -h $firmwarePath
if ($LASTEXITCODE -ne 0) {
    throw "Failed to inspect firmware ELF: $firmwarePath"
}

$loadableSections = 0
$vectorTableFound = $false
$imageLimit = $flashStart
$sectionPattern = '^\s*\d+\s+(\S+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+[0-9A-Fa-f]+'
for ($index = 0; $index -lt $sectionOutput.Count; $index++) {
    $sectionMatch = [regex]::Match($sectionOutput[$index], $sectionPattern)
    if (-not $sectionMatch.Success -or $index + 1 -ge $sectionOutput.Count) {
        continue
    }

    $flags = $sectionOutput[$index + 1]
    if ($flags -notmatch "CONTENTS" -or $flags -notmatch "LOAD") {
        continue
    }

    $sectionName = $sectionMatch.Groups[1].Value
    $sectionSize = [Convert]::ToUInt64($sectionMatch.Groups[2].Value, 16)
    $loadAddress = [Convert]::ToUInt64($sectionMatch.Groups[4].Value, 16)
    $sectionLimit = $loadAddress + $sectionSize
    if ($sectionSize -eq 0) {
        continue
    }
    if ($loadAddress -lt $flashStart -or $sectionLimit -gt $flashLimit) {
        throw "Loadable section '$sectionName' at 0x$($loadAddress.ToString('X8')) exceeds firmware sector 0. No target was touched."
    }

    $loadableSections++
    if ($sectionLimit -gt $imageLimit) {
        $imageLimit = $sectionLimit
    }
    if ($sectionName -eq ".isr_vector" -and $loadAddress -eq $flashStart) {
        $vectorTableFound = $true
    }
}

if ($loadableSections -eq 0 -or -not $vectorTableFound) {
    throw "ELF has no valid loadable vector table at 0x08000000. No target was touched."
}

$imageSize = $imageLimit - $flashStart
Write-Output ("Firmware load range: 0x{0:X8}-0x{1:X8} ({2} bytes)" -f $flashStart, ($imageLimit - 1), $imageSize)
Write-Output "Erase range: 0x08000000-0x0801FFFF only; configuration sector 7 will be backed up and verified."

if ($PSCmdlet.ShouldProcess($device, "Back up sector 7, erase sector 0, flash and verify firmware")) {
    $jlink = Get-Command JLink.exe -ErrorAction Stop
    $workDirectory = Join-Path $env:TEMP ("PiccoloMotorDriverFW-flash-" + [guid]::NewGuid().ToString("N"))
    $null = New-Item -ItemType Directory -Path $workDirectory
    $binaryPath = Join-Path $workDirectory "firmware.bin"
    $configBackup = Join-Path $workDirectory "sector7-before.bin"
    $commandFile = Join-Path $workDirectory "flash.jlink"

    & $objcopy.Source -O binary $firmwarePath $binaryPath
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to convert firmware ELF to binary. No target was touched."
    }
    if ((Get-Item -LiteralPath $binaryPath).Length -ne $imageSize) {
        throw "Binary image size does not match the validated ELF load range. No target was touched."
    }

    $commands = @(
        "connect",
        "r",
        "h",
        "savebin `"$configBackup`" 0x080E0000 0x20000",
        "erase 0x08000000 0x0801FFFF noreset",
        "loadfile `"$binaryPath`" 0x08000000 noreset",
        "verifybin `"$binaryPath`" 0x08000000",
        "verifybin `"$configBackup`" 0x080E0000",
        "r",
        "g",
        "exit"
    )
    [System.IO.File]::WriteAllLines($commandFile, $commands, [System.Text.Encoding]::ASCII)

    & $jlink.Source -NoGui 1 -ExitOnError 1 -device $device -if SWD -speed $SpeedKHz -CommandFile $commandFile
    if ($LASTEXITCODE -ne 0) {
        throw "J-Link failed. Configuration backup and command log directory retained at: $workDirectory"
    }

    Remove-Item -LiteralPath $binaryPath, $commandFile -Force
    Write-Output "Flash and verification completed. Sector 7 backup retained at: $configBackup"
}
else {
    Write-Output "Preflight passed. No target connection, erase, or programming was performed."
}