using Avalonia.Controls;
using Avalonia.Platform.Storage;
using HakozunaBenchLab.Agent;
using HakozunaBenchLab.Core.Results;
using HakozunaBenchLab.Core.Providers;
using System.Runtime.InteropServices;

namespace HakozunaBenchLab.App;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        ProviderRootText.Text = $"Provider storage: {ProviderPaths.GetProviderRoot()}";
    }

    private async void ImportProviderClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        var files = await StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions
        {
            Title = "Choose a Hakozuna Bench Lab provider pack",
            AllowMultiple = false,
            FileTypeFilter =
            [
                new FilePickerFileType("Hakozuna provider pack")
                {
                    Patterns = ["*.hbl-provider.zip", "*.zip"]
                }
            ]
        });
        if (files.Count == 0) return;

        var packagePath = files[0].TryGetLocalPath();
        if (string.IsNullOrWhiteSpace(packagePath))
        {
            ProviderStatusText.Text = "This file is not available as a local provider pack.";
            return;
        }

        ProviderStatusText.Text = "Verifying provider manifest and artifact hashes...";
        try
        {
            var installed = await new ProviderPackageInstaller().InstallAsync(
                packagePath,
                ProviderPaths.GetProviderRoot(),
                new ProviderInstallOptions(GetPlatform(), GetArchitecture()));
            ProviderStatusText.Text =
                $"Installed {installed.Manifest.Label} {installed.Manifest.Version}. Restart is not required.";
        }
        catch (Exception exception) when (exception is IOException or InvalidDataException or UnauthorizedAccessException)
        {
            ProviderStatusText.Text = $"Provider was not installed: {exception.Message}";
        }
    }

    private async void RunBenchmarkClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        var allocatorId = (AllocatorComboBox.SelectedItem as ComboBoxItem)?.Content?.ToString();
        var workloadId = (WorkloadComboBox.SelectedItem as ComboBoxItem)?.Content?.ToString();
        if (string.IsNullOrWhiteSpace(allocatorId) || string.IsNullOrWhiteSpace(workloadId))
        {
            RunStatusText.Text = "Select an allocator and workload first.";
            return;
        }

        RunButton.IsEnabled = false;
        RunStatusText.Text = "Running isolated Preview benchmark...";
        OpsValueText.Text = "...";
        PeakRssValueText.Text = "...";
        PostRssValueText.Text = "...";
        try
        {
            // This connected slice is Preview-only; Verified requires package hashes.
            var plan = RepositoryBenchCatalog.CreatePreviewPlan(allocatorId, workloadId);
            var result = await new BenchmarkAgent().RunAsync(plan,
                new AgentOptions(TimeSpan.FromSeconds(30)));
            var measurement = BenchmarkOutputParser.ParseSingle(result);
            OpsValueText.Text = FormatOps(measurement.OpsPerSecond);
            PeakRssValueText.Text = FormatBytes(measurement.PeakWorkingSetBytes);
            PostRssValueText.Text = "runner does not emit it";
            RunStatusText.Text = $"Preview complete: {allocatorId} / {workloadId}";
        }
        catch (Exception exception) when (exception is InvalidOperationException or
            NotSupportedException or FileNotFoundException or InvalidDataException or IOException)
        {
            OpsValueText.Text = "--";
            PeakRssValueText.Text = "--";
            PostRssValueText.Text = "--";
            RunStatusText.Text = exception.Message;
        }
        finally
        {
            RunButton.IsEnabled = true;
        }
    }

    private async void RunComparisonClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
    {
        var suite = (CompareSuiteComboBox.SelectedItem as ComboBoxItem)?.Content?.ToString();
        var profile = (CompareProfileComboBox.SelectedItem as ComboBoxItem)?.Content?.ToString();
        if (string.IsNullOrWhiteSpace(suite) || string.IsNullOrWhiteSpace(profile))
        {
            CompareStatusText.Text = "Select a suite and profile first.";
            return;
        }

        RunComparisonButton.IsEnabled = false;
        CompareStatusText.Text = $"Running {suite} comparison...";
        CompareRecommendedText.Text = "Recommended: ...";
        CompareSummaryText.Text = "Waiting for isolated allocator workers...";
        try
        {
            var request = RepositoryComparisonCatalog.CreateRequest(suite, profile);
            var scorecard = await new ComparisonAgent().RunAsync(
                request, new ComparisonAgentOptions(TimeSpan.FromMinutes(5)));
            ShowComparisonScorecard(scorecard);
        }
        catch (Exception exception) when (exception is InvalidOperationException or
            NotSupportedException or FileNotFoundException or InvalidDataException or
            IOException or TimeoutException)
        {
            CompareRecommendedText.Text = "Recommended: --";
            CompareSummaryText.Text = exception.Message;
            CompareStatusText.Text = "Comparison failed";
        }
        finally
        {
            RunComparisonButton.IsEnabled = true;
        }
    }

    private void ShowComparisonScorecard(ComparisonScorecard scorecard)
    {
        CompareStatusText.Text = $"Complete: {scorecard.Profile} / {string.Join(", ", scorecard.SuiteNames)}";
        CompareRecommendedText.Text = $"Recommended: {scorecard.RecommendedAllocator ?? "n/a"}";
        CompareSummaryText.Text = string.Join(Environment.NewLine,
            scorecard.AllocatorSummaries.Select(summary =>
                $"{summary.Allocator,-22} speed {summary.AverageSpeedScore,6:F1}  memory {FormatScore(summary.AverageMemoryScore),6}  score {FormatScore(summary.AverageProfileScore),6}"));
    }

    private static string FormatScore(double? score) => score is double value ? $"{value:F1}" : "n/a";

    private static string GetPlatform() => OperatingSystem.IsMacOS() ? "macos" : "windows";

    private static string GetArchitecture() => RuntimeInformation.ProcessArchitecture switch
    {
        Architecture.X64 => "x64",
        Architecture.Arm64 => "arm64",
        Architecture.X86 => "x86",
        Architecture.Arm => "arm",
        var architecture => architecture.ToString().ToLowerInvariant()
    };

    private static string FormatOps(double opsPerSecond) => opsPerSecond >= 1_000_000
        ? $"{opsPerSecond / 1_000_000d:F2}M"
        : $"{opsPerSecond:N0}";

    private static string FormatBytes(long bytes) => bytes <= 0
        ? "not sampled"
        : $"{bytes / 1024d / 1024d:F2} MiB";
}
