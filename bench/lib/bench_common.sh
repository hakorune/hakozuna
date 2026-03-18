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
    "$(bench_find_from_ldconfig 'libmimalloc\\.so' || true)"
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
    "$(bench_find_from_ldconfig 'libtcmalloc\\.so' || true)"
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
        echo "hint: install the distro mimalloc package or set MIMALLOC_SO" >&2
      fi
      ;;
    tcmalloc)
      if bench_is_macos; then
        echo "hint: install with 'brew install gperftools' or set TCMALLOC_SO" >&2
      else
        echo "hint: install the distro gperftools / tcmalloc package or set TCMALLOC_SO" >&2
      fi
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

  local preload_var
  preload_var="$(bench_preload_var)"
  env "${preload_var}=${lib_path}" "$@"
}
