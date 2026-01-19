# S16-2D: hz3_free fastpath shape split (NO-GO)

目的:
- hz3_free の fast/slow を分離して prolog/spill を減らし、mixed の命令数を下げる。

変更内容:
- `HZ3_FREE_FASTPATH_SPLIT=1` で `hz3_free` の分岐先を `noinline` 関数に逃がす。

SSOT (RUNS=10):
- baseline logs: `/tmp/hz3_ssot_6cf087fc6_20260101_152106`
- split logs: `/tmp/hz3_ssot_6cf087fc6_20260101_152212`
- small: 100667439.46 → 100083143.72 (-0.58%)
- medium: 102875818.92 → 101964668.33 (-0.89%)
- mixed: 94342898.98 → 95924636.39 (+1.68%)

perf stat (mixed, RUNS=1):
- baseline: `/tmp/s16_2d_baseline_perf.txt`
- split: `/tmp/s16_2d_split_perf.txt`
- instructions: 1.446B → 1.495B (増加)

objdump:
- baseline: `/tmp/s16_2d_baseline_hz3_free.txt`
- split: `/tmp/s16_2d_split_hz3_free.txt`

判定: NO-GO
- mixed は +1.68% だが目標(+2%〜)に届かず。
- instructions が増加。

復活条件:
- hz3_free の prolog/spill が減り、instructions が減少する生成形が得られた時に再検討。
