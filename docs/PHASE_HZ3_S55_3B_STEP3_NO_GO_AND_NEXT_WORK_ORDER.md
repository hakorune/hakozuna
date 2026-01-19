# PHASE_HZ3_S55-3B Step3: NO-GO確定と次の手（RSS本命の切り分け）

目的:
- S55-3B（DelayedMediumSubreleaseBox）の **steady-state（mstress/larson）でのNO-GO** を固定し、迷走を止める。
- 「RSSをさらに下げたい」場合の **次の“本命”**（＝subrelease以外の原因）を切り分ける。

前提（Box Theory）:
- hot path を汚さない（event-only / epoch境界のみ）。
- A/B は compile-time `-D` のみ。いつでも戻せる。
- 可視化は one-shot/atexit（常時ログ禁止）。

---

## 1) S55-3B（Delayed purge）の結論

技術的には成立:
- `reused > 0` が出ており “hot run（すぐ再利用されるrun）を避ける” 検出は成立。
- Step2で必須fixも反映済み（head/tail wrap, global epoch）。

RSS目的としてはNO-GO（steady-state）:
- `mstress/larson` の constant high-load では RSS が下がらない/増えるケースがあり、目標（mean RSS -10%）に届かない。
- よって **laneデフォルトに混ぜない**（研究箱として default OFF のまま保持）。

関連ドキュメント:
- 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S55_3B_DELAYED_MEDIUM_SUBRELEASE_WORK_ORDER.md`
- ゲートA/B: `hakozuna/hz3/docs/PHASE_HZ3_S55_3B_STEP2_GATE_AB_WORK_ORDER.md`

---

## 2) 次の“本命”の仮説（S55-1 OBSERVE を根拠にする）

S55-1 OBSERVE では `arena_used_bytes`（segment/arena側proxy）が 97–99% を占める。
ただし、steady-state RSSが落ちない主因が「freeページを返せない」ではなく、次のどれかの可能性が高い:

1. **mediumの内部断片化（size→classの丸めが粗い）**
   - 4KB境界の丸めが粗い場合、4KB直上の要求が 8KB run になり、RSS差が出やすい。
2. **segment open の過多（再利用より新規openが多い）**
   - S49 pack が効いていない/探索が足りない/条件が合わない。
3. **ベンチ側の“touch”がRSSを決めている**
   - allocatorの返却よりも、アプリが書く総ページ数が支配的で、subreleaseが平均RSSに反映されにくい。

→ “返す（madvise）”より前に、**どこで膨らんでいるかの切り分け**が必要。

---

## 3) 指示書（次フェーズ）：S56 Medium丸め/浪費 OBSERVE（最小）

### 3.1 ゴール
- 「どのサイズ帯で、どれだけ丸め損（waste）が出ているか」を atexit で1回だけ出す。
- これが大きいなら、RSS差の本命は “subrelease” ではなく **size class granularity**。

### 3.2 追加する観測（event-onlyではなく“alloc入口のみ”だが、研究用限定）
※この観測は hot に触れるため、**デフォルトOFF/研究用**でのみ実施。

- `requested_bytes_total`
- `allocated_bytes_total`（実際に渡した usable/sizeclass bytes）
- `waste_bytes_total = allocated - requested`
- medium（4KB–32KB）だけに限定し、サイズclass別のカウンタも持つ（例：sc=0..7）。

出力（atexit one-shot）:
- `[HZ3_MEDIUM_WASTE_OBS] req=... alloc=... waste=... waste_pct=...`
- class別: `[HZ3_MEDIUM_WASTE_SC] sc=... req=... alloc=... waste=...`

### 3.3 A/B
- `HZ3_S56_MEDIUM_WASTE_OBSERVE=1`（default 0）

### 3.4 判定
- waste が大きい（例：>10%）なら **“medium sizeclassの細分化”**がRSS差の主因候補。
- waste が小さい（例：<3%）なら、次は “segment open過多” を疑い、S49 pack hit/miss/new_segment を観測する。

---

## 4) もし本命が “medium丸め” だった場合の次案（S57の入口）

ここは実装規模が大きいので、S56の観測で “勝ち筋がある” と確定してから開始する。

候補:
- 4KB–32KB を 4KB刻みではなく、より細かい size class にする（例：8分割/倍など）。
- “run=連続ページ”前提を崩さずに、丸め損を減らす（PTAG設計と整合）。

---

## 5) S55-3/S55-3B の扱い（整理）

- S55-3（immediate madvise）は “効率が悪い” が、**steady-stateでも一定のRSS低下（-5%級）**は出る。
  - 省メモリ mode として残すなら、madvise頻度/予算を固定し、lane（mem-mode）として隔離する。
- S55-3B は “burst/idle” が明確に必要。steady-state SSOTではNO-GOとして固定する。

