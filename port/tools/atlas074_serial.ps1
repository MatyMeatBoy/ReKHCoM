param([switch]$AssignedSlot)
$ErrorActionPreference = 'Stop'
if (-not $AssignedSlot) { throw 'Esperar turno serial asignado; invocar con -AssignedSlot solo en ese turno.' }
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
Push-Location $taskRoot
try {
    $env:SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT = '0xffff/0xffff'
    $env:SDL_VIDEO_DRIVER = 'dummy'
    $env:SDL_AUDIO_DRIVER = 'dummy'
    $env:KHCOM_PPU_THREADS = '1'
    $env:KHCOM_BACKGROUND_TEST = '1'
    & '../toolchain/bin/ninja.exe' -f build/pc-atlas/build.ninja -j1
    if ($LASTEXITCODE) { throw 'Build EU fallido' }
    $eventCode = @'
import sys
sys.path.insert(0,'port/tools')
from test_events import event_lists
print(next('%d:%d'%(l,i) for l,i,n in event_lists() if n.split('_')[1]=='074'))
'@
    $env:KHCOM_EVENT = $eventCode | python -
    if ($LASTEXITCODE -or -not $env:KHCOM_EVENT) { throw 'No se pudo resolver el evento 074' }
    $taskRun = Join-Path $taskRoot ('build/atlas074/runtime-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
    New-Item -ItemType Directory $taskRun | Out-Null
    Copy-Item build/pc-atlas/khcom.exe,build/pc-atlas/SDL3.dll $taskRun
    "config_version=11`nview_mode=4`nzoom=6`nvsync=0`nhd_text=0`n" | Set-Content (Join-Path $taskRun 'kh_com.ini')
    $env:KHCOM_FAST = '1'
    $env:KHCOM_WINDOW = '3840x1080'
    $env:KHCOM_PRELIMINAR_V1 = '1'
    $env:KHCOM_KEYS = ((30..299 | Where-Object { ($_ - 30) % 40 -eq 0 } | ForEach-Object { "$_-$($_+3):A" }) -join ',')
    $env:KHCOM_OUTPUT_SHOT = '300'
    $env:KHCOM_QUIT = '304'
    Push-Location $taskRun
    try {
        & ./khcom.exe --rom (Join-Path $taskRoot 'roms/B8CP.gba') *> runtime.log
        if ($LASTEXITCODE) { throw 'Captura EU fallida' }
        if (-not (Test-Path out_00300.bmp)) { throw 'Falta captura de frame 300' }
    } finally { Pop-Location }
    Write-Output $taskRun
} finally { Pop-Location }
