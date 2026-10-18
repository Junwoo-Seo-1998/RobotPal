param([Parameter(Mandatory=$true)][int[]]$ProcessId, [Parameter(Mandatory=$true)][string]$Output, [int]$IntervalMilliseconds=1000, [int]$DurationSeconds=70)
$rows = @(); $previous = @{}; $started = Get-Date
while (((Get-Date) - $started).TotalSeconds -lt $DurationSeconds) {
    $sampleStart = Get-Date; $totalCpu = 0.0; $workingSet = 0
    foreach ($id in $ProcessId) {
        $process = Get-Process -Id $id -ErrorAction SilentlyContinue
        if ($process) { $cpu = if ($null -eq $process.CPU) { 0.0 } else { [double]$process.CPU }; if ($previous.ContainsKey($id)) { $totalCpu += ($cpu - $previous[$id]) * 100.0 / ($IntervalMilliseconds / 1000.0) }; $previous[$id] = $cpu; $workingSet += $process.WorkingSet64 }
    }
    $rows += [pscustomobject]@{ timestamp=(Get-Date).ToUniversalTime().ToString('o'); cpu_percent=$totalCpu; working_set_bytes=$workingSet }
    $remaining = $IntervalMilliseconds - ((Get-Date) - $sampleStart).TotalMilliseconds; if ($remaining -gt 0) { Start-Sleep -Milliseconds ([int]$remaining) }
}
$rows | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath $Output
