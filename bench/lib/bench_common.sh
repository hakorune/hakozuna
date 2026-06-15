#!/usr/bin/env bash
set -euo pipefail

bench_os_name() {
  uname -s
}

bench_is_macos() {
  [[ "$(bench_os_name)" == "Darwin" ]]
}

bench_is_linux() {
  [[ "$(bench_os_name)" == "Linux" ]]
}

bench_preload_var() {
  if bench_is_macos; then
    printf '%s\n' "DYLD_INSERT_LIBRARIES"
  else
    printf '%s\n' "LD_PRELOAD"
  fi
}

bench_find_first_existing() {
  local candidate
  for candidate in "$@"; do
    [[ -n "$candidate" && -f "$candidate" ]] && {
      printf '%s\n' "$candidate"
      return 0
    }
  done
  return 1
}

bench_find_from_ldconfig() {
  local pattern="$1"
  if command -v ldconfig >/dev/null 2>&1; then
    ldconfig -p 2>/dev/null | rg "${pattern}" | head -n 1 | awk '{print $NF}' || true
  fi
}

bench_find_from_brew_prefix() {
  local formula="$1"
  shift
  local prefix=""
  if command -v brew >/dev/null 2>&1; then
    prefix="$(brew --prefix "${formula}" 2>/dev/null || true)"
  fi
  if [[ -n "${prefix}" ]]; then
    bench_find_first_existing "${prefix}/lib/$1" "${prefix}/lib/$2" "${prefix}/lib/$3" "${prefix}/lib/$4"
  fi
}

bench_find_private_linux_library() {
  local allocator="$1"
  local pattern="$2"
  local cache_root="${ROOT_DIR}/private/bench-assets/linux/allocators"

  if [[ -d "${cache_root}" ]]; then
    find "${cache_root}" -type f -path "*/${allocator}/*" -name "${pattern}" 2>/dev/null | sort | head -n 1
  fi
}

bench_find_mimalloc_library() {
  if bench_is_macos; then
    bench_find_first_existing \
      "${MIMALLOC_SO:-}" \
      "$(bench_find_from_brew_prefix mimalloc libmimalloc.dylib libmimalloc.so libmimalloc_minimal.dylib libmimalloc_minimal.so || true)" \
      "/opt/homebrew/opt/mimalloc/lib/libmimalloc.dylib" \
      "/usr/local/opt/mimalloc/lib/libmimalloc.dylib"
    return $?
  fi

  bench_find_first_existing \
    "${MIMALLOC_SO:-}" \
    "$(bench_find_from_ldconfig 'libmimalloc\\.so' || true)" \
    "$(bench_find_private_linux_library libmimalloc2.0 'libmimalloc.so*' || true)"
}

bench_find_tcmalloc_library() {
  if bench_is_macos; then
    bench_find_first_existing \
      "${TCMALLOC_SO:-}" \
      "$(bench_find_from_brew_prefix gperftools libtcmalloc.dylib libtcmalloc_minimal.dylib libtcmalloc.so libtcmalloc_minimal.so || true)" \
      "/opt/homebrew/opt/gperftools/lib/libtcmalloc.dylib" \
      "/opt/homebrew/opt/gperftools/lib/libtcmalloc_minimal.dylib" \
      "/usr/local/opt/gperftools/lib/libtcmalloc.dylib" \
      "/usr/local/opt/gperftools/lib/libtcmalloc_minimal.dylib"
    return $?
  fi

  bench_find_first_existing \
    "${TCMALLOC_SO:-}" \
    "$(bench_find_from_ldconfig 'libtcmalloc_minimal\\.so' || true)" \
    "$(bench_find_from_ldconfig 'libtcmalloc\\.so' || true)" \
    "$(bench_find_private_linux_library libtcmalloc-minimal4 'libtcmalloc_minimal.so*' || true)" \
    "$(bench_find_private_linux_library libtcmalloc-minimal4t64 'libtcmalloc_minimal.so*' || true)" \
    "$(bench_find_private_linux_library libgoogle-perftools4 'libtcmalloc.so*' || true)" \
    "$(bench_find_private_linux_library libgoogle-perftools4t64 'libtcmalloc.so*' || true)"
}

bench_find_hakorune_mimalloc_library() {
  local provider_dir="${HAKORUNE_MIMALLOC_PROVIDER_DIR:-${ROOT_DIR}/private/bench-assets/linux/allocators/hakorune-mimalloc-provider/hakorune-mimalloc-provider}"

  bench_find_first_existing \
    "${HAKORUNE_MIMALLOC_LDPRELOAD_SO:-}" \
    "${provider_dir}/libhakorune_provider_ldpreload.so" \
    "${ROOT_DIR}/hakorune-mimalloc-provider/libhakorune_provider_ldpreload.so"
}

bench_find_hz5_library() {
  local arch
  arch="$(uname -m)"
  case "${arch}" in
    amd64) arch="x86_64" ;;
    arm64) arch="arm64" ;;
  esac

  bench_find_first_existing \
    "${HZ5_PRELOAD_FULL_SO:-}" \
    "${HZ5_SO:-}" \
    "${ROOT_DIR}/hakozuna-hz5/out/linux/${arch}-hz5-preload-full/libhakozuna_hz5_preload_full.so" \
    "${ROOT_DIR}/hakozuna-hz5/out/linux/${arch}-hz5-preload-full/libhakozuna_hz5_preload_hybrid.so" \
    "${ROOT_DIR}/hakozuna-hz5/out/linux/hz5-preload-full/libhakozuna_hz5_preload_full.so" \
    "${ROOT_DIR}/hakozuna-hz5/out/linux/hz5-preload-hybrid-smoke/libhakozuna_hz5_preload_hybrid.so"
}

bench_find_hz6_library() {
  local arch
  arch="$(uname -m)"
  case "${arch}" in
    amd64) arch="x86_64" ;;
    arm64) arch="arm64" ;;
  esac

  bench_find_first_existing \
    "${HZ6_PRELOAD_SO:-}" \
    "${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so" \
    "${ROOT_DIR}/hakozuna-hz6/out/linux/${arch}-hz6-preload/libhakozuna_hz6_preload.so"
}

bench_find_hz6_toy_target_library() {
  local arch
  arch="$(uname -m)"
  case "${arch}" in
    amd64) arch="x86_64" ;;
    arm64) arch="arm64" ;;
  esac

  bench_find_first_existing \
    "${HZ6_TOY_TARGET_PRELOAD_SO:-}" \
    "${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_toy_target/libhakozuna_hz6_preload.so" \
    "${ROOT_DIR}/hakozuna-hz6/out/linux/${arch}-hz6-preload-toy-target/libhakozuna_hz6_preload.so"
}

bench_find_hz6_aligned_target_library() {
  local arch
  arch="$(uname -m)"
  case "${arch}" in
    amd64) arch="x86_64" ;;
    arm64) arch="arm64" ;;
  esac

  bench_find_first_existing \
    "${HZ6_ALIGNED_TARGET_PRELOAD_SO:-}" \
    "${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_aligned_target/libhakozuna_hz6_preload.so" \
    "${ROOT_DIR}/hakozuna-hz6/out/linux/${arch}-hz6-preload-aligned-target/libhakozuna_hz6_preload.so"
}

bench_find_allocator_library() {
  local allocator="$1"

  case "${allocator}" in
    system)
      printf '%s\n' ""
      ;;
    hz3)
      bench_find_first_existing \
        "${HZ3_SO:-${ROOT_DIR}/libhakozuna_hz3_scale.so}" \
        "${ROOT_DIR}/libhakozuna_hz3_scale.dylib"
      ;;
    hz4)
      bench_find_first_existing \
        "${HZ4_SO:-${ROOT_DIR}/hakozuna-mt/libhakozuna_hz4.so}" \
        "${ROOT_DIR}/hakozuna-mt/libhakozuna_hz4.dylib"
      ;;
    mimalloc)
      bench_find_mimalloc_library
      ;;
    tcmalloc)
      bench_find_tcmalloc_library
      ;;
    hz5)
      bench_find_hz5_library
      ;;
    hz6)
      bench_find_hz6_library
      ;;
    hz6-toy-target|hz6_toy_target)
      bench_find_hz6_toy_target_library
      ;;
    hz6-aligned-target|hz6_aligned_target)
      bench_find_hz6_aligned_target_library
      ;;
    hakorune-mimalloc|hakorune_mimalloc)
      bench_find_hakorune_mimalloc_library
      ;;
    /*)
      bench_find_first_existing "${allocator}"
      ;;
    *)
      echo "unknown allocator: ${allocator}" >&2
      return 1
      ;;
  esac
}

bench_print_allocator_hints() {
  local allocator="$1"
  case "${allocator}" in
    mimalloc)
      if bench_is_macos; then
        echo "hint: install with 'brew install mimalloc' or set MIMALLOC_SO" >&2
      else
        echo "hint: install the distro mimalloc package, run './linux/prepare_linux_bench_allocators.sh', or set MIMALLOC_SO" >&2
      fi
      ;;
    tcmalloc)
      if bench_is_macos; then
        echo "hint: install with 'brew install gperftools' or set TCMALLOC_SO" >&2
      else
        echo "hint: install the distro gperftools / tcmalloc package, run './linux/prepare_linux_bench_allocators.sh', or set TCMALLOC_SO" >&2
      fi
      ;;
    hakorune-mimalloc|hakorune_mimalloc)
      echo "hint: extract hakorune-mimalloc-provider.zip under private/bench-assets/linux/allocators or set HAKORUNE_MIMALLOC_PROVIDER_DIR" >&2
      ;;
    hz5)
      echo "hint: build the HZ5 preload full lane with './linux/build_linux_hz5_preload_full.sh' or set HZ5_PRELOAD_FULL_SO" >&2
      ;;
    hz6)
      echo "hint: build the HZ6 preload lane with './hakozuna-hz6/linux/build_hz6_preload.sh' or set HZ6_PRELOAD_SO" >&2
      ;;
    hz6-aligned-target|hz6_aligned_target)
      echo "hint: build the HZ6 aligned target lane with './hakozuna-hz6/linux/build_hz6_preload_aligned_target.sh' or set HZ6_ALIGNED_TARGET_PRELOAD_SO" >&2
      ;;
    hz3)
      echo "hint: build hz3 first, for example './linux/build_linux_release_lane.sh' or './mac/build_mac_release_lane.sh'" >&2
      ;;
    hz4)
      echo "hint: build hz4 first, for example './linux/build_linux_release_lane.sh' or './mac/build_mac_release_lane.sh'" >&2
      ;;
  esac
}

bench_run_with_allocator() {
  local allocator="$1"
  local lib_path="$2"
  shift 2

  if [[ "${allocator}" == "system" ]]; then
    "$@"
    return $?
  fi

  if bench_is_macos; then
    DYLD_INSERT_LIBRARIES="${lib_path}" "$@"
  else
    case "${allocator}" in
      hakorune-mimalloc|hakorune_mimalloc)
        local provider_dir
        provider_dir="$(cd "$(dirname "${lib_path}")" && pwd)"
        env \
          HAKORUNE_PROVIDER_LIBRARY="${HAKORUNE_PROVIDER_LIBRARY:-${provider_dir}/libhakorune_provider.so}" \
          HAKORUNE_PROVIDER_LDPRELOAD_REPORT="${HAKORUNE_PROVIDER_LDPRELOAD_REPORT:-${provider_dir}/ldpreload_counts.out}" \
          LD_PRELOAD="${lib_path}" \
          "$@"
        ;;
      *)
        env LD_PRELOAD="${lib_path}" "$@"
        ;;
    esac
  fi
}
