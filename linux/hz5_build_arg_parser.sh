#!/usr/bin/env bash
# Sourced by build_linux_hz5_standalone.sh after defaults and profile helper functions are defined.
# Keep option parsing here so the build driver stays readable.

source "${ROOT_DIR}/linux/hz5_build_profile_aliases.sh"
source "${ROOT_DIR}/linux/hz5_build_lowlevel_args.sh"

hz5_parse_build_args() {
  while [[ $# -gt 0 ]]; do
    if hz5_try_apply_profile_alias "$1"; then
      shift
      continue
    fi

    case "$1" in
      --arch)
        require_value "$@"
        ARCH="$2"
        shift 2
        ;;
      --out-dir)
        require_value "$@"
        OUT_DIR="$2"
        shift 2
        ;;
      --help|-h)
        usage
        exit 0
        ;;
      *)
        if hz5_try_parse_lowlevel_build_arg "$@"; then
          shift "$HZ5_PARSE_SHIFT"
          continue
        fi
        echo "unknown option: $1" >&2
        usage >&2
        exit 1
        ;;
    esac
  done
}
