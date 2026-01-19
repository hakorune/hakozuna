# S28-2A: Bank Row Cache (NO-GO)

## 概要

tiny hot path の依存チェーン短縮を目的として、`&bank[my_shard][0]` を TLS にキャッシュする最適化を試みた。

## 実装

- `HZ3_TCACHE_BANK_ROW_CACHE=0/1` フラグ追加
- `t_hz3_cache.bank_my` フィールド追加
- `hz3_tcache_get_small_bin()` / `hz3_tcache_get_bin()` で `bank_my[index]` を使用

## アセンブリ変化

Before:
```asm
# bin計算: 7命令
mov    %rcx,%rdx
shl    $0x4,%rdx        # my_shard << 4
add    %rcx,%rdx        # my_shard * 17
lea    0x88(%rax,%rdx,8),%rdx
shl    $0x4,%rdx
add    %fs:0x0,%rdx
add    %rbx,%rdx
```

After:
```asm
# bin計算: 2命令
mov    %fs:0x4c80(%r12),%rbx  # bank_my (cached)
add    %rdi,%rbx              # bin = bank_my + sc*16
```

依存チェーン: 7命令 → 2命令 に短縮成功

## 結果 (tiny 100%, 5 runs)

| 設定 | range | median | vs tcmalloc |
|------|-------|--------|-------------|
| tcmalloc | 63.4M-65.7M | 65.3M | baseline |
| hz3 OFF | 50.8M-55.2M | 52.7M | -19.3% |
| hz3 ON | 49.3M-54.2M | 52.5M | -19.6% |

**A/B差: ±1% 以内（統計誤差内）**

## NO-GO 理由

1. **追加 TLS load**: `bank_my` 自体のロードが追加のメモリアクセスを発生
2. **スーパースカラ相殺**: 元の計算が CPU のスーパースカラ実行で効率よく並列処理されていた
3. **依存チェーン短縮のメリットが load レイテンシで相殺**

## 学び

- アセンブリ命令数削減 ≠ パフォーマンス向上
- TLS 内でも追加の load は依存を増やす
- 依存チェーンの「深さ」より「load の数」が効く場合がある

## 参照

- 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S28_2A_TINY_BANK_ROW_CACHE_WORK_ORDER.md`
- 実装: `HZ3_TCACHE_BANK_ROW_CACHE` (default 0, デッドコード)

## 日付

2026-01-02
