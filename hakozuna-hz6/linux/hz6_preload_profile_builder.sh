#!/usr/bin/env bash

# Shared helpers for thin HZ6 preload profile builders.  Keep profile scripts
# focused on their macro deltas so selected flag drift stays centralized.

HZ6_PROFILE_HELPER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${ROOT_DIR:-$(cd "${HZ6_PROFILE_HELPER_DIR}/../.." && pwd)}"
HZ6_DIR="${HZ6_DIR:-${ROOT_DIR}/hakozuna-hz6}"

source "${HZ6_DIR}/linux/hz6_preload_flags.sh"

hz6_preload_profile_selected_cflags() {
  local out_name="$1"
  local enable_midpage_boundary="${2:-1}"
  hz6_preload_effective_selected_cflags "$out_name" "$enable_midpage_boundary"
}

hz6_preload_profile_owner_inbox_external_cflags() {
  local out_name="$1"
  local enable_midpage_boundary="${2:-1}"
  hz6_preload_effective_owner_inbox_external_cflags \
    "$out_name" "$enable_midpage_boundary"
}

hz6_preload_profile_owner_inbox_direct_reuse_cflags() {
  local out_name="$1"
  local enable_midpage_boundary="${2:-1}"
  hz6_preload_effective_owner_inbox_external_cflags \
    "$out_name" "$enable_midpage_boundary"
  hz6_preload_replace_define "$out_name" HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 1
  hz6_preload_replace_define "$out_name" HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1 1
  hz6_preload_replace_define "$out_name" HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1 0
}

hz6_preload_profile_owner_inbox_toy2_split_cflags() {
  local out_name="$1"
  local enable_midpage_boundary="${2:-1}"
  hz6_preload_effective_owner_inbox_external_cflags \
    "$out_name" "$enable_midpage_boundary"
  hz6_preload_replace_define \
    "$out_name" HZ6_REMOTE_PENDING_TOY_CLASS2_FRONT_MAINTENANCE_GATE_L1 1
  hz6_preload_replace_define \
    "$out_name" HZ6_REMOTE_PENDING_SOURCE_GATE_MAINTENANCE_L1 1
}

hz6_preload_profile_owner_inbox_off_cflags() {
  local out_name="$1"
  local enable_midpage_boundary="${2:-1}"
  hz6_preload_effective_owner_inbox_off_cflags \
    "$out_name" "$enable_midpage_boundary"
}

hz6_preload_profile_high_remote_transfer_presence_cflags() {
  local out_name="$1"
  local enable_midpage_boundary="${2:-1}"
  hz6_preload_effective_owner_inbox_off_cflags \
    "$out_name" "$enable_midpage_boundary"
  hz6_preload_replace_define "$out_name" HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1 1
  hz6_preload_replace_define "$out_name" \
    HZ6_TRANSFER_CLASS_PRESENCE_MIN_TOTAL '((size_t)192)'
}

hz6_preload_profile_build() {
  local -n flags_ref="$1"
  local default_out_dir="$2"

  OUT_DIR="${OUT_DIR:-${default_out_dir}}" \
  HZ6_PRELOAD_DEFAULT_CFLAGS="$(hz6_preload_join_flags "${flags_ref[@]}")" \
    "${HZ6_DIR}/linux/build_hz6_preload.sh"
}
