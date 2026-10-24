$ErrorActionPreference = 'Stop'
$taskProject = Split-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) -Parent
$taskData = Join-Path $taskProject 'build\art-review'
$taskUrl = 'http://127.0.0.1:8766'
try {
    $taskExisting = Invoke-RestMethod "$taskUrl/api/state" -TimeoutSec 2
    if ($taskExisting.data_dir -ne $taskData) { throw 'Puerto 8766 ocupado por otra mesa de revision.' }
    Start-Process $taskUrl
    exit 0
} catch {
    if ($_.Exception.Message -like '*ocupado*') { throw }
}
New-Item -ItemType Directory -Path $taskData -Force | Out-Null
$taskVenv = Join-Path $taskData '.venv\Scripts\python.exe'
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
        if ($taskReady.data_dir -eq $taskData) { Start-Process $taskUrl; exit 0 }
    } catch { }
    if ($taskProcess.HasExited) { break }
}
throw "No inicio la mesa. Revisa $taskData\server-error.log"
