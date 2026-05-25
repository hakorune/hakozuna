# HZ5 Linux Build Script Layout

HZ5 Linux build scripting is split so the main driver stays readable while the
large option surface remains reproducible.

## Files

```text
linux/build_linux_hz5_standalone.sh
  Main build driver.
  Owns defaults, top-level flow, compile flags, and build commands.

linux/hz5_build_usage.sh
  Long --help text only.

linux/hz5_build_profile_bases.sh
  Compound profile helper functions.
  These are reusable building blocks used by human aliases and legacy lanes.

linux/hz5_build_profile_aliases.sh
  Human-facing profile aliases.
  Add short lane names here when a profile is meant to be run by humans.

linux/hz5_build_arg_parser.sh
  Top-level argument dispatch.
  Keeps common options, help, profile aliases, and low-level dispatch glue.

linux/hz5_build_lowlevel_args.sh
  Low-level feature flag dispatcher.

linux/hz5_build_exact_args.sh
  Local2P / P25 / P43 exact-route and bridge flags.

linux/hz5_build_midpage_args.sh
  Preload, SmallFront, general presets, MidPage, and MidPage diagnostic flags.

linux/hz5_build_frontends_args.sh
  OwnerHub, MidFront, and LargeFront low-level flags.

linux/hz5_build_validate.sh
  Post-parse consistency checks.

linux/hz5_build_config_writer.sh
  Writes hz5_build_config.env for result attribution.
```

## Rule

```text
New saved or diagnostic lane:
  add a profile alias in hz5_build_profile_aliases.sh
  prefer composing existing helpers from hz5_build_profile_bases.sh

New low-level knob:
  add parser support in the matching hz5_build_*_args.sh group
  add validation in hz5_build_validate.sh if combinations can be invalid
  add build-config output in hz5_build_config_writer.sh

Do not:
  put long help text or profile alias tables back into build_linux_hz5_standalone.sh
```
