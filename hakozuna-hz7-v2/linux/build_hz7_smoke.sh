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
mt_out="${out_dir}/hz7_mt_smoke"
stats_out="${out_dir}/hz7_stats_smoke"
cpp_obj="${out_dir}/hz7_cpp_hz7.o"
cpp_out="${out_dir}/hz7_cpp_smoke"

"${cc}" -std=c11 -O2 -Wall -Wextra -Werror \
  "${hz7_root}/hz7.c" \
  "${hz7_root}/tests/hz7_smoke.c" \
  -o "${out}"

"${out}"

"${cc}" -std=c11 -O2 -Wall -Wextra -Werror -pthread \
  "${hz7_root}/hz7.c" \
  "${hz7_root}/tests/hz7_remote_smoke.c" \
  -o "${remote_out}"

"${remote_out}"

"${cc}" -std=c11 -O2 -Wall -Wextra -Werror -pthread \
  "${hz7_root}/hz7.c" \
  "${hz7_root}/tests/hz7_mt_smoke.c" \
  -o "${mt_out}"

"${mt_out}"

"${cc}" -std=c11 -O2 -Wall -Wextra -Werror \
  "${hz7_root}/hz7.c" \
  "${hz7_root}/tests/hz7_stats_smoke.c" \
  -o "${stats_out}"

"${stats_out}"

"${cc}" -std=c11 -O2 -Wall -Wextra -Werror \
  -c "${hz7_root}/hz7.c" \
  -o "${cpp_obj}"

"${cxx}" -std=c++11 -O2 -Wall -Wextra -Werror \
  "${cpp_obj}" \
  "${hz7_root}/tests/hz7_cpp_smoke.cpp" \
  -o "${cpp_out}"

"${cpp_out}"
