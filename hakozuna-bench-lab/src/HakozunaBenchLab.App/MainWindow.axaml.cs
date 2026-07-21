using Avalonia.Controls;
using Avalonia.Platform.Storage;
using HakozunaBenchLab.Agent;
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

    private static string GetPlatform() => OperatingSystem.IsMacOS() ? "macos" : "windows";

    private static string GetArchitecture() => RuntimeInformation.ProcessArchitecture switch
    {
        Architecture.X64 => "x64",
        Architecture.Arm64 => "arm64",
        Architecture.X86 => "x86",
        Architecture.Arm => "arm",
        var architecture => architecture.ToString().ToLowerInvariant()
    };
}
