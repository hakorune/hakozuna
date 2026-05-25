#!/usr/bin/env bash
# Sourced by hz5_build_arg_parser.sh.
# Dispatches low-level Linux HZ5 feature flag parsing to focused groups.

source "${ROOT_DIR}/linux/hz5_build_exact_args.sh"
source "${ROOT_DIR}/linux/hz5_build_midpage_args.sh"
source "${ROOT_DIR}/linux/hz5_build_frontends_args.sh"

hz5_try_parse_lowlevel_build_arg() {
  if hz5_try_parse_exact_build_arg "$@"; then
    return 0
  fi
  if hz5_try_parse_midpage_build_arg "$@"; then
    return 0
  fi
  if hz5_try_parse_frontend_build_arg "$@"; then
    return 0
  fi
  return 1
}
