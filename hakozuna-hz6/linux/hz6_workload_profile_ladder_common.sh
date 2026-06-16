#!/usr/bin/env bash

hz6_workload_parse_k_capacity() {
  local raw="$1"
  case "$raw" in
    *k)
      echo "$(( ${raw%k} * 1024 ))"
      ;;
    *)
      echo "$raw"
      ;;
  esac
}

hz6_workload_parse_static_profile() {
  local profile="$1"
  local -n desc_out="$2"
  local -n source_out="$3"
  local -n route_out="$4"
  if [[ "$profile" =~ ^desc([0-9]+k?)_source([0-9]+k?)_route([0-9]+k?)$ ]]; then
    desc_out="$(hz6_workload_parse_k_capacity "${BASH_REMATCH[1]}")"
    source_out="$(hz6_workload_parse_k_capacity "${BASH_REMATCH[2]}")"
    route_out="$(hz6_workload_parse_k_capacity "${BASH_REMATCH[3]}")"
  else
    echo "invalid profile: ${profile}" >&2
    return 2
  fi
}

hz6_workload_append_proxy_rows() {
  local -n rows_out="$1"
  local iters="$2"
  local rows_csv="$3"
  local row_group
  local row_groups=()

  IFS=',' read -r -a row_groups <<< "$rows_csv"
  for row_group in "${row_groups[@]}"; do
    case "$row_group" in
      small_proxy)
        rows_out+=(
          "redis_proxy 4 ${iters} 2000 16 256"
          "small_object_cache 4 ${iters} 8192 16 1024"
          "mixed_small_cache 4 ${iters} 8192 16 4096"
        )
        ;;
      cache_proxy)
        rows_out+=(
          "mixed_object_cache 4 ${iters} 8192 64 8192"
          "midpage_cache 4 ${iters} 4096 4096 32768"
          "wide_midpage_cache 4 ${iters} 8192 4096 32768"
        )
        ;;
      all)
        rows_out+=(
          "redis_proxy 4 ${iters} 2000 16 256"
          "small_object_cache 4 ${iters} 8192 16 1024"
          "mixed_small_cache 4 ${iters} 8192 16 4096"
          "mixed_object_cache 4 ${iters} 8192 64 8192"
          "midpage_cache 4 ${iters} 4096 4096 32768"
          "wide_midpage_cache 4 ${iters} 8192 4096 32768"
        )
        ;;
      "")
        ;;
      *)
        echo "unknown row group: ${row_group}" >&2
        return 2
        ;;
    esac
  done
}

hz6_workload_append_gap_diag_rows() {
  local -n rows_out="$1"
  local iters="$2"
  local rows_csv="$3"
  local row_group
  local row_groups=()

  IFS=',' read -r -a row_groups <<< "$rows_csv"
  for row_group in "${row_groups[@]}"; do
    case "$row_group" in
      small_object_cache)
        rows_out+=("small_object_cache 4 ${iters} 8192 16 1024")
        ;;
      mixed_small_cache)
        rows_out+=("mixed_small_cache 4 ${iters} 8192 16 4096")
        ;;
      mixed_object_cache)
        rows_out+=("mixed_object_cache 4 ${iters} 8192 64 8192")
        ;;
      wide_midpage_cache)
        rows_out+=("wide_midpage_cache 4 ${iters} 8192 4096 32768")
        ;;
      all)
        rows_out+=(
          "small_object_cache 4 ${iters} 8192 16 1024"
          "mixed_small_cache 4 ${iters} 8192 16 4096"
          "mixed_object_cache 4 ${iters} 8192 64 8192"
          "wide_midpage_cache 4 ${iters} 8192 4096 32768"
        )
        ;;
      "")
        ;;
      *)
        echo "unknown row: ${row_group}" >&2
        return 2
        ;;
    esac
  done
}
