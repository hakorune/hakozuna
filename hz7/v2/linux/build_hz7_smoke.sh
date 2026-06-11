#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
hz7_root="$(cd "${script_dir}/.." && pwd)"
out_dir="${hz7_root}/out/linux/smokes"

mkdir -p "${out_dir}"

cc="${CC:-cc}"
cxx="${CXX:-c++}"
out="${out_dir}/hz7_smoke"
remote_out="${out_dir}/hz7_remote_smoke"
remote_natural_out="${out_dir}/hz7_remote_natural_smoke"
mt_out="${out_dir}/hz7_mt_smoke"
stats_out="${out_dir}/hz7_stats_smoke"
cpp_obj="${out_dir}/hz7_cpp_hz7.o"
cpp_out="${out_dir}/hz7_cpp_smoke"

build_c_smoke() {
  local name="$1"
  local source="$2"
  local output="$3"
  shift 3

  echo "[hz7-linux] building ${name}"
  "${cc}" -std=c11 -O2 -Wall -Wextra -Werror "$@" \
    "${hz7_root}/hz7.c" \
    "${source}" \
    -o "${output}"
}

run_smoke() {
  local name="$1"
  local output="$2"

  echo "[hz7-linux] running ${name}"
  "${output}"
}

build_cpp_smoke() {
  echo "[hz7-linux] building hz7_cpp_smoke"
  "${cc}" -std=c11 -O2 -Wall -Wextra -Werror \
    -c "${hz7_root}/hz7.c" \
    -o "${cpp_obj}"

  "${cxx}" -std=c++11 -O2 -Wall -Wextra -Werror \
    "${cpp_obj}" \
    "${hz7_root}/tests/hz7_cpp_smoke.cpp" \
    -o "${cpp_out}"
}

build_c_smoke "hz7_smoke" "${hz7_root}/tests/hz7_smoke.c" "${out}"
build_c_smoke "hz7_remote_smoke" "${hz7_root}/tests/hz7_remote_smoke.c" "${remote_out}" -pthread
build_c_smoke "hz7_remote_natural_smoke" "${hz7_root}/tests/hz7_remote_natural_smoke.c" "${remote_natural_out}" -pthread -DH7_REMOTE_NATURAL_PRESET=1
build_c_smoke "hz7_mt_smoke" "${hz7_root}/tests/hz7_mt_smoke.c" "${mt_out}" -pthread
build_c_smoke "hz7_stats_smoke" "${hz7_root}/tests/hz7_stats_smoke.c" "${stats_out}"
build_cpp_smoke

run_smoke "hz7_smoke" "${out}"
run_smoke "hz7_remote_smoke" "${remote_out}"
run_smoke "hz7_remote_natural_smoke" "${remote_natural_out}"
run_smoke "hz7_mt_smoke" "${mt_out}"
run_smoke "hz7_stats_smoke" "${stats_out}"
run_smoke "hz7_cpp_smoke" "${cpp_out}"
