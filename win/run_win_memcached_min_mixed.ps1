[CmdletBinding()]
param(
    [string]$OutputDir,
    [string]$RawOutputDir,
    [string]$ExePath,
    [int]$Port = 11430,
    [string]$Listen = "127.0.0.1",
    [int]$ServerThreads = 4,
    [int]$MemMb = 64,
    [int]$Clients = 4,
    [int]$RequestsPerClient = 4000,
    [int]$PayloadSize = 32,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\apps"
}
if (-not $RawOutputDir) {
    $RawOutputDir = Join-Path $RepoRoot "private\raw-results\windows\memcached\mixed"
}
if (-not $ExePath) {
    $ExePath = Join-Path $RepoRoot "out_win_memcached\memcached_win_min_main.exe"
}

foreach ($path in @($OutputDir, $RawOutputDir)) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Force -Path $path | Out-Null
    }
}

if (-not $SkipBuild) {
    $BuildScript = Join-Path $RepoRoot "win\build_win_memcached_min_main.ps1"
    & $BuildScript
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_memcached_min_main.ps1 failed with exit code $LASTEXITCODE"
    }
}

if (-not (Test-Path $ExePath)) {
    throw "memcached minimal main exe not found: $ExePath"
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_memcached_min_mixed.md")
$RawLogPath = Join-Path $RawOutputDir ($Stamp + "_memcached_min_mixed.log")
$StdoutPath = Join-Path $RawOutputDir ($Stamp + "_memcached_min_mixed_stdout.log")
$StderrPath = Join-Path $RawOutputDir ($Stamp + "_memcached_min_mixed_stderr.log")

if (-not ("HzMemcachedMiniMixedBench" -as [type])) {
    Add-Type -Language CSharp -TypeDefinition @"
using System;
using System.Diagnostics;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

public sealed class HzMixedBenchResult {
    public int TotalOps { get; set; }
    public int SetOps { get; set; }
    public int GetOps { get; set; }
    public double ElapsedSeconds { get; set; }
    public double OpsPerSecond { get; set; }
}

public static class HzMemcachedMiniMixedBench {
    private static string ReadGetResponse(StreamReader reader) {
        string header = reader.ReadLine();
        if (header == null) {
            throw new IOException("unexpected EOF while reading VALUE header");
        }
        if (header == "END") {
            throw new IOException("cache miss during mixed benchmark");
        }
        if (!header.StartsWith("VALUE ", StringComparison.Ordinal)) {
            throw new IOException("unexpected get header: " + header);
        }
        string value = reader.ReadLine();
        string end = reader.ReadLine();
        if (value == null || end == null) {
            throw new IOException("unexpected EOF while reading get response body");
        }
        if (end != "END") {
            throw new IOException("unexpected get terminator: " + end);
        }
        return value;
    }

    private static void PreloadWorker(string host, int port, int clientId, int requestsPerClient, int keySpace, string payload) {
        using (TcpClient client = new TcpClient()) {
            client.NoDelay = true;
            client.ReceiveTimeout = 5000;
            client.SendTimeout = 5000;
            client.Connect(host, port);
            using (NetworkStream stream = client.GetStream())
            using (StreamWriter writer = new StreamWriter(stream, new UTF8Encoding(false), 4096, true))
            using (StreamReader reader = new StreamReader(stream, Encoding.ASCII, false, 4096, true)) {
                writer.NewLine = "\r\n";
                writer.AutoFlush = true;
                for (int i = 0; i < requestsPerClient; i++) {
                    int keyId = ((clientId * requestsPerClient) + i) % keySpace;
                    string key = "k" + keyId.ToString();
                    writer.Write("set ");
                    writer.Write(key);
                    writer.Write(" 0 0 ");
                    writer.Write(payload.Length.ToString());
                    writer.Write("\r\n");
                    writer.Write(payload);
                    writer.Write("\r\n");
                    string line = reader.ReadLine();
                    if (line != "STORED") {
                        throw new IOException("unexpected preload set response: " + line);
                    }
                }
                writer.Write("quit\r\n");
                writer.Flush();
            }
        }
    }

    private static void MixedWorker(string host, int port, int clientId, int requestsPerClient, int keySpace, string payload) {
        using (TcpClient client = new TcpClient()) {
            client.NoDelay = true;
            client.ReceiveTimeout = 5000;
            client.SendTimeout = 5000;
            client.Connect(host, port);
            using (NetworkStream stream = client.GetStream())
            using (StreamWriter writer = new StreamWriter(stream, new UTF8Encoding(false), 4096, true))
            using (StreamReader reader = new StreamReader(stream, Encoding.ASCII, false, 4096, true)) {
                writer.NewLine = "\r\n";
                writer.AutoFlush = true;
                for (int i = 0; i < requestsPerClient; i++) {
                    int keyId = ((clientId * requestsPerClient) + i) % keySpace;
                    string key = "k" + keyId.ToString();
                    if ((i % 2) == 0) {
                        writer.Write("set ");
                        writer.Write(key);
                        writer.Write(" 0 0 ");
                        writer.Write(payload.Length.ToString());
                        writer.Write("\r\n");
                        writer.Write(payload);
                        writer.Write("\r\n");
                        string line = reader.ReadLine();
                        if (line != "STORED") {
                            throw new IOException("unexpected mixed set response: " + line);
                        }
                    } else {
                        writer.Write("get ");
                        writer.Write(key);
                        writer.Write("\r\n");
                        string value = ReadGetResponse(reader);
                        if (value != payload) {
                            throw new IOException("unexpected mixed get payload");
                        }
                    }
                }
                writer.Write("quit\r\n");
                writer.Flush();
            }
        }
    }

    public static void Preload(string host, int port, int clients, int requestsPerClient, string payload) {
        int keySpace = clients * requestsPerClient;
        Task[] tasks = new Task[clients];
        for (int i = 0; i < clients; i++) {
            int clientId = i;
            tasks[i] = Task.Run(() => PreloadWorker(host, port, clientId, requestsPerClient, keySpace, payload));
        }
        Task.WaitAll(tasks);
    }

    public static HzMixedBenchResult RunMixed(string host, int port, int clients, int requestsPerClient, string payload) {
        int keySpace = clients * requestsPerClient;
        Stopwatch sw = Stopwatch.StartNew();
        Task[] tasks = new Task[clients];
        for (int i = 0; i < clients; i++) {
            int clientId = i;
            tasks[i] = Task.Run(() => MixedWorker(host, port, clientId, requestsPerClient, keySpace, payload));
        }
        Task.WaitAll(tasks);
        sw.Stop();
        int totalOps = clients * requestsPerClient;
        return new HzMixedBenchResult {
            TotalOps = totalOps,
            SetOps = totalOps / 2,
            GetOps = totalOps / 2,
            ElapsedSeconds = sw.Elapsed.TotalSeconds,
            OpsPerSecond = totalOps / sw.Elapsed.TotalSeconds
        };
    }
}
"@
}

$payloadBuilder = New-Object System.Text.StringBuilder
while ($payloadBuilder.Length -lt $PayloadSize) {
    [void]$payloadBuilder.Append("0123456789abcdefghijklmnopqrstuvwxyz")
}
$Payload = $payloadBuilder.ToString().Substring(0, $PayloadSize)

$server = $null
$raw = New-Object System.Collections.Generic.List[string]
$summary = New-Object System.Collections.Generic.List[string]

try {
    Remove-Item $StdoutPath, $StderrPath -ErrorAction SilentlyContinue
    $serverArgs = @(
        "-p", [string]$Port,
        "-l", $Listen,
        "-t", [string]$ServerThreads,
        "-m", [string]$MemMb
    )
    $server = Start-Process -FilePath $ExePath -ArgumentList $serverArgs `
        -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath -PassThru
    Start-Sleep -Seconds 1

    [HzMemcachedMiniMixedBench]::Preload($Listen, $Port, $Clients, $RequestsPerClient, $Payload)
    $mixedResult = [HzMemcachedMiniMixedBench]::RunMixed($Listen, $Port, $Clients, $RequestsPerClient, $Payload)

    $raw.Add("exe: $ExePath")
    $raw.Add("args: " + ($serverArgs -join " "))
    $raw.Add(("clients: {0}" -f $Clients))
    $raw.Add(("requests_per_client: {0}" -f $RequestsPerClient))
    $raw.Add(("payload_size: {0}" -f $PayloadSize))
    $raw.Add(("mixed_ops_per_sec: {0:N2}" -f $mixedResult.OpsPerSecond))
    $raw.Add("")

    $summary.Add("# Windows Memcached Min Mixed")
    $summary.Add("")
    $summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
    $summary.Add("")
    $summary.Add("References:")
    $summary.Add("- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)")
    $summary.Add("- [win/run_win_memcached_min_mixed.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_mixed.ps1)")
    $summary.Add("- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)")
    $summary.Add("")
    $summary.Add("| total ops | set ops | get ops | elapsed sec | ops/sec |")
    $summary.Add("| ---: | ---: | ---: | ---: | ---: |")
    $summary.Add(('| {0} | {1} | {2} | {3:N3} | {4:N2} |' -f `
        $mixedResult.TotalOps, $mixedResult.SetOps, $mixedResult.GetOps, $mixedResult.ElapsedSeconds, $mixedResult.OpsPerSecond))
    $summary.Add("")
    $summary.Add("Config:")
    $summary.Add(('- exe: `{0}`' -f $ExePath))
    $summary.Add(('- listen: `{0}:{1}`' -f $Listen, $Port))
    $summary.Add(('- server_threads: `{0}`' -f $ServerThreads))
    $summary.Add(('- clients: `{0}`' -f $Clients))
    $summary.Add(('- requests_per_client: `{0}`' -f $RequestsPerClient))
    $summary.Add(('- payload_size: `{0}`' -f $PayloadSize))
    $summary.Add("")
    $summary.Add(('Raw log: `{0}`' -f $RawLogPath))
} finally {
    if ($server -and -not $server.HasExited) {
        Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 200
    }
    if (Test-Path $StdoutPath) {
        $raw.Add("stdout:")
        foreach ($line in (Get-Content $StdoutPath)) {
            $raw.Add($line)
        }
    }
    if (Test-Path $StderrPath) {
        $raw.Add("stderr:")
        foreach ($line in (Get-Content $StderrPath)) {
            $raw.Add($line)
        }
    }
    Set-Content -Path $RawLogPath -Value $raw -Encoding UTF8
    if ($summary.Count -gt 0) {
        Set-Content -Path $SummaryPath -Value $summary -Encoding UTF8
        Write-Host "Wrote summary: $SummaryPath"
    }
    Write-Host "Wrote raw log: $RawLogPath"
}
