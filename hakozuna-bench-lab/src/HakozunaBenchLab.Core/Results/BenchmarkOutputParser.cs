using System.Text.RegularExpressions;
using HakozunaBenchLab.Core.Models;

namespace HakozunaBenchLab.Core.Results;

public static partial class BenchmarkOutputParser
{
    public static BenchmarkMeasurement ParseSingle(RunResult result)
    {
        ArgumentNullException.ThrowIfNull(result);
        var sample = result.Samples.SingleOrDefault()
            ?? throw new InvalidDataException("A preview result must contain one sample.");
        if (sample.TimedOut || sample.ExitCode != 0)
            throw new InvalidDataException("The benchmark process did not complete successfully.");

        var match = OpsPerSecondRegex().Match(sample.StandardOutput);
        if (!match.Success)
            throw new InvalidDataException("Benchmark output did not contain an ops/s value.");
        if (!double.TryParse(match.Groups["value"].Value,
                System.Globalization.NumberStyles.Float,
                System.Globalization.CultureInfo.InvariantCulture,
                out var opsPerSecond))
            throw new InvalidDataException("Benchmark ops/s value was invalid.");

        var peakWorkingSetBytes = sample.PeakWorkingSetBytes;
        var peakMatch = PeakRssKilobytesRegex().Match(sample.StandardOutput);
        if (peakMatch.Success && long.TryParse(peakMatch.Groups["value"].Value,
                System.Globalization.NumberStyles.Integer,
                System.Globalization.CultureInfo.InvariantCulture,
                out var peakKilobytes))
            peakWorkingSetBytes = checked(peakKilobytes * 1024);

        return new BenchmarkMeasurement(opsPerSecond, peakWorkingSetBytes,
            sample.StandardOutput);
    }

    [GeneratedRegex(@"\bops/s=(?<value>[0-9]+(?:\.[0-9]+)?)", RegexOptions.CultureInvariant)]
    private static partial Regex OpsPerSecondRegex();

    [GeneratedRegex(@"\bpeak_rss_kb=(?<value>[0-9]+)", RegexOptions.CultureInvariant)]
    private static partial Regex PeakRssKilobytesRegex();
}
