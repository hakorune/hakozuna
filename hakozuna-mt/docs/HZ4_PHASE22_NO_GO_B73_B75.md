# HZ4 Phase22 NO-GO Details (B73-B75)

## B73 TransferCache

### 仮説
- size-class 毎の global transfer cache
- lock-free MPSC push で cross-thread object handoff

### 結果
| Version | main_r0 | main_r50 | 備考 |
|---------|---------|----------|------|
| v1 | -1% | +3% | guard_r0 で -2% regression |
| v2 (adaptive) | 0% | -5% | probe overhead が致命的 |
| v3 (bug fix) | +1% | -1% | O(N) list walk 修正も不十分 |

### 失敗原因
1. v1: guard_r0 で cold cache overhead が回復不能
2. v2: adaptive probe の状態管理 overhead
3. v3: 依然として main_r50 で regression

### 教訓
- cross-thread cache は remote-heavy で有効だが、remote-light で overhead
- adaptive mechanism は状態管理コストが予想外に高い
- O(N) 操作は splice でも隠れボトルネックになりうる

---

## B74 MidLockPathOutlockRefill

### 仮説
- lock-path で extra objects を temp list に detach
- lock release 後に alloc_run に push (list linking work を lock 外へ)

### Stage A Results (PAGE_CREATE_OUTSIDE_SC_LOCK_BOX)
| Lane | Delta | Gate | Result |
|------|-------|------|--------|
| guard_r0 | -1% | -1.0% | PASS |
| main_r0 | -1% | +3.0% | FAIL |
| main_r50 | +4% | +1.0% | PASS |
| cross128_r90 | -3% | 0.0% | FAIL |

### Stage B Results (LOCKPATH_OUTLOCK_REFILL_BOX)
| Lane | Base | Var | Delta | Gate | Result |
|------|------|-----|-------|------|--------|
| guard_r0 | 0.079430 | 0.079208 | 0% | -1.0% | PASS |
| main_r0 | 0.143560 | 0.140555 | -2.00% | +3.0% | FAIL |
| main_r50 | 0.308892 | 0.308894 | 0% | +1.0% | FAIL |
| cross128_r90 | 0.193912 | 0.212225 | +9.00% | 0.0% | PASS |

### 失敗原因
- 機構自体は動作 (lock-path 頻度低下、alloc_run hit 増)
- list walk の overhead が benefit を上回る
- cross128_r90 では +9% を確認したが main lanes は未達

---

## B75 Pre-Observation (MidOwnerRemoteDrainPathSlim)

### 仮説
- mid owner remote drain が main bottleneck の可能性
- periodic drain (inefficient batching) が significant portion なら、slim path で改善

### Phase 1 Results (Existing Counters)
| Lane | drain_ratio | objs/drain | Phase 1 Result |
|------|-------------|------------|----------------|
| main_r50 | 8.81% | 2.73 | NO-GO (objs/drain > 2.5) |
| cross128_r90 | 31.02% | 1.73 | GO candidate |

### Phase 2 Results (B75 Probe Counters)
| Lane | should_drain | rc0 (%) | threshold (%) | periodic (%) |
|------|-------------|---------|---------------|--------------|
| main_r50 | 584,901 | 99.60% | 0.005% | 0% |
| cross128_r90 | 164,061 | 99.91% | 0% | 0% |

### 失敗原因
- periodic drain は 0% で仮説が誤り
- periodic path は 0 < rc < THRESHOLD の範囲でのみ動作
- 実際には rc がほぼ常に 0 または THRESHOLD 以上

### Mainline cleanup
- B75 probe-only countersは verdict 後に本線から削除済み

### 詳細
- Report: `/tmp/hz4_b75_probe_phase2_20260216_174237/REPORT.md`
- Data: `/tmp/hz4_b75_probe_phase2_20260216_174237/`
