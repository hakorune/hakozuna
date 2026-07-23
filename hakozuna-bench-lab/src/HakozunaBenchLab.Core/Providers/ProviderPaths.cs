namespace HakozunaBenchLab.Core.Providers;

public static class ProviderPaths
{
    public static string GetApplicationDataRoot()
    {
        if (OperatingSystem.IsMacOS())
        {
            return Path.Combine(
                System.Environment.GetFolderPath(System.Environment.SpecialFolder.UserProfile),
                "Library", "Application Support", "HakozunaBenchLab");
        }

        return Path.Combine(
            System.Environment.GetFolderPath(System.Environment.SpecialFolder.LocalApplicationData),
            "HakozunaBenchLab");
    }

    public static string GetProviderRoot() => Path.Combine(GetApplicationDataRoot(), "providers");
}
