# Provider Pack Build

Provider packs are the GUI-installable bundles for allocators that are not
shipped inside the portable Bench Lab application. A pack is intentionally
one allocator and one version, not a mixed archive of unrelated DLLs.

## Build Windows packs

The repository does not commit third-party binaries. Point the builder at the
private allocator artifact tree and generate local packs:

```powershell
$env:HAKOZUNA_ALLOCATOR_BENCH_ROOT = 'Z:\TextureVoice_local\git\allocator-bench-lab'
pwsh hakozuna-bench-lab/scripts/build_provider_pack.ps1 `
  -ProviderId mimalloc `
  -SourceRoot $env:HAKOZUNA_ALLOCATOR_BENCH_ROOT
pwsh hakozuna-bench-lab/scripts/build_provider_pack.ps1 `
  -ProviderId tcmalloc `
  -SourceRoot $env:HAKOZUNA_ALLOCATOR_BENCH_ROOT
```

Generated files are written under `hakozuna-bench-lab/.artifacts/providers/`:

```text
hakozuna-mimalloc-<version>.hbl-provider.zip
hakozuna-tcmalloc-<version>.hbl-provider.zip
```

The version defaults to the build date. Use `-Version` for a reproducible
release identifier, for example `-Version 20260722-win-x64`.

## Pack contents

Each pack contains a root `provider.json`, allocator artifacts under
`artifacts/`, a `LICENSES/` notice, and `SHA256SUMS`. The mimalloc pack also
contains `minject.exe`, the redirect DLL, and a package-local launcher because
that Windows lane needs injection. The tcmalloc pack contains the Bench Lab
adapter DLL and its runtime DLL when present.

The manifest environment values use package-relative paths. A future runner
resolves them against the installed provider directory; absolute paths from a
maintainer's machine are never written into the pack.

## Install and verify

1. Open the Bench Lab `Providers` tab.
2. Choose the generated `.hbl-provider.zip`.
3. Confirm that the provider appears under its own `id/version` directory.
4. Use the provider only with an isolated child-process runner.

For an offline preflight before importing a pack:

```powershell
pwsh hakozuna-bench-lab/scripts/verify_provider_pack.ps1 `
  hakozuna-bench-lab/.artifacts/providers/hakozuna-mimalloc-20260722-win-x64.hbl-provider.zip
```

The installer validates platform, architecture, safe paths, declared artifact
hashes, and side-by-side version identity. It rejects overwrite and never
loads provider DLLs into the GUI process.

Maintainers can exercise the same installer code used by the GUI without
opening a file picker. Run one command per pack:

```powershell
pwsh hakozuna-bench-lab/scripts/install_provider_pack_smoke.ps1 `
  hakozuna-bench-lab/.artifacts/providers/hakozuna-mimalloc-20260722-win-x64.hbl-provider.zip
pwsh hakozuna-bench-lab/scripts/install_provider_pack_smoke.ps1 `
  hakozuna-bench-lab/.artifacts/providers/hakozuna-tcmalloc-20260722-win-x64.hbl-provider.zip
```

`PACK_NOTICES.txt` deliberately marks locally generated packs as evaluation
artifacts. Before public redistribution, replace it with the applicable
upstream license files and review redistribution terms for every binary.

## Scope

These packs prepare the installation and identity boundary. They do not yet
silently rewrite the Rust manifest runner or claim that every provider has the
same workload adapter. The raw CLI runner remains authoritative until a
provider-aware runner resolves an installed manifest's environment profile.
