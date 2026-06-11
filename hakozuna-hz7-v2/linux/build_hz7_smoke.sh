#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
hz7_root="$(cd "${script_dir}/.." && pwd)"
out_dir="${hz7_root}/out/linux/smokes"

mkdir -p "${out_dir}"

cc="${CC:-cc}"
out="${out_dir}/hz7_smoke"
remote_out="${out_dir}/hz7_remote_smoke"

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
