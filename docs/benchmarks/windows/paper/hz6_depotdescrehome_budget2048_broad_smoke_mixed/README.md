# HZ6 DepotDescriptorRehomeBudget2048 Broad Mixed Smoke

Generated during the 2026-06-06 broad-lane check for:

```text
speed +
ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-
depotownerdirect-depotdescrehome-budget2048-desc16k-front4k-
thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-
storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-
source64-route16k-run4096
```

Result:

```text
mixed_ws balanced:
  PASS, run-1 around 65.0M ops/s, peak around 112000 KB,
  alloc_fail=0, route_invalid=0, route_miss=0, route_register_fail=0.

mixed_ws wide_ws:
  NO-GO. Direct run exits with access violation:
    code=0xc0000005
    info0=1
    info1=000001D0AE140020

Interpretation:
  DepotDescriptorRehomeBudget2048-L1 stays a Larson bounded
  source-depot candidate-control. Do not promote it as a broad mixed_ws
  or default lane without a separate wide_ws safety fix.
```

