# Build, upload, and monitor both gateway_tbeam and node_tbeam in parallel.
# Each device's serial output goes to its own timestamped log file.
#
# Phases:
#   1. pio run  (compile)  — both envs in parallel
#   2. pio run -t upload   — both envs in parallel (each to its own port)
#   3. pio device monitor  — both in parallel, timestamped, per-device logs
#
# Usage:
#   .\upload_monitor.ps1                                          # COM3 / COM4
#   .\upload_monitor.ps1 -GatewayPort COM5 -NodePort COM7
#   .\upload_monitor.ps1 -SkipUpload                              # monitor only
#   .\upload_monitor.ps1 -Baud 115200                             # override baud
#
# Ctrl-C stops everything cleanly.

#Requires -Version 5.1
[CmdletBinding()]
param(
    [string]$GatewayPort = "COM3",
    [string]$NodePort    = "COM4",
    [int]   $Baud        = 460800,
    [switch]$SkipUpload
)

$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot

# Always use PlatformIO's own venv. A bare `pio` resolves via PATH and can
# pick up a pio.exe wired to a different Python (e.g. D:\Programs\Python 3.13).
# PlatformIO injects penv/Lib/site-packages onto sys.path; its `littlefs`
# ships a cp311-only .pyd, so a 3.13 interpreter fails with a misleading
# "cannot import name 'lfs'" error inside the espressif32 builder.
$PioExe = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\pio.exe'
if (-not (Test-Path -LiteralPath $PioExe)) {
    throw "PlatformIO venv pio.exe not found at $PioExe"
}

$stamp   = Get-Date -Format 'yyyyMMdd_HHmmss'
$logsDir = Join-Path $PSScriptRoot "logs\$stamp"
$null = New-Item -ItemType Directory -Force -Path $logsDir
$gwLog    = Join-Path $logsDir "loramesher_gw.log"
$ndLog    = Join-Path $logsDir "loramesher_nd.log"
$gwBuildLog  = Join-Path $logsDir "build_gw.log"
$ndBuildLog  = Join-Path $logsDir "build_nd.log"
$gwUploadLog = Join-Path $logsDir "upload_gw.log"
$ndUploadLog = Join-Path $logsDir "upload_nd.log"

Write-Host "Session logs: $logsDir"
Write-Host "Gateway: gateway_tbeam on $GatewayPort"
Write-Host "Node:    node_tbeam    on $NodePort"
Write-Host "Baud:    $Baud"

# UTF-8 everywhere. Without this, pio prints a box-drawing char, Python
# encodes through cp1252, UnicodeEncodeError kills pio's stdout thread, and
# the upload hangs forever.
function Set-Utf8Console {
    $env:PYTHONIOENCODING = 'utf-8'
    $env:PYTHONUTF8       = '1'
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    $global:OutputEncoding    = [System.Text.Encoding]::UTF8
}
Set-Utf8Console

# Run pio in a background job so we can capture $LASTEXITCODE reliably.
# (Start-Process -PassThru + redirection drops ExitCode on some PS versions.)
# Use a single StreamWriter per file — Add-Content opens/closes on every line
# and loses the race against pio's own build output under Windows file locking.
$pioJobBlock = {
    param($wd, $pioExe, $pioArgs, $logPath)
    Set-Location -LiteralPath $wd
    $env:PYTHONIOENCODING = 'utf-8'
    $env:PYTHONUTF8       = '1'
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    $OutputEncoding           = [System.Text.Encoding]::UTF8
    $utf8NoBom = [System.Text.UTF8Encoding]::new($false)
    $writer = [System.IO.StreamWriter]::new($logPath, $false, $utf8NoBom)
    $writer.AutoFlush = $true
    try {
        & $pioExe @pioArgs 2>&1 | ForEach-Object { $writer.WriteLine([string]$_) }
    } finally {
        $writer.Dispose()
    }
    $LASTEXITCODE
}

function Wait-PioJobs {
    param($Jobs, $Labels, $Phase)
    Wait-Job $Jobs | Out-Null
    for ($i = 0; $i -lt $Jobs.Count; $i++) {
        $out = @(Receive-Job $Jobs[$i])
        $rc  = [int]$out[-1]
        Remove-Job $Jobs[$i]
        if ($rc -ne 0) { throw "$($Labels[$i]) $Phase failed (exit $rc) - check per-phase log" }
    }
}

if (-not $SkipUpload) {
    Write-Host "--- Phase 1: compile both envs in parallel ---"
    $gwBuild = Start-Job -Name 'build-GW' -ScriptBlock $pioJobBlock -ArgumentList `
        $PSScriptRoot, $PioExe, @('run','-e','gateway_tbeam'), $gwBuildLog
    $ndBuild = Start-Job -Name 'build-ND' -ScriptBlock $pioJobBlock -ArgumentList `
        $PSScriptRoot, $PioExe, @('run','-e','node_tbeam'),    $ndBuildLog
    Wait-PioJobs @($gwBuild, $ndBuild) @('Gateway','Node') 'build'
    Write-Host "Both builds succeeded."

    Write-Host "--- Phase 2: upload both in parallel ---"
    $gwUp = Start-Job -Name 'upload-GW' -ScriptBlock $pioJobBlock -ArgumentList `
        $PSScriptRoot, $PioExe, @('run','-e','gateway_tbeam','-t','upload','--upload-port',$GatewayPort), $gwUploadLog
    $ndUp = Start-Job -Name 'upload-ND' -ScriptBlock $pioJobBlock -ArgumentList `
        $PSScriptRoot, $PioExe, @('run','-e','node_tbeam',   '-t','upload','--upload-port',$NodePort),    $ndUploadLog
    Wait-PioJobs @($gwUp, $ndUp) @('Gateway','Node') 'upload'
    Write-Host "Both uploads succeeded."
}

$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$gwWriter  = [System.IO.StreamWriter]::new($gwLog, $true, $utf8NoBom)
$ndWriter  = [System.IO.StreamWriter]::new($ndLog, $true, $utf8NoBom)
$gwWriter.AutoFlush = $true
$ndWriter.AutoFlush = $true
$sessionTs = Get-Date -Format '[yyyy-MM-dd HH:mm:ss.fff]'
$gwWriter.WriteLine("$sessionTs # session started on $GatewayPort @ $Baud")
$ndWriter.WriteLine("$sessionTs # session started on $NodePort @ $Baud")

Write-Host "--- Phase 3: monitoring (Ctrl-C to stop) ---"

# Each background job runs pio device monitor with an explicit baud.
$monitorBlock = {
    param($pioExe, $port, $baud)
    $env:PYTHONIOENCODING = 'utf-8'
    $env:PYTHONUTF8       = '1'
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    $OutputEncoding           = [System.Text.Encoding]::UTF8
    & $pioExe device monitor --port $port -b $baud 2>&1
}
$gwJob = Start-Job -Name 'mon-GW' -ScriptBlock $monitorBlock -ArgumentList $PioExe, $GatewayPort, $Baud
$ndJob = Start-Job -Name 'mon-ND' -ScriptBlock $monitorBlock -ArgumentList $PioExe, $NodePort,    $Baud

# Recursively kill every descendant of the given PID. Needed because Stop-Job
# only terminates the per-job pwsh runspace, not its pio.exe / python.exe
# grandchildren that actually hold the COM port.
function Stop-ProcessTree {
    param([int]$ParentId)
    Get-CimInstance Win32_Process -Filter "ParentProcessId=$ParentId" -ErrorAction SilentlyContinue | ForEach-Object {
        Stop-ProcessTree -ParentId $_.ProcessId
        Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue
    }
}

# Ctrl-C becomes a normal key press instead of a pipeline-terminating signal,
# so the finally block is guaranteed to run.
[Console]::TreatControlCAsInput = $true
$stopRequested = $false

try {
    while (-not $stopRequested) {
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            if (($key.Modifiers -band [ConsoleModifiers]::Control) -and ($key.Key -eq 'C')) {
                $stopRequested = $true
                break
            }
        }

        foreach ($entry in @(@{Job=$gwJob; Tag='GW'; Writer=$gwWriter}, @{Job=$ndJob; Tag='ND'; Writer=$ndWriter})) {
            $lines = Receive-Job -Job $entry.Job
            foreach ($line in $lines) {
                $text = [string]$line
                if ([string]::IsNullOrEmpty($text)) { continue }
                # pio's printable filter rewrites 0x1B to U+241B (␛) on Windows;
                # undo it so ANSI strippers downstream (e.g. log_analyzer.html) match.
                $text = $text.Replace([char]0x241B, [char]0x1B)
                $ts = Get-Date -Format '[yyyy-MM-dd HH:mm:ss.fff]'
                $entry.Writer.WriteLine("$ts $text")
                Write-Host "$ts [$($entry.Tag)] $text"
            }
        }
        if ($gwJob.State -ne 'Running' -and $ndJob.State -ne 'Running') { break }
        Start-Sleep -Milliseconds 200
    }
}
finally {
    [Console]::TreatControlCAsInput = $false
    Write-Host ""
    Write-Host "--- Stopping ---"
    # Kill grandchildren (pio.exe / python.exe) before the jobs — otherwise they
    # hold COM handles and the next run fails with "port already open".
    Stop-ProcessTree -ParentId $PID
    Stop-Job   $gwJob, $ndJob -ErrorAction SilentlyContinue
    Remove-Job $gwJob, $ndJob -Force -ErrorAction SilentlyContinue
    if ($gwWriter) { $gwWriter.Dispose() }
    if ($ndWriter) { $ndWriter.Dispose() }
    Write-Host "Stopped."
}
