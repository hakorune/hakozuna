# PHASE_HZ3_S56: ActiveSegmentSet + Pack Best-Fit（segment “開き散らかし”抑制）

目的:
- S55-1 OBSERVE で `arena_used_bytes`（=開いたsegment数 proxy）が 97–99% を支配しているため、RSS gap の本命は **segment/arena 側の保持**。
- `madvise` を増やす前に、**割当側で “segment を増やしにくい（散らばりを作らない）”** を強める。
- hot path は汚さない（slow/event-only 境界だけ）。

前提（既存の箱）:
- S49 SegmentPacking: contig-aware に既存 segment を優先（alloc_failed を大幅に減らした）。
- S47 Quarantine + pinned avoid: “止まらない”ための safety（alloc_failed 抑止）。
- S45: reclaim（central→segment demote / focus reclaim など）保険。
- S55-2/2B/3/3B: RSS目的では NO-GO / アーカイブ（steady-stateで効きづらい）。

---

## S56 の狙い（最小原理）

他アロケータ（tcmalloc/jemalloc/mimalloc）が共通してやっている最小原理:
1) **active span/segment を増やしにくくする**（散らばりを作らない）
2) 必要なら **OS 返却（purge/decay/subrelease）** を “間引き” つきで行う

hz3 は 1) が弱い（steady-state で segment が増えやすい）ため、S56 は **(1)だけ**をまず強化する。

---

## S56-1: Pack Best-Fit（S49の“選び方”だけ強化）

### 背景
S49 は bucket（npages→MAX）で “ちょうど良い” を探すが、各 bucket で「先頭に当たった segment」を返すと、運が悪いと散らばりが残る。

### 変更方針（境界: `hz3_pack_try_alloc()`）
各 bucket で最大 `K` 個だけ候補を取り出し、**best-fit 1本**を返す（コスト上限固定）。

候補条件:
- `max_contig_free(meta) >= npages`（既存関数）
- `meta->owner == my_shard`（既存の shard 前提）
- `!S47 draining`（draining中は除外）

スコア（小さいほど良い）案:
- `score = (free_pages << 8) + (max_contig_free - npages)`
  - `free_pages` が少ない（=より埋まってる）segmentを優先
  - 同点なら contig が “tight” なものを優先

### フラグ（compile-time, A/B）
- `HZ3_S56_PACK_BESTFIT=0/1`（default 0）
- `HZ3_S56_PACK_BESTFIT_K=2/4`（default 2）

### GO/NO-GO
- GO: `segment_new_count` が減る、`arena_used_slots` が増えにくい、SSOTが±2%内
- NO-GO: SSOT -2%超退行、または `alloc_failed` 増加（設計違反）

---

## S56-2: ActiveSegmentSet（medium carve 先を“少数に固定”）

### 背景
steady-state の RSS が高いのは “少しずつ使われている segment が多い”ため。
そこで “active segments を少数に固定し、そこへ run を寄せる”。

### 形（最小）
- per shard で medium 用 active segment を `2本`（`active0/active1`）だけ持つ。
- 新しい medium run は **基本この2本から切る**。
- 2本とも無理なら S49 pack（best-fit）へ。それでも無理なら new segment。

### 境界（実装箇所）
- `hz3_slow_alloc_from_segment()`（medium carve入口、event-only）
  - hot 0コストを守る（fast path では触らない）

### ルール（簡略）
1) `active0/active1` を score で選んで try
2) 失敗なら `max_contig_free` を測って active から外す（or pack_max_fit を下げて poolへ）
3) pack best-fit（S56-1）で候補取得→try
4) 最後に new segment

### フラグ（compile-time, A/B）
- `HZ3_S56_ACTIVE_SET=0/1`（default 0）
- `HZ3_S56_ACTIVE_SET_K=2`（固定）

---

## 観測（最小メトリクス）

S56 の GO/NO-GO は “次の4つ”だけで良い（event-only / one-shot推奨）。

1) `arena_used_slots` / `arena_used_bytes`
2) `segment_new_count`
3) `pack_hit / pack_miss`（S49）
4) `S56_active_hit / miss / evict`

任意（深掘り用）:
- segment の `free_pages` 分布ヒスト（“二極化”しているほど返しやすい）

---

## 測定手順（SSOT）

1) baseline（S56=0）
2) S56-1 のみ（bestfit）
3) S56-1 + S56-2（active set）

必須:
- `bench_random_mixed_mt_remote_malloc`（T=32, R=50, 16–32768）
- `mstress`（RSS timeseries を取る）
- SSOT（small/medium/mixed）回帰（±2%）

GO基準:
- RSS: `mstress mean RSS -10%`（vs baseline）
- 速度: SSOT ±2%以内
- alloc_failed: 増加なし

NO-GO:
- RSSが動かない（-5%未満）かつ速度が落ちる
- hot path に固定費が入ってしまう（設計違反）

---

## S56-1A 測定（2026-01-07）: Pack Best-Fit（K=2）

測定結果（mstress, RUNS=10）:
- mean RSS: **-0.3% 程度**（誤差域、目標 -10% に届かず）

測定結果（SSOT, RUNS=10）:
- small/medium/mixed で **+38〜+41% 退行**（NO-GO）
  - small: 9.05 → 12.73 ns/op（+40.6%）
  - medium: 8.82 → 12.14 ns/op（+37.7%）
  - mixed: 9.25 → 13.07 ns/op（+41.3%）

重要な原因（実装バグ）:
- S56-1A 実装が「K候補を集めた後も `tries` が尽きるまで走査」しており、**bounded のつもりが unbounded になっていた**。
- 結果として pack pool が頻繁に呼ばれる条件（WS/itrs/pressure）で探索コストが爆発し、SSOT が大きく退行。

対応:
- `hz3_pack_try_alloc()` を **K候補を集めた時点で即終了**する形へ修正（S56-1B）。
- 次の判断は **S56-1B の再測定**で行う（S56-1A の数値は “実装ミス込み” として扱う）。

## S56-1B 測定（2026-01-07）: Pack Best-Fit（K=2, Kで即終了）

### SSOT（暫定）
- small: 10.19 → 9.25 ns/op（-9.2%）
- medium: 9.17 → 9.19 ns/op（+0.2%）
- mixed: 9.62 → 10.56 ns/op（+9.8%）

解釈:
- S56-1A の “探索コスト爆発” は修正され、small/medium の退行は解消。
- mixed のみ +9.8% が残るため、S56-1B は現時点では **GO条件（±2%）未達**。

### RSS（再測定が必要）
- RSS測定が baseline=malloc-large / treatment=mstress に分裂しており、比較不能。
- RSS は **同一ワークロードで OFF/ON を揃えて**計測する（auto fallback でベンチが切り替わらないよう固定する）。

### RSS（2026-01-08）: mstress 統一 A/B
| 項目 | Baseline | S56-1B | Delta |
|---|---:|---:|---:|
| Mean RSS | 867,287 KB | 870,467 KB | +0.37% |
| Max RSS | 925,072 KB | 926,064 KB | +0.11% |

結論:
- RSS は **改善なし（微増）**。
- mixed は +9.8% が残っており、S56-1B は **NO-GO**。

次の手:
- S56-2（Active Segment Set）で「pack pool の churn」を減らし、mixed 退行の削減を狙う。

---

## S56-2 測定（2026-01-08）: Active Segment Set（S56-1はOFF）

### SSOT（RUNS=10, ops/s median）

| Workload | Baseline | S56-2 | Delta |
|---|---:|---:|---:|
| small | 113.567M | 113.784M | +0.19% |
| medium | 114.394M | 114.427M | +0.03% |
| mixed | 108.389M | 108.708M | +0.29% |

結論:
- SSOT は **ほぼ同等**（S56-2単体では mixed 退行の削減／改善は見えない）。

### RSS（mstress 10 100 100, /proc RSS timeseries）

| Metric | Baseline | S56-2 | Delta |
|---|---:|---:|---:|
| mean_kb | 869,965 | 868,684 | -0.15% |
| max_kb | 927,564 | 924,748 | -0.30% |
| p95_kb | 927,448 | 924,640 | -0.30% |

結論:
- RSS は **誤差域**（-0.15%）。
- S56-2 も “RSS本線（-10%）” のレバーにはなっていない。

判定:
- **S56-2: NO-GO（現状の目的: RSS -10% に対して効果が無い）**
- ただし SSOT への回帰が無いので、将来の “Active set” 探索の土台としては保持は可能（default OFF）。
