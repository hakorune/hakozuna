.PHONY: all all_ldpreload clean
.NOTPARALLEL: clean

CC ?= gcc
CFLAGS ?= -O3 -fPIC -Wall -Wextra -Werror -std=c11
LDFLAGS ?=
LDLIBS ?=
LDLIBS += -pthread -ldl

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)

HZ3_DIR := $(ROOT)/hakozuna/hz3
HZ3_HYBRID_DIR := $(HZ3_DIR)/hybrid
OUT_DIR := $(HZ3_DIR)/out
OUT_LDPRELOAD_DIR := $(HZ3_DIR)/out_ldpreload

INC := -I$(ROOT)/hakozuna/hz3/include

LDPRELOAD_CFLAGS_EXTRA ?= -ftls-model=initial-exec -fno-plt
LDPRELOAD_LDFLAGS_EXTRA ?=
LDPRELOAD_LDLIBS_EXTRA ?=
LDPRELOAD_BSYMBOLIC ?= 1

# S122: Bin Count Policy (0=u16, 1=u32, 2=lazy, 3=nocount+u16, 4=nocount+u32, 5=split)
# Usage: make -C hakozuna/hz3 clean all_ldpreload_scale HZ3_BIN_COUNT_POLICY=5
HZ3_BIN_COUNT_POLICY ?= 1

# S28-4: LTO A/B (build-only, no behavior change)
# Usage:
#   make -C hakozuna/hz3 clean all_ldpreload HZ3_LTO=0
#   make -C hakozuna/hz3 clean all_ldpreload HZ3_LTO=1
HZ3_LTO ?= 0
ifeq ($(HZ3_LTO),1)
LDPRELOAD_CFLAGS_EXTRA += -flto
LDPRELOAD_LDFLAGS_EXTRA += -flto
endif

# Enable hz3 by default for hz3's own LD_PRELOAD build.
# (hz3_config.h keeps safe defaults for other build contexts.)
#
# Default profile (SSOT as of S17):
# - Small v2 + self-describing segments
# - PageTagMap (PTAG) for v2 + v1-medium (unified dispatch)
# - PTAG dst/bin direct (free hot path shortest)
#
# Override example (back to minimal enable/forward-only off):
#   make -C hakozuna/hz3 all_ldpreload \
#     HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0'
HZ3_LDPRELOAD_DEFS_BASE ?= -DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
  -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 \
  -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 \
  -DHZ3_BIN_COUNT_POLICY=$(HZ3_BIN_COUNT_POLICY) \
  -DHZ3_LOCAL_BINS_SPLIT=1 \
  -DHZ3_TCACHE_INIT_ON_MISS=1 \
  -DHZ3_BIN_PAD_LOG2=8

# PTAG mode defaults (shared across lanes)
HZ3_LDPRELOAD_DEFS_PTAG32 ?= \
  -DHZ3_TCACHE_SOA=1 \
  -DHZ3_PTAG_DSTBIN_ENABLE=1 \
  -DHZ3_PTAG_DSTBIN_FASTLOOKUP=1 \
  -DHZ3_PTAG32_NOINRANGE=1 \
  -DHZ3_REALLOC_PTAG32=1 -DHZ3_USABLE_SIZE_PTAG32=1

HZ3_LDPRELOAD_DEFS_PTAG16 ?= \
  -DHZ3_TCACHE_SOA=0 \
  -DHZ3_PTAG_DSTBIN_ENABLE=0 \
  -DHZ3_PTAG_DSTBIN_FASTLOOKUP=0 \
  -DHZ3_PTAG32_NOINRANGE=0 \
  -DHZ3_REALLOC_PTAG32=0 -DHZ3_USABLE_SIZE_PTAG32=0

# PTAG32-on (scale lane default)
HZ3_LDPRELOAD_DEFS_SCALE ?= $(HZ3_LDPRELOAD_DEFS_BASE) $(HZ3_LDPRELOAD_DEFS_PTAG32) \
  -DHZ3_S142_CENTRAL_LOCKFREE=1 -DHZ3_S142_XFER_LOCKFREE=1

# PTAG32-only (p32 lane)
HZ3_LDPRELOAD_DEFS_P32 ?= $(HZ3_LDPRELOAD_DEFS_BASE) $(HZ3_LDPRELOAD_DEFS_PTAG32) \
  -DHZ3_PTAG32_ONLY=1 -DHZ3_S142_CENTRAL_LOCKFREE=1 -DHZ3_S142_XFER_LOCKFREE=1

# PTAG16-primary (fast lane default)
HZ3_LDPRELOAD_DEFS_FAST ?= $(HZ3_LDPRELOAD_DEFS_BASE) $(HZ3_LDPRELOAD_DEFS_PTAG16)

# Backward-compatible alias (default = scale)
HZ3_LDPRELOAD_DEFS ?= $(HZ3_LDPRELOAD_DEFS_SCALE)

# Extra compile-time defines for A/B (appended after HZ3_LDPRELOAD_DEFS).
# Prefer this to overriding HZ3_LDPRELOAD_DEFS with a single -D flag (which can
# accidentally drop required defaults like HZ3_ENABLE=1).
HZ3_LDPRELOAD_DEFS_EXTRA ?=

CFLAGS_LDPRELOAD := $(CFLAGS) $(LDPRELOAD_CFLAGS_EXTRA)
LDFLAGS_LDPRELOAD := $(LDFLAGS) $(LDPRELOAD_LDFLAGS_EXTRA)
ifneq ($(LDPRELOAD_BSYMBOLIC),0)
LDFLAGS_LDPRELOAD += -Wl,-Bsymbolic-functions
endif
LDLIBS_LDPRELOAD := $(LDLIBS) $(LDPRELOAD_LDLIBS_EXTRA)

LDPRELOAD_LIB := $(ROOT)/libhakozuna_hz3_ldpreload.so

HZ3_SRCS := $(wildcard $(HZ3_DIR)/src/*.c)
HZ3_OBJS := $(patsubst $(HZ3_DIR)/src/%.c,$(OUT_DIR)/%.o,$(HZ3_SRCS))
HZ3_LDPRELOAD_OBJS := $(patsubst $(HZ3_DIR)/src/%.c,$(OUT_LDPRELOAD_DIR)/%.o,$(HZ3_SRCS))

all: $(OUT_DIR) $(HZ3_OBJS)

# S41: fast/scale lane separation
.PHONY: all_ldpreload_fast all_ldpreload_scale all_ldpreload_scale_p32

# Output directories
OUT_LDPRELOAD_FAST_DIR := $(HZ3_DIR)/out_ldpreload_fast
OUT_LDPRELOAD_SCALE_DIR := $(HZ3_DIR)/out_ldpreload_scale
OUT_LDPRELOAD_P32_DIR := $(HZ3_DIR)/out_ldpreload_p32

# Library targets
LDPRELOAD_FAST_LIB := $(ROOT)/libhakozuna_hz3_fast.so
LDPRELOAD_SCALE_LIB := $(ROOT)/libhakozuna_hz3_scale.so
LDPRELOAD_SCALE_P32_LIB := $(ROOT)/libhakozuna_hz3_scale_p32.so
LDPRELOAD_SCALE_R50_LIB := $(ROOT)/libhakozuna_hz3_scale_r50.so
LDPRELOAD_SCALE_R50_S94_LIB := $(ROOT)/libhakozuna_hz3_scale_r50_s94.so
LDPRELOAD_SCALE_R50_S97_1_LIB := $(ROOT)/libhakozuna_hz3_scale_r50_s97_1.so
LDPRELOAD_SCALE_R50_S97_8_LIB := $(ROOT)/libhakozuna_hz3_scale_r50_s97_8.so
LDPRELOAD_SCALE_R90_LIB := $(ROOT)/libhakozuna_hz3_scale_r90.so
LDPRELOAD_SCALE_R90_PF2_LIB := $(ROOT)/libhakozuna_hz3_scale_r90_pf2.so
LDPRELOAD_SCALE_R90_PF2_S67_LIB := $(ROOT)/libhakozuna_hz3_scale_r90_pf2_s67.so
LDPRELOAD_SCALE_R90_PF2_S97_LIB := $(ROOT)/libhakozuna_hz3_scale_r90_pf2_s97.so
LDPRELOAD_SCALE_R90_PF2_S97_2_LIB := $(ROOT)/libhakozuna_hz3_scale_r90_pf2_s97_2.so
LDPRELOAD_SCALE_R90_PF2_S97_8_T8_LIB := $(ROOT)/libhakozuna_hz3_scale_r90_pf2_s97_8_t8.so
LDPRELOAD_SCALE_TOLERANT_LIB := $(ROOT)/libhakozuna_hz3_scale_tolerant.so
LDPRELOAD_SCALE_S118_64_LIB := $(ROOT)/libhakozuna_hz3_scale_s118_64.so

# Common parameter sets for scale variants (reduce duplication)
SCALE_PARAMS_R50 := HZ3_SCALE_NUM_SHARDS=56 HZ3_SCALE_S74_REFILL_BURST=16 HZ3_SCALE_S74_FLUSH_BATCH=64 HZ3_SCALE_S74_STATS=0
SCALE_PARAMS_R90 := HZ3_SCALE_NUM_SHARDS=63 HZ3_SCALE_S74_REFILL_BURST=8 HZ3_SCALE_S74_FLUSH_BATCH=64 HZ3_SCALE_S74_STATS=0

# Object lists
HZ3_LDPRELOAD_FAST_OBJS := $(patsubst $(HZ3_DIR)/src/%.c,$(OUT_LDPRELOAD_FAST_DIR)/%.o,$(HZ3_SRCS))
HZ3_LDPRELOAD_SCALE_OBJS := $(patsubst $(HZ3_DIR)/src/%.c,$(OUT_LDPRELOAD_SCALE_DIR)/%.o,$(HZ3_SRCS))
HZ3_LDPRELOAD_P32_OBJS := $(patsubst $(HZ3_DIR)/src/%.c,$(OUT_LDPRELOAD_P32_DIR)/%.o,$(HZ3_SRCS))

# fast lane (current behavior)
all_ldpreload_fast: $(LDPRELOAD_FAST_LIB)

# scale lane (32 shards + sparse)
all_ldpreload_scale: $(LDPRELOAD_SCALE_LIB)

# PTAG32-only lane (p32)
all_ldpreload_scale_p32: $(LDPRELOAD_SCALE_P32_LIB)

# Default: scale lane + symlink
all_ldpreload: all_ldpreload_scale
	@ln -sf $(notdir $(LDPRELOAD_SCALE_LIB)) $(LDPRELOAD_LIB)

# Preset lanes (scale variants; keep fast lane minimal)
.PHONY: all_ldpreload_scale_r50 all_ldpreload_scale_r50_s94 all_ldpreload_scale_r50_s97_1 all_ldpreload_scale_r50_s97_8 all_ldpreload_scale_r90 all_ldpreload_scale_r90_pf2 all_ldpreload_scale_r90_pf2_s67 all_ldpreload_scale_r90_pf2_s97 all_ldpreload_scale_r90_pf2_s97_2 all_ldpreload_scale_r90_pf2_s97_8_t8 all_ldpreload_scale_tolerant all_ldpreload_scale_s118_64

# r50: balanced workload oriented (shards=56, burst=16)
all_ldpreload_scale_r50:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale $(SCALE_PARAMS_R50)
	@cp -f $(LDPRELOAD_SCALE_LIB) $(LDPRELOAD_SCALE_R50_LIB)

# r50_s94: r50 oriented + S94 SpillLite (r90 NO-GO, r50 opt-in only)
all_ldpreload_scale_r50_s94:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale $(SCALE_PARAMS_R50) \
	    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S94_SPILL_LITE=1 -DHZ3_S95_S94_OVERFLOW_O1=1 -DHZ3_S94_SPILL_SC_MAX=16 -DHZ3_S94_SPILL_CAP=16'
	@cp -f $(LDPRELOAD_SCALE_LIB) $(LDPRELOAD_SCALE_R50_S94_LIB)

# r50_s97_1: r50 oriented + S97-1 bucketize (r90 NO-GO)
all_ldpreload_scale_r50_s97_1:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale $(SCALE_PARAMS_R50) \
	    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S97_REMOTE_STASH_BUCKET=1 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1'
	@cp -f $(LDPRELOAD_SCALE_LIB) $(LDPRELOAD_SCALE_R50_S97_1_LIB)

# r50_s97_8: r50 oriented + S97-8 sort+group bucketize (stack-only, no TLS tables)
all_ldpreload_scale_r50_s97_8:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale $(SCALE_PARAMS_R50) \
	    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S97_REMOTE_STASH_BUCKET=6 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1'
	@cp -f $(LDPRELOAD_SCALE_LIB) $(LDPRELOAD_SCALE_R50_S97_8_LIB)

# r90: remote-heavy oriented (shards=63, burst=8)
all_ldpreload_scale_r90:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale $(SCALE_PARAMS_R90)
	@cp -f $(LDPRELOAD_SCALE_LIB) $(LDPRELOAD_SCALE_R90_LIB)

# r90_pf2: remote-heavy oriented + S44 pop_batch prefetch distance=2 (r90 GO, r50 NO-GO)
all_ldpreload_scale_r90_pf2:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale $(SCALE_PARAMS_R90) \
	    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2'
	@cp -f $(LDPRELOAD_SCALE_LIB) $(LDPRELOAD_SCALE_R90_PF2_LIB)

# r90_pf2_s67: remote-heavy oriented + S44 prefetch dist=2 + S67 spill array2 (r90 GO, r50 NO-GO)
all_ldpreload_scale_r90_pf2_s67:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale $(SCALE_PARAMS_R90) \
	    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S67_SPILL_ARRAY2=1 -DHZ3_S67_DRAIN_LIMIT=1 -DHZ3_S67_SPILL_CAP_OVERRIDE=1 -DHZ3_S67_SPILL_CAP=64'
	@cp -f $(LDPRELOAD_SCALE_LIB) $(LDPRELOAD_SCALE_R90_PF2_S67_LIB)

# r90_pf2_s97: remote-heavy oriented + S97-1 bucketize within flush_budget (legacy target name; r90 may be NO-GO due to probe branch-miss)
all_ldpreload_scale_r90_pf2_s97:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale $(SCALE_PARAMS_R90) \
	    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_BUCKET=1 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1'
	@cp -f $(LDPRELOAD_SCALE_LIB) $(LDPRELOAD_SCALE_R90_PF2_S97_LIB)

# r90_pf2_s97_2: remote-heavy oriented + S97-2 direct-map bucketize (probe-less)
# Note (2026-01): thread sensitivity observed; tends to win on r90 when threads >= 16, and lose at low thread counts.
all_ldpreload_scale_r90_pf2_s97_2:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale $(SCALE_PARAMS_R90) \
	    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_BUCKET=2 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1'
	@cp -f $(LDPRELOAD_SCALE_LIB) $(LDPRELOAD_SCALE_R90_PF2_S97_2_LIB)

# r90_pf2_s97_8_t8: remote-heavy oriented + S97-8 sort+group bucketize (stack-only)
# Note (2026-01): tends to win at low thread counts (e.g., T=8), but can be NO-GO at threads >= 16.
all_ldpreload_scale_r90_pf2_s97_8_t8:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale $(SCALE_PARAMS_R90) \
	    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_BUCKET=6 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1'
	@cp -f $(LDPRELOAD_SCALE_LIB) $(LDPRELOAD_SCALE_R90_PF2_S97_8_T8_LIB)

# tolerant: allow shard collision (use for larson/flood 100+ threads; slower but safe)
all_ldpreload_scale_tolerant:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale $(SCALE_PARAMS_R90) \
	    HZ3_SCALE_COLLISION_FAILFAST=0 \
	    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_LANE_SPLIT=1 -DHZ3_OWNER_LEASE_ENABLE=1'
	@cp -f $(LDPRELOAD_SCALE_LIB) $(LDPRELOAD_SCALE_TOLERANT_LIB)

# s118_64: refill batch tuning (workload-split; opt-in only)
all_ldpreload_scale_s118_64:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale \
	    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S118_SMALL_V2_REFILL_BATCH=64'
	@cp -f $(LDPRELOAD_SCALE_LIB) $(LDPRELOAD_SCALE_S118_64_LIB)

# S138: SmallMaxSize A/B test (max=1024 vs baseline=2048)
# CRITICAL: HZ3_SUB4K_ENABLE=1 必須（これがないと1025-4095BがMedium 4096Bに丸められる）
all_ldpreload_scale_s138_1024:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale \
	    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_SMALL_MAX_SIZE_OVERRIDE=1024 -DHZ3_SUB4K_ENABLE=1'
	@cp -f $(LDPRELOAD_SCALE_LIB) $(ROOT)/libhakozuna_hz3_scale_s138_1024.so
	@echo "S138 treat build (max=1024, sub4k enabled) -> libhakozuna_hz3_scale_s138_1024.so"

$(OUT_DIR):
	@mkdir -p $(OUT_DIR)

$(OUT_LDPRELOAD_DIR):
	@mkdir -p $(OUT_LDPRELOAD_DIR)

# S41: fast lane build
$(OUT_LDPRELOAD_FAST_DIR):
	@mkdir -p $(OUT_LDPRELOAD_FAST_DIR)

$(OUT_LDPRELOAD_FAST_DIR)/%.o: $(HZ3_DIR)/src/%.c | $(OUT_LDPRELOAD_FAST_DIR)
	$(CC) $(CFLAGS_LDPRELOAD) $(HZ3_LDPRELOAD_DEFS_FAST) $(HZ3_LDPRELOAD_DEFS_EXTRA) $(INC) -c $< -o $@

$(LDPRELOAD_FAST_LIB): $(OUT_LDPRELOAD_FAST_DIR) $(HZ3_LDPRELOAD_FAST_OBJS)
	$(CC) -shared -o $@ $(HZ3_LDPRELOAD_FAST_OBJS) $(LDFLAGS_LDPRELOAD) $(LDLIBS_LDPRELOAD)

# S41: scale lane build
# NOTE: Scale lane raises arena size to avoid shard*class segment exhaustion.
# S42: Small transfer cache enabled for remote-heavy optimization.
# S44: Owner shared stash enabled for remote-heavy optimization (MPSC).
# S44-3: Owner stash fast pop (O(1) when old_head==NULL) + count disabled.
# S45: Memory Budget Box - segment-level reclaim on arena exhaustion.
# S45-FOCUS: Focused reclaim - concentrate on ONE segment to reach free_pages==512.
# S45-FOCUS enhanced: Multi-pass + remote flush for zero alloc-failed.
# S50: Large cache size-class box for O(1) cache hit.
# S46: Global Pressure Box - broadcast arena pressure to all threads.
# S49: Segment Packing - existing segment priority allocation.
# NOTE: `HZ3_ARENA_SIZE` is defined in this lane; override via `HZ3_SCALE_ARENA_SIZE=...` (avoid -D redefinition Werror).
HZ3_SCALE_ARENA_SIZE ?= 0x1000000000ULL

# Default shard count for scale lane.
# Note: collision fail-fast counts ALL threads that touch the allocator (including main thread),
# so shards should have some slack vs "worker thread count".
HZ3_SCALE_NUM_SHARDS ?= 63
HZ3_SCALE_SHARDS_DEF := -DHZ3_NUM_SHARDS=$(HZ3_SCALE_NUM_SHARDS)

# Default shard count for p32 lane (PTAG32-only).
HZ3_P32_NUM_SHARDS ?= 96
HZ3_P32_SHARDS_DEF := -DHZ3_NUM_SHARDS=$(HZ3_P32_NUM_SHARDS)

# Shard collision is a correctness hazard for scale lane (many "my_shard only" boxes).
# Make it fail-fast by default (override via `HZ3_SCALE_COLLISION_FAILFAST=0` if needed).
HZ3_SCALE_COLLISION_FAILFAST ?= 1
HZ3_SCALE_COLLISION_SHOT ?= 1

# S74: LaneBatchRefill/FlushBox presets (scale lane only)
# Default: ON with conservative parameters (remote-heavy safety first).
# NOTE: Use make vars for A/B (avoid -D redefinition under -Werror).
HZ3_SCALE_S74_LANE_BATCH ?= 1
HZ3_SCALE_S74_REFILL_BURST ?= 8
HZ3_SCALE_S74_FLUSH_BATCH ?= 64
HZ3_SCALE_S74_STATS ?= 0

# S46: Global Pressure Box (enable via make var for A/B testing)
HZ3_SCALE_ARENA_PRESSURE_BOX ?= 0
# S51: Large madvise policy (enable via make var for A/B testing)
HZ3_SCALE_S51_LARGE_MADVISE ?= 0

HZ3_SCALE_DEFS := -DHZ3_REMOTE_STASH_SPARSE=1 \
	-DHZ3_SHARD_COLLISION_FAILFAST=$(HZ3_SCALE_COLLISION_FAILFAST) \
	-DHZ3_SHARD_COLLISION_SHOT=$(HZ3_SCALE_COLLISION_SHOT) \
	-DHZ3_ARENA_SIZE=$(HZ3_SCALE_ARENA_SIZE) \
	-DHZ3_SEG_SIZE=0x100000 -DHZ3_SEG_SHIFT=20 \
	-DHZ3_S42_SMALL_XFER=1 \
	-DHZ3_S44_OWNER_STASH=1 \
	-DHZ3_S44_OWNER_STASH_FASTPOP=1 \
	-DHZ3_S44_OWNER_STASH_COUNT=0 \
	-DHZ3_MEM_BUDGET_ENABLE=1 \
	-DHZ3_S45_FOCUS_RECLAIM=1 \
	-DHZ3_S45_EMERGENCY_FLUSH_REMOTE=1 \
	-DHZ3_S50_LARGE_SCACHE=1 \
	-DHZ3_S50_LARGE_SCACHE_EVICT=1 \
	-DHZ3_S51_LARGE_MADVISE=$(HZ3_SCALE_S51_LARGE_MADVISE) \
	-DHZ3_S52_LARGE_BESTFIT=1 \
	-DHZ3_S52_BESTFIT_RANGE=4 \
	-DHZ3_LARGE_CACHE_MAX_BYTES='(8192ULL << 20)' \
	-DHZ3_ARENA_PRESSURE_BOX=$(HZ3_SCALE_ARENA_PRESSURE_BOX) \
	-DHZ3_S47_SEGMENT_QUARANTINE=1 \
	-DHZ3_S49_SEGMENT_PACKING=1 \
	-DHZ3_S62_RETIRE=1 \
	-DHZ3_S62_PURGE=1 \
	-DHZ3_S65_RELEASE_BOUNDARY=1 \
	-DHZ3_S65_RELEASE_LEDGER=1 \
	-DHZ3_S65_MEDIUM_RECLAIM=1 \
	-DHZ3_S65_IDLE_GATE=1 \
	-DHZ3_S74_LANE_BATCH=$(HZ3_SCALE_S74_LANE_BATCH) \
	-DHZ3_S74_REFILL_BURST=$(HZ3_SCALE_S74_REFILL_BURST) \
	-DHZ3_S74_FLUSH_BATCH=$(HZ3_SCALE_S74_FLUSH_BATCH) \
	-DHZ3_S74_STATS=$(HZ3_SCALE_S74_STATS)

# S53-2: mem-mode presets (opt-in lanes)
# MEM_MSTRESS: RSS重視（mstress/MT向け）
HZ3_MEM_MSTRESS_NUM_SHARDS ?= 32
# NOTE: S64 retire-scan is currently unstable in 32-shards non-collision mstress
# (sporadic segfault / memory corruption). Keep default OFF and enable explicitly
# for experiments while the root cause is being debugged.
HZ3_MEM_MSTRESS_S64_RETIRE_SCAN ?= 0
HZ3_MEM_MSTRESS_S69_LIVECOUNT ?= 0
# S86: Extra defines for debug (e.g., HZ3_MEM_MSTRESS_EXTRA='-DHZ3_S86_CENTRAL_SHADOW=1')
HZ3_MEM_MSTRESS_EXTRA ?=
HZ3_MEM_MSTRESS_DEFS := -DHZ3_LARGE_CACHE_BUDGET=1 \
	-DHZ3_LARGE_CACHE_SOFT_BYTES='((128ULL << 20))' \
	-DHZ3_S53_MADVISE_MIN_INTERVAL=16 \
	-DHZ3_S53_MADVISE_RATE_PAGES=256 \
	-DHZ3_NUM_SHARDS=$(HZ3_MEM_MSTRESS_NUM_SHARDS) \
	-DHZ3_S64_RETIRE_SCAN=$(HZ3_MEM_MSTRESS_S64_RETIRE_SCAN) \
	-DHZ3_S69_LIVECOUNT=$(HZ3_MEM_MSTRESS_S69_LIVECOUNT) \
	-DHZ3_S80_MEDIUM_RECLAIM_GATE=1 \
	-DHZ3_S80_MEDIUM_RECLAIM_DEFAULT=0 \
	$(HZ3_MEM_MSTRESS_EXTRA)

# MEM_LARGE: 速度重視（malloc-large向け）
HZ3_MEM_LARGE_NUM_SHARDS ?= 32
HZ3_MEM_LARGE_DEFS := -DHZ3_LARGE_CACHE_BUDGET=1 \
	-DHZ3_LARGE_CACHE_SOFT_BYTES='((512ULL << 20))' \
	-DHZ3_S53_MADVISE_MIN_INTERVAL=64 \
	-DHZ3_S53_MADVISE_RATE_PAGES=4096 \
	-DHZ3_NUM_SHARDS=$(HZ3_MEM_LARGE_NUM_SHARDS)

$(OUT_LDPRELOAD_SCALE_DIR):
	@mkdir -p $(OUT_LDPRELOAD_SCALE_DIR)

$(OUT_LDPRELOAD_SCALE_DIR)/%.o: $(HZ3_DIR)/src/%.c | $(OUT_LDPRELOAD_SCALE_DIR)
	$(CC) $(CFLAGS_LDPRELOAD) $(HZ3_LDPRELOAD_DEFS) $(HZ3_LDPRELOAD_DEFS_EXTRA) \
	    $(HZ3_SCALE_SHARDS_DEF) $(HZ3_SCALE_DEFS) $(INC) -c $< -o $@

$(LDPRELOAD_SCALE_LIB): $(OUT_LDPRELOAD_SCALE_DIR) $(HZ3_LDPRELOAD_SCALE_OBJS)
	$(CC) -shared -o $@ $(HZ3_LDPRELOAD_SCALE_OBJS) $(LDFLAGS_LDPRELOAD) $(LDLIBS_LDPRELOAD)

$(OUT_LDPRELOAD_P32_DIR):
	@mkdir -p $(OUT_LDPRELOAD_P32_DIR)

$(OUT_LDPRELOAD_P32_DIR)/%.o: $(HZ3_DIR)/src/%.c | $(OUT_LDPRELOAD_P32_DIR)
	$(CC) $(CFLAGS_LDPRELOAD) $(HZ3_LDPRELOAD_DEFS_P32) $(HZ3_LDPRELOAD_DEFS_EXTRA) \
	    $(HZ3_P32_SHARDS_DEF) $(HZ3_SCALE_DEFS) $(INC) -c $< -o $@

$(LDPRELOAD_SCALE_P32_LIB): $(OUT_LDPRELOAD_P32_DIR) $(HZ3_LDPRELOAD_P32_OBJS)
	$(CC) -shared -o $@ $(HZ3_LDPRELOAD_P32_OBJS) $(LDFLAGS_LDPRELOAD) $(LDLIBS_LDPRELOAD)

# S53-2 mem lanes (copy of scale lane with different DEFS)
.PHONY: all_ldpreload_scale_mem_mstress all_ldpreload_scale_mem_large
.PHONY: rebuild_ldpreload_scale rebuild_ldpreload_scale_mem_mstress rebuild_ldpreload_scale_mem_large
.NOTPARALLEL: rebuild_ldpreload_scale rebuild_ldpreload_scale_mem_mstress rebuild_ldpreload_scale_mem_large

OUT_LDPRELOAD_MEM_MSTRESS_DIR := $(HZ3_DIR)/out_ldpreload_mem_mstress
OUT_LDPRELOAD_MEM_LARGE_DIR := $(HZ3_DIR)/out_ldpreload_mem_large

LDPRELOAD_MEM_MSTRESS_LIB := $(ROOT)/libhakozuna_hz3_scale_mem_mstress.so
LDPRELOAD_MEM_LARGE_LIB := $(ROOT)/libhakozuna_hz3_scale_mem_large.so

HZ3_LDPRELOAD_MEM_MSTRESS_OBJS := $(patsubst $(HZ3_DIR)/src/%.c,$(OUT_LDPRELOAD_MEM_MSTRESS_DIR)/%.o,$(HZ3_SRCS))
HZ3_LDPRELOAD_MEM_LARGE_OBJS := $(patsubst $(HZ3_DIR)/src/%.c,$(OUT_LDPRELOAD_MEM_LARGE_DIR)/%.o,$(HZ3_SRCS))

all_ldpreload_scale_mem_mstress: $(LDPRELOAD_MEM_MSTRESS_LIB)
all_ldpreload_scale_mem_large: $(LDPRELOAD_MEM_LARGE_LIB)

# NOTE: Do not run `make -j clean <build-target>` (clean may race with compile).
rebuild_ldpreload_scale:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale

rebuild_ldpreload_scale_mem_mstress:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale_mem_mstress

rebuild_ldpreload_scale_mem_large:
	@$(MAKE) clean
	@$(MAKE) all_ldpreload_scale_mem_large

$(OUT_LDPRELOAD_MEM_MSTRESS_DIR):
	@mkdir -p $(OUT_LDPRELOAD_MEM_MSTRESS_DIR)

$(OUT_LDPRELOAD_MEM_LARGE_DIR):
	@mkdir -p $(OUT_LDPRELOAD_MEM_LARGE_DIR)

$(OUT_LDPRELOAD_MEM_MSTRESS_DIR)/%.o: $(HZ3_DIR)/src/%.c | $(OUT_LDPRELOAD_MEM_MSTRESS_DIR)
	$(CC) $(CFLAGS_LDPRELOAD) $(HZ3_LDPRELOAD_DEFS) $(HZ3_LDPRELOAD_DEFS_EXTRA) \
	    $(HZ3_SCALE_DEFS) $(HZ3_MEM_MSTRESS_DEFS) $(INC) -c $< -o $@

$(OUT_LDPRELOAD_MEM_LARGE_DIR)/%.o: $(HZ3_DIR)/src/%.c | $(OUT_LDPRELOAD_MEM_LARGE_DIR)
	$(CC) $(CFLAGS_LDPRELOAD) $(HZ3_LDPRELOAD_DEFS) $(HZ3_LDPRELOAD_DEFS_EXTRA) \
	    $(HZ3_SCALE_DEFS) $(HZ3_MEM_LARGE_DEFS) $(INC) -c $< -o $@

$(LDPRELOAD_MEM_MSTRESS_LIB): $(OUT_LDPRELOAD_MEM_MSTRESS_DIR) $(HZ3_LDPRELOAD_MEM_MSTRESS_OBJS)
	$(CC) -shared -o $@ $(HZ3_LDPRELOAD_MEM_MSTRESS_OBJS) $(LDFLAGS_LDPRELOAD) $(LDLIBS_LDPRELOAD)

$(LDPRELOAD_MEM_LARGE_LIB): $(OUT_LDPRELOAD_MEM_LARGE_DIR) $(HZ3_LDPRELOAD_MEM_LARGE_OBJS)
	$(CC) -shared -o $@ $(HZ3_LDPRELOAD_MEM_LARGE_OBJS) $(LDFLAGS_LDPRELOAD) $(LDLIBS_LDPRELOAD)

$(OUT_DIR)/%.o: $(HZ3_DIR)/src/%.c | $(OUT_DIR)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(OUT_LDPRELOAD_DIR)/%.o: $(HZ3_DIR)/src/%.c | $(OUT_LDPRELOAD_DIR)
	$(CC) $(CFLAGS_LDPRELOAD) $(HZ3_LDPRELOAD_DEFS) $(HZ3_LDPRELOAD_DEFS_EXTRA) $(INC) -c $< -o $@

$(LDPRELOAD_LIB): $(OUT_LDPRELOAD_DIR) $(HZ3_LDPRELOAD_OBJS)
	$(CC) -shared -o $@ $(HZ3_LDPRELOAD_OBJS) $(LDFLAGS_LDPRELOAD) $(LDLIBS_LDPRELOAD)

clean:
	@rm -rf $(OUT_DIR) $(OUT_LDPRELOAD_DIR) $(OUT_HYBRID_DIR) \
	        $(OUT_LDPRELOAD_FAST_DIR) $(OUT_LDPRELOAD_SCALE_DIR) \
	        $(OUT_LDPRELOAD_MEM_MSTRESS_DIR) $(OUT_LDPRELOAD_MEM_LARGE_DIR) \
	        $(LDPRELOAD_LIB) $(LDPRELOAD_FAST_LIB) $(LDPRELOAD_SCALE_LIB) \
	        $(LDPRELOAD_MEM_MSTRESS_LIB) $(LDPRELOAD_MEM_LARGE_LIB)

# ============================================================================
# Day 8: Hybrid LD_PRELOAD (hz3 + hakozuna)
# ============================================================================

HYBRID_LDPRELOAD_LIB := $(ROOT)/libhakozuna_hybrid_ldpreload.so
OUT_HYBRID_DIR := $(HZ3_DIR)/out_hybrid

# Include both hz3 and hakozuna headers
INC_HYBRID := $(INC) -I$(ROOT)/hakozuna/include

# hz3 objects (shim excluded - we use hybrid shim instead)
HZ3_HYBRID_OBJS := $(OUT_HYBRID_DIR)/hz3_hot.o \
    $(OUT_HYBRID_DIR)/hz3_segmap.o \
    $(OUT_HYBRID_DIR)/hz3_segment.o \
    $(OUT_HYBRID_DIR)/hz3_tcache.o \
    $(OUT_HYBRID_DIR)/hz3_central.o \
    $(OUT_HYBRID_DIR)/hz3_inbox.o \
    $(OUT_HYBRID_DIR)/hz3_epoch.o \
    $(OUT_HYBRID_DIR)/hz3_knobs.o \
    $(OUT_HYBRID_DIR)/hz3_learn.o

# hakozuna objects (shim excluded - we use hybrid shim instead)
HZ_HYBRID_OBJS := $(ROOT)/hakozuna/out_ldpreload/hz_dir.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_segment.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_segmem.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_page.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_pagemap.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_remote.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_remote_notify.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_remote_mailbox.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_remote_policy.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_central.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_transfer.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_tcache.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_large.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_large_pool.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_lcache.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_learn.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_df.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_canary.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_leak.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_foreign.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_batchpool.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_transferlist.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_stats.o \
    $(ROOT)/hakozuna/out_ldpreload/hz_rseq.o

# Hybrid shim object
HYBRID_SHIM_SRC := $(HZ3_HYBRID_DIR)/hz3_hybrid_shim.c
HYBRID_SHIM_OBJ := $(OUT_HYBRID_DIR)/hz3_hybrid_shim.o

$(OUT_HYBRID_DIR):
	@mkdir -p $(OUT_HYBRID_DIR)

# Compile hz3 sources for hybrid (with both hz3 and hakozuna includes)
# IMPORTANT: Enable hz3 allocator (HZ3_ENABLE=1, HZ3_SHIM_FORWARD_ONLY=0)
$(OUT_HYBRID_DIR)/%.o: $(HZ3_DIR)/src/%.c | $(OUT_HYBRID_DIR)
	$(CC) $(CFLAGS_LDPRELOAD) $(INC_HYBRID) -DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 -c $< -o $@

$(HYBRID_SHIM_OBJ): $(HYBRID_SHIM_SRC) | $(OUT_HYBRID_DIR)
	$(CC) $(CFLAGS_LDPRELOAD) $(INC_HYBRID) -DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 -c $< -o $@

# Ensure hakozuna objects are built first
.PHONY: hakozuna_ldpreload_objs
hakozuna_ldpreload_objs:
	$(MAKE) -C $(ROOT)/hakozuna all_ldpreload

# Build hybrid library
$(HYBRID_LDPRELOAD_LIB): hakozuna_ldpreload_objs $(HZ3_HYBRID_OBJS) $(HYBRID_SHIM_OBJ)
	$(CC) -shared -o $@ $(HZ3_HYBRID_OBJS) $(HYBRID_SHIM_OBJ) $(HZ_HYBRID_OBJS) \
	    $(LDFLAGS_LDPRELOAD) $(LDLIBS_LDPRELOAD)

.PHONY: all_hybrid_ldpreload
all_hybrid_ldpreload: $(HYBRID_LDPRELOAD_LIB)

.PHONY: print-ldpreload-defs print-ldpreload-defs-extra
print-ldpreload-defs:
	@echo "$(HZ3_LDPRELOAD_DEFS)"
print-ldpreload-defs-extra:
	@echo "$(HZ3_LDPRELOAD_DEFS_EXTRA)"
