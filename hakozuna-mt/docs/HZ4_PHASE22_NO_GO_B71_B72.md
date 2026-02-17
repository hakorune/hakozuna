# HZ4 Phase22 NO-GO Details (B71-B72)

## B71 SpanLiteBox

### 仮説
- span leaf を軽量表現 (単一 page のみ) で管理
- lock-path での segment 操作を削減

### 結果
- main_r0: -1% (FAIL)
- main_r50: 0% (FAIL)

### 失敗原因
- 追加の構造管理オーバーヘッド
- lock-path 頻度削減効果なし

---

## B72 SpanLeafRemoteQueue

### 仮説
- 各 span leaf に独立した remote queue を持たせる
- owner 固定による remote drain 効率化

### 結果
- main_r0: 0% (FAIL)
- main_r50: 0% (FAIL)

### 失敗原因
- leaf 単位の queue 配布がオーバーヘッド増
- drain 頻度は変わらず

---

## 補足

- B71/B72 は span 方向の構造変更を試したが、Phase22 目標 lane では効果が再現しなかった。
