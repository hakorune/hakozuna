[CmdletBinding()]
param(
    [ValidateSet("read_heavy", "balanced", "write_heavy", "larger_payload")]
    [string]$Profile = "read_heavy",
    [string]$OutputDir,
    [string]$RawOutputDir,
    [string]$ExePath,
    [int]$Port = 11448,
    [string]$Listen = "127.0.0.1",
    [int]$ServerThreads = 4,
    [int]$MemMb = 64,
    [int]$Clients = 8,
    [int]$WarmupKeysPerClient = 1000,
    [int]$RequestsPerClient = 10000,
    [int]$PayloadSize = 64,
    [int]$GetPercent = -1,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\apps"
}
if (-not $RawOutputDir) {
    $RawOutputDir = Join-Path $RepoRoot "private\raw-results\windows\memcached\benchmark_client"
}
if (-not $ExePath) {
    $ExePath = Join-Path $RepoRoot "out_win_memcached\memcached_win_min_main.exe"
}

foreach ($path in @($OutputDir, $RawOutputDir)) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Force -Path $path | Out-Null
    }
}

if ($GetPercent -lt 0) {
    switch ($Profile) {
        "read_heavy" { $GetPercent = 90 }
        "balanced" { $GetPercent = 50 }
        "write_heavy" { $GetPercent = 20 }
        default { throw "unsupported profile: $Profile" }
    }
}

if ($GetPercent -lt 0 -or $GetPercent -gt 100) {
    throw "GetPercent must be between 0 and 100"
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
$SummaryPath = Join-Path $OutputDir ($Stamp + "_memcached_benchmark_client.md")
$RawLogPath = Join-Path $RawOutputDir ($Stamp + "_memcached_benchmark_client.log")
$StdoutPath = Join-Path $RawOutputDir ($Stamp + "_memcached_benchmark_client_stdout.log")
$StderrPath = Join-Path $RawOutputDir ($Stamp + "_memcached_benchmark_client_stderr.log")
$resultObject = $null

if (-not ("HzMemcachedBenchmarkClient" -as [type])) {
    Add-Type -Language CSharp -TypeDefinition @"
using System;
using System.Diagnostics;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

public sealed class HzClientBenchResult {
    public int TotalOps { get; set; }
    public int GetOps { get; set; }
    public int SetOps { get; set; }
    public double ElapsedSeconds { get; set; }
    public double OpsPerSecond { get; set; }
}

public static class HzMemcachedBenchmarkClient {
    private static string ReadGetResponse(StreamReader reader) {
        string header = reader.ReadLine();
        if (header == null) {
            throw new IOException("unexpected EOF while reading VALUE header");
        }
        if (header == "END") {
            throw new IOException("cache miss during benchmark");
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

    private static void WriteSet(StreamWriter writer, StreamReader reader, string key, string payload) {
        writer.Write("set ");
        writer.Write(key);
        writer.Write(" 0 0 ");
        writer.Write(payload.Length.ToString());
        writer.Write("\r\n");
        writer.Write(payload);
        writer.Write("\r\n");
        string line = reader.ReadLine();
        if (line != "STORED") {
            throw new IOException("unexpected set response: " + line);
        }
    }

    private static void WarmupWorker(string host, int port, int clientId, int warmupKeysPerClient, int keySpace, string payload) {
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
                for (int i = 0; i < warmupKeysPerClient; i++) {
                    int keyId = ((clientId * warmupKeysPerClient) + i) % keySpace;
                    string key = "k" + keyId.ToString();
                    WriteSet(writer, reader, key, payload);
                }
                writer.Write("quit\r\n");
                writer.Flush();
            }
        }
    }

    private static HzClientBenchResult WorkerResult(int totalOps, int getOps, int setOps) {
        return new HzClientBenchResult {
            TotalOps = totalOps,
            GetOps = getOps,
            SetOps = setOps
        };
    }

    private static HzClientBenchResult BenchmarkWorker(
        string host,
        int port,
        int clientId,
        int requestsPerClient,
        int warmupKeysPerClient,
        int keySpace,
        int getPercent,
        string payload
    ) {
        Random rng = new Random(1337 + (clientId * 97));
        int getOps = 0;
        int setOps = 0;
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
                    int keyId = rng.Next(keySpace);
                    string key = "k" + keyId.ToString();
                    if (rng.Next(100) < getPercent) {
                        writer.Write("get ");
                        writer.Write(key);
                        writer.Write("\r\n");
                        string value = ReadGetResponse(reader);
                        if (value != payload) {
                            throw new IOException("unexpected benchmark get payload");
                        }
                        getOps++;
                    } else {
                        WriteSet(writer, reader, key, payload);
                        setOps++;
                    }
                }
                writer.Write("quit\r\n");
                writer.Flush();
            }
        }
        return WorkerResult(requestsPerClient, getOps, setOps);
    }

    public static void Warmup(string host, int port, int clients, int warmupKeysPerClient, string payload) {
        int keySpace = clients * warmupKeysPerClient;
        Task[] tasks = new Task[clients];
        for (int i = 0; i < clients; i++) {
            int clientId = i;
            tasks[i] = Task.Run(() => WarmupWorker(host, port, clientId, warmupKeysPerClient, keySpace, payload));
        }
        Task.WaitAll(tasks);
    }

    public static HzClientBenchResult Run(string host, int port, int clients, int warmupKeysPerClient, int requestsPerClient, int getPercent, string payload) {
        int keySpace = clients * warmupKeysPerClient;
        Stopwatch sw = Stopwatch.StartNew();
        Task<HzClientBenchResult>[] tasks = new Task<HzClientBenchResult>[clients];
        for (int i = 0; i < clients; i++) {
            int clientId = i;
            tasks[i] = Task.Run(() => BenchmarkWorker(host, port, clientId, requestsPerClient, warmupKeysPerClient, keySpace, getPercent, payload));
        }
        Task.WaitAll(tasks);
        sw.Stop();

        int totalOps = 0;
        int getOps = 0;
        int setOps = 0;
        foreach (Task<HzClientBenchResult> task in tasks) {
            HzClientBenchResult result = task.Result;
            totalOps += result.TotalOps;
            getOps += result.GetOps;
            setOps += result.SetOps;
        }

        return new HzClientBenchResult {
            TotalOps = totalOps,
            GetOps = getOps,
            SetOps = setOps,
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

    [HzMemcachedBenchmarkClient]::Warmup($Listen, $Port, $Clients, $WarmupKeysPerClient, $Payload)
    $result = [HzMemcachedBenchmarkClient]::Run(
        $Listen,
        $Port,
        $Clients,
        $WarmupKeysPerClient,
        $RequestsPerClient,
        $GetPercent,
        $Payload
    )

    $raw.Add("exe: $ExePath")
    $raw.Add("args: " + ($serverArgs -join " "))
    $raw.Add(("profile: {0}" -f $Profile))
    $raw.Add(("clients: {0}" -f $Clients))
    $raw.Add(("warmup_keys_per_client: {0}" -f $WarmupKeysPerClient))
    $raw.Add(("requests_per_client: {0}" -f $RequestsPerClient))
    $raw.Add(("payload_size: {0}" -f $PayloadSize))
    $raw.Add(("get_percent: {0}" -f $GetPercent))
    $raw.Add(("ops_per_sec: {0:N2}" -f $result.OpsPerSecond))
    $raw.Add("")

    $summary.Add("# Windows Memcached Benchmark Client")
    $summary.Add("")
    $summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
    $summary.Add("")
    $summary.Add("References:")
    $summary.Add("- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)")
    $summary.Add("- [win/run_win_memcached_benchmark_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client.ps1)")
    $summary.Add("- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)")
    $summary.Add("")
    $summary.Add("| profile | total ops | get ops | set ops | elapsed sec | ops/sec |")
    $summary.Add("| --- | ---: | ---: | ---: | ---: | ---: |")
    $summary.Add(('| {0} | {1} | {2} | {3} | {4:N3} | {5:N2} |' -f `
        $Profile, $result.TotalOps, $result.GetOps, $result.SetOps, $result.ElapsedSeconds, $result.OpsPerSecond))
    $summary.Add("")
    $summary.Add("Config:")
    $summary.Add(('- exe: `{0}`' -f $ExePath))
    $summary.Add(('- listen: `{0}:{1}`' -f $Listen, $Port))
    $summary.Add(('- server_threads: `{0}`' -f $ServerThreads))
    $summary.Add(('- clients: `{0}`' -f $Clients))
    $summary.Add(('- warmup_keys_per_client: `{0}`' -f $WarmupKeysPerClient))
    $summary.Add(('- requests_per_client: `{0}`' -f $RequestsPerClient))
    $summary.Add(('- payload_size: `{0}`' -f $PayloadSize))
    $summary.Add(('- get_percent: `{0}`' -f $GetPercent))
    $summary.Add("")
    $summary.Add(('Raw log: `{0}`' -f $RawLogPath))

    $resultObject = [pscustomobject]@{
        Profile = $Profile
        SummaryPath = $SummaryPath
        RawLogPath = $RawLogPath
        TotalOps = $result.TotalOps
        GetOps = $result.GetOps
        SetOps = $result.SetOps
        ElapsedSeconds = $result.ElapsedSeconds
        OpsPerSecond = $result.OpsPerSecond
        Clients = $Clients
        WarmupKeysPerClient = $WarmupKeysPerClient
        RequestsPerClient = $RequestsPerClient
        PayloadSize = $PayloadSize
        GetPercent = $GetPercent
    }
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

if ($resultObject) {
    $resultObject
}
