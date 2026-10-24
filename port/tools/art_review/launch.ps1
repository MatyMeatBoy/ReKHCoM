param([switch]$NoBrowser)
$ErrorActionPreference = 'Stop'
$taskMutex = [System.Threading.Mutex]::new($false, 'Local\ReKHCoM-ArtReview-8766')
if (-not $taskMutex.WaitOne(30000)) { throw 'El launcher ya esta actualizando la mesa. Intentalo en unos segundos.' }
function Start-UnifiedReview {
$taskProject = Split-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) -Parent
$taskData = Join-Path $taskProject 'build\art-review'
$taskUrl = 'http://127.0.0.1:8766'
$taskVenv = Join-Path $taskData '.venv\Scripts\python.exe'
$taskVersionPython = if (Test-Path -LiteralPath $taskVenv) { $taskVenv } else { (Get-Command python -ErrorAction Stop).Source }
$taskDiskBuild = (& $taskVersionPython (Join-Path $PSScriptRoot 'runtime_version.py')).Trim()
function Open-ReviewDesk($taskState) {
    if ($NoBrowser) { return }
    $taskLatest = @($taskState.assets) | Where-Object id -eq $taskState.latest_delivery_id | Select-Object -First 1
    if (-not $taskLatest) { $taskLatest = @($taskState.assets) | Select-Object -Last 1 }
    $taskTarget = if ($taskLatest) { "$taskUrl/?image=$($taskLatest.id)&latest=1&app=$taskDiskBuild" } else { "$taskUrl/?latest=1&app=$taskDiskBuild" }
    Start-Process $taskTarget
}
try {
    $taskExisting = Invoke-RestMethod "$taskUrl/api/state" -TimeoutSec 2
    if ($taskExisting.data_dir -ne $taskData) { throw 'Puerto 8766 ocupado por otra mesa de revision.' }
    if ($taskExisting.app_build -eq $taskDiskBuild) {
        Open-ReviewDesk $taskExisting
        exit 0
    }
    if ($taskExisting.auto_restart) {
        for ($taskAutoTry=0; $taskAutoTry -lt 50; $taskAutoTry++) {
            Start-Sleep -Milliseconds 200
            try {
                $taskAutoState = Invoke-RestMethod "$taskUrl/api/state" -TimeoutSec 1
                if ($taskAutoState.data_dir -eq $taskData -and $taskAutoState.app_build -eq $taskDiskBuild) {
                    Open-ReviewDesk $taskAutoState
                    return
                }
            } catch { }
        }
        throw 'La actualizacion del servidor unico sigue pendiente; no se inicio otro servidor.'
    }
    # Restart only this exact local app, never an unrelated occupant of the port.
    $taskListener = Get-NetTCPConnection -LocalPort 8766 -State Listen -ErrorAction Stop | Where-Object LocalAddress -eq '127.0.0.1' | Select-Object -First 1
    $taskOwner = Get-CimInstance Win32_Process -Filter "ProcessId=$($taskListener.OwningProcess)"
    $taskServerPath = Join-Path $PSScriptRoot 'server.py'
    if (-not $taskOwner.CommandLine.Contains($taskServerPath)) { throw 'No se pudo verificar el servidor existente; no se detuvo otro proceso.' }
    Stop-Process -Id $taskOwner.ProcessId -ErrorAction Stop
    for ($taskWait=0; $taskWait -lt 30; $taskWait++) {
        if (-not (Get-NetTCPConnection -LocalPort 8766 -State Listen -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Milliseconds 100
    }
} catch {
    if ($_.Exception.Message -like '*ocupado*' -or $_.Exception.Message -like '*verificar*' -or $_.Exception.Message -like '*servidor unico*') { throw }
}
New-Item -ItemType Directory -Path $taskData -Force | Out-Null
if (-not (Test-Path -LiteralPath $taskVenv)) {
    $taskPython = (Get-Command python -ErrorAction Stop).Source
    & $taskPython -m venv (Join-Path $taskData '.venv')
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo crear el entorno Python.' }
    & $taskVenv -m pip install -r (Join-Path $PSScriptRoot 'requirements.txt')
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo instalar Pillow. Revisa la conexion e intentalo otra vez.' }
}
$taskArgs = '"' + (Join-Path $PSScriptRoot 'server.py') + '" --data "' + $taskData + '"'
$taskProcess = Start-Process -FilePath $taskVenv -ArgumentList $taskArgs -WorkingDirectory $taskProject -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskData 'server.log') -RedirectStandardError (Join-Path $taskData 'server-error.log')
for ($taskTry = 0; $taskTry -lt 40; $taskTry++) {
    Start-Sleep -Milliseconds 250
    try {
        $taskReady = Invoke-RestMethod "$taskUrl/api/state" -TimeoutSec 1
        if ($taskReady.data_dir -eq $taskData -and $taskReady.app_build -eq $taskDiskBuild) { Open-ReviewDesk $taskReady; exit 0 }
    } catch { }
    if ($taskProcess.HasExited) { break }
}
throw "No inicio la mesa. Revisa $taskData\server-error.log"
}
try { Start-UnifiedReview } finally { $taskMutex.ReleaseMutex(); $taskMutex.Dispose() }
