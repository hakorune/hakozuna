# Distribution and Provider Packs

## Application Distribution

The first Windows release is a self-contained portable archive:

```text
HakozunaBenchLab-win-x64.zip
  HakozunaBenchLab.exe
  HakozunaBenchLab.Agent.dll
  HakozunaBenchLab.Core.dll
  providers/
    system/
    hz8/
```

Users extract the archive and run the application without administrator
rights or a separate .NET installation. A signed installer and updater may be
added after the provider and result schemas are stable.

Maintainers build the portable archive with:

```powershell
pwsh hakozuna-bench-lab/scripts/build_windows_portable.ps1
```

The output is written under `hakozuna-bench-lab/.artifacts/`. Provider
versions are installed into the per-user application-data directory rather
than beside the executable, so upgrading the portable application does not
overwrite them.

The macOS follow-up uses the same Avalonia application and schemas in a
self-contained, signed, and notarized `.dmg`.

## User Flow

```text
1. Download Hakozuna Bench Lab.
2. Start the application.
3. System and HZ8 appear as installed providers.
4. Open Providers to install mimalloc or tcmalloc packs.
5. Select allocators and a workload in Run Setup.
6. Run Preview or Verified measurements.
```

The application never asks users to copy allocator DLLs into the GUI folder.

Maintainers can build local Windows packs from prepared private artifacts with
`scripts/build_provider_pack.ps1`. The reproducible procedure and the current
mimalloc/tcmalloc artifact mapping are documented in
`docs/PROVIDER_PACK_BUILD.md`. Generated ZIPs stay under `.artifacts/` and are
not committed to the repository.

## Provider Pack

A provider pack is a platform-specific ZIP with the extension
`.hbl-provider.zip`:

```text
provider.json
artifacts/
  bench-local.exe
  bench-remote.exe
  allocator.dll
LICENSES/
SHA256SUMS
```

`provider.json` identifies the allocator version, build, platform,
architecture, lane kind, environment profiles, and every benchmark artifact.
All artifact paths are relative to the pack root and carry SHA-256 values.

## Installation Root

Windows:

```text
%LOCALAPPDATA%/HakozunaBenchLab/
  providers/<provider-id>/<version>/
  results/
  environment-profiles/
```

macOS:

```text
~/Library/Application Support/HakozunaBenchLab/
  providers/<provider-id>/<version>/
  results/
  environment-profiles/
```

Versions install side by side. Import refuses to overwrite an existing
`provider-id/version` directory.

## Built-in and Downloadable Providers

| Provider | Initial delivery |
|---|---|
| System allocator | bundled |
| HZ8 selected default | bundled first-party pack |
| mimalloc | explicit downloadable official pack |
| tcmalloc | explicit downloadable official pack |
| Custom allocator | user-imported provider pack |

No third-party pack is downloaded silently. A future catalog may expose
available versions, but installation always requires user action.

Provider selection means selecting one allocator pack for a comparison lane;
it does not mean that the application ships an unreviewed bundle of every
allocator. Each provider has its own manifest, version, environment profile,
license notice, and artifact hashes.

## Security Contract

1. The GUI never loads provider DLLs into its own process.
2. ZIP paths must remain inside a temporary staging directory.
3. Symbolic links and absolute paths are rejected.
4. Provider IDs and versions must be safe path segments.
5. Every declared artifact must exist and match its SHA-256.
6. Installation is atomic and refuses replacement of an existing version.
7. License files remain inside the installed provider directory.
8. Verified runs record provider, runner, environment, and hashes.

Cryptographic release signatures are a follow-up layer. SHA-256 protects
identity and accidental corruption; it is not by itself publisher
authentication.
