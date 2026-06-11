# HZ7 TinyRoute V2 Stop Or Continue Prompt

Use this prompt when asking for an external design review on whether HZ7 v2
should stop here or continue with another focused experiment.

```text
HZ7 v2 の stop / continue 判断を相談したいです。

現在の HZ7 v2 は、tiny route-safe direct-API allocator として以下の状態です。

Identity:
  tiny route-safe direct-API allocator
  local small/medium focused
  very low RSS
  coarse-lock thread safe
  cross-thread free safe
  not remote-throughput optimized

Code size:
  hz7/v2/hz7.c = 907 lines

Current default lane:
  SlowPathOutsideLock-L1
    OS allocation/release を global lock 外に出す
    route/state transition は lock 内で安全に完了

  DirectRetainCap-L2
    H7_DIRECT_RETAIN_CAP = 64 を default 化
    cap128/cap256 は control 扱い

  DirectRetireHelper-L1
    direct free の validation と retain/release dispatch を分離

  RoutePublicKindHelper-L1
    h7_route の lookup と public VALID/INVALID 解釈を分離

Baseline snapshot:
  docs/benchmarks/windows/hz7_v2_baseline_snapshot/
  20260611_174745_paper_random_mixed_windows.md

Windows random_mixed repeat-3, selected cross-allocator rows:

small:
  hz7-v2 79.741M ops/s, 4,576 KB
  fastest: hz3 156.548M
  lowest RSS: hz7-v2

medium:
  hz7-v2 53.353M ops/s, 5,140 KB
  fastest: tcmalloc 154.623M
  lowest RSS: hz7-v2

mixed:
  hz7-v2 52.911M ops/s, 5,664 KB
  fastest: tcmalloc 151.538M
  lowest RSS: hz7-v2

Safety:
  Windows smoke pass:
    hz7_smoke
    hz7_remote_smoke
    hz7_mt_smoke
    hz7_stats_smoke
    hz7_cpp_smoke

  Linux smoke pass:
    hz7_smoke
    hz7_remote_smoke
    hz7_mt_smoke
    hz7_stats_smoke
    hz7_cpp_smoke

Current reading:
  HZ7 v2 は throughput winner ではない。
  ただし、low RSS / tiny readability / route safety / remote-free safety はかなり綺麗。
  cap64 によって medium/mixed は cap32 より大きく改善した。

Non-goals:
  owner-aware remote free
  owner inbox
  TLS ownership
  lock-free remote queue
  remote batching
  HZ6-style profile matrix
  production hot-path diagnostics

質問:

1. HZ7 v2 はここで cap64 tiny reference allocator として closeout してよいか？

2. もし続けるなら、HZ7 v2 の identity を壊さずに ROI がある次の一手は何か？
   候補は以下です。

   A. closeout:
      ここで HZ7 v2 を low-RSS tiny reference として固定する。

   B. OptionalCleanup-L1:
      policy 変更なしの小さな source/list readability cleanup だけ続ける。

   C. Performance-L1:
      baseline snapshot を scoreboard にして、small/medium/mixed のどれかを1本だけ攻める。
      ただし counters / owner / TLS / remote-fast は入れない。

   D. HZ7 v2 を止めて、remote/performance 系は HZ6/HZ8 family に分ける。

3. HZ7 v2 の 907 lines というサイズは、tiny reference として十分に美しいか？
   それとも 1500-2000 lines まで使って、もう少し allocator textbook 的に育てる価値があるか？

4. もし 1500-2000 lines まで増やしてよい場合、何を足すのが一番教育的か？
   ただし remote throughput allocator にはしない前提です。

判断基準:
  keep:
    route safety 維持
    remote-free safety 維持
    RSS 低さ維持
    tiny readability 維持

  no-go:
    owner/inbox/TLS/remote queue が必要になる
    HZ6-style profile family になる
    RSS の強みを失う
    速度だけのためにコードが説明しにくくなる
```

