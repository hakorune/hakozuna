#!/usr/bin/env bash
# Sourced by build_linux_hz5_standalone.sh after argument parsing.

hz5_validate_build_config() {
  if [[ "$LINUX_LOCAL2P" -eq 1 && \
        ( "$LINUX_P25_BRIDGE_ATTR" -eq 1 || "$ENABLE_LINUX_P43" -eq 1 ) ]]; then
    echo "local2p, p25attr, and p43 lanes are mutually exclusive" >&2
    exit 1
  fi

  if [[ "$LINUX_LOCAL2P_NO_COOKIE" -eq 1 && \
        "$LINUX_LOCAL2P_NO_CAS" -eq 1 ]]; then
    echo "local2p diagnostic variants are mutually exclusive" >&2
    exit 1
  fi

  if [[ "$LINUX_LOCAL2P_REMOTE_BATCH_CAP" -lt 1 ]]; then
    echo "remote batch cap must be >= 1" >&2
    exit 1
  fi

  if [[ "$LINUX_SMALLFRONT_REMOTE_BATCH_CAP" -lt 1 ]]; then
    echo "smallfront remote batch cap must be >= 1" >&2
    exit 1
  fi

  if [[ "$LINUX_MIDFRONT_REMOTE_BATCH_CAP" -lt 1 ]]; then
    echo "midfront remote batch cap must be >= 1" >&2
    exit 1
  fi

  if [[ "$LINUX_MIDPAGEFRONT_REMOTE_BATCH_CAP" -lt 1 ]]; then
    echo "midpagefront remote batch cap must be >= 1" >&2
    exit 1
  fi

  if [[ "$LINUX_MIDPAGEFRONT_M6_RAW_CAP" -lt 1 ]]; then
    echo "midpagefront M6 raw cap must be >= 1" >&2
    exit 1
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M7_REMOTE_PAGE_CAP" -lt 1 ]]; then
    echo "midpagefront M7 remote page cap must be >= 1" >&2
    exit 1
  fi
  if [[ "$LINUX_MIDPAGEFRONT_M8_XFER_PAGE_CAP" -lt 1 ]]; then
    echo "midpagefront M8 transfer page cap must be >= 1" >&2
    exit 1
  fi

  if [[ "$LINUX_MIDPAGEFRONT_EMPTY_RETAIN_CAP" -lt 1 ]]; then
    echo "midpagefront empty retain cap must be >= 1" >&2
    exit 1
  fi

  if [[ "$LINUX_MIDFRONT_MAX_BYTES" -lt 2049 || \
        "$LINUX_MIDFRONT_MAX_BYTES" -gt 65536 ]]; then
    echo "midfront max bytes must be in 2049..65536" >&2
    exit 1
  fi

  if [[ "$LINUX_MIDFRONT_REMOTE_OUTBOX_SLOTS" -lt 1 ]]; then
    echo "midfront remote outbox slots must be >= 1" >&2
    exit 1
  fi

  if [[ "$LINUX_MIDFRONT_REMOTE_OUTBOX" -eq 1 && \
        "$LINUX_MIDFRONT_REMOTE_RBUF" -eq 1 ]]; then
    echo "midfront remote outbox and rbuf are mutually exclusive" >&2
    exit 1
  fi

  if [[ "$LINUX_MIDFRONT_REMOTE_RBUF_CAP" -lt 1 ]]; then
    echo "midfront remote rbuf cap must be >= 1" >&2
    exit 1
  fi

  if [[ "$LINUX_MIDFRONT_REMOTE_RBUF_FLUSH_THRESHOLD" -lt 1 || \
        "$LINUX_MIDFRONT_REMOTE_RBUF_FLUSH_THRESHOLD" -gt \
        "$LINUX_MIDFRONT_REMOTE_RBUF_CAP" ]]; then
    echo "midfront remote rbuf threshold must be in 1..cap" >&2
    exit 1
  fi

  if [[ "$LINUX_MIDFRONT_REMOTE_TRUST_DRAIN_STATE" -eq 1 && \
        "$LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE" -ne 1 ]]; then
    echo "midfront trust-drain-state requires direct-free-state" >&2
    exit 1
  fi

  if [[ "$LINUX_LARGEFRONT_REMOTE_BATCH_CAP" -lt 1 ]]; then
    echo "largefront remote batch cap must be >= 1" >&2
    exit 1
  fi

  if [[ "$LINUX_LARGEFRONT_MAP_BASE_ONLY" -eq 1 && \
        "$LINUX_LARGEFRONT_REGION_MAP" -eq 1 ]]; then
    echo "largefront base-only and region-map diagnostics are mutually exclusive" >&2
    exit 1
  fi

  if [[ "$LINUX_MIDFRONT_DRAIN_ALL_ON_MISS" -eq 1 && \
        "$LINUX_MIDFRONT_DRAIN_MASK_ON_MISS" -eq 1 ]]; then
    echo "midfront drain-all and drain-mask are mutually exclusive" >&2
    exit 1
  fi

  if [[ "$LINUX_P25_BRIDGE_ATTR" -eq 1 && "$ENABLE_LINUX_P43" -eq 1 ]]; then
    echo "p25attr and p43 lanes are mutually exclusive" >&2
    exit 1
  fi

  if [[ "$LINUX_P25_BRIDGE_ATTR" -eq 1 ]]; then
    p25attr_mode_count=0
    [[ "$LINUX_P25_BRIDGE_ATTR_NO_CAS" -eq 1 ]] && ((p25attr_mode_count += 1))
    [[ "$LINUX_P25_BRIDGE_ATTR_NO_COOKIE" -eq 1 ]] && ((p25attr_mode_count += 1))
    [[ "$LINUX_P25_BRIDGE_ATTR_READONLY_STATE" -eq 1 ]] && ((p25attr_mode_count += 1))
    if [[ "$p25attr_mode_count" -gt 1 ]]; then
      echo "p25attr diagnostic variants are mutually exclusive" >&2
      exit 1
    fi
  fi

  if [[ "$ENABLE_LINUX_P43" -eq 1 ]]; then
    p43_mode_count=0
    [[ "$LINUX_P43_PREPARED_BRIDGE" -eq 0 ]] && ((p43_mode_count += 1))
    [[ "$LINUX_P43_UNSAFE_NO_LOOKUP" -eq 1 ]] && ((p43_mode_count += 1))
    [[ "$LINUX_P43_TRUST_FAST_LOOKUP" -eq 1 ]] && ((p43_mode_count += 1))
    [[ "$LINUX_P43_TRUST_WRAPPER_SOURCE" -eq 1 ]] && ((p43_mode_count += 1))
    [[ "$LINUX_P43_DECODED_RAW_LOOKUP" -eq 1 ]] && ((p43_mode_count += 1))
    [[ "$LINUX_P43_WRAPPER_TOKEN" -eq 1 ]] && ((p43_mode_count += 1))
    if [[ "$p43_mode_count" -gt 1 ]]; then
      echo "P43 lane selectors are mutually exclusive" >&2
      exit 1
    fi
    if [[ "$LINUX_P43_DECODED_RAW_LOOKUP" -eq 0 && \
          ( "$LINUX_P43_RAW_FAST_LOOKUP_ONLY" -eq 1 || \
            "$LINUX_P43_RAW_ALLOCATED_LOOKUP_ONLY" -eq 1 ) ]]; then
      echo "rawlookup submodes require --linux-p43-decoded-raw-lookup" >&2
      exit 1
    fi
    if [[ "$LINUX_P43_RAW_FAST_LOOKUP_ONLY" -eq 1 && \
          "$LINUX_P43_RAW_ALLOCATED_LOOKUP_ONLY" -eq 1 ]]; then
      echo "rawlookup fast/allocated submodes are mutually exclusive" >&2
      exit 1
    fi
    if [[ "$LINUX_P43_WRAPPER_TOKEN_BRIDGE" -eq 1 && \
          "$LINUX_P43_WRAPPER_TOKEN" -eq 0 ]]; then
      echo "--linux-p43-token-bridge requires --linux-p43-token" >&2
      exit 1
    fi
  fi

}
